"""Common parts of the L2 tests against the test VM (tools/testenv/testvm.py):
the VM, an isolated linscp-cli per test, the remote files of the tests as
seen on the VM, and the matrix of tests that every protocol passes.

A test module calls set_up_module() with the services it needs from its
setUpModule (the module is skipped without the VM, see LINSCP_TEST_VM) and
tear_down_module() from its tearDownModule. The binary is taken from
LINSCP_CLI. Results are checked on the VM, as root.
"""

import hashlib
import os
import re
import shlex
import shutil
import tempfile
import unittest
import uuid
from datetime import datetime
from pathlib import Path
from urllib.parse import quote

from linscp_cli import LinScpCli
from testvm import TestVm, TestVmError

CLI = os.environ.get('LINSCP_CLI')
# The VM and a temporary directory of the module
VM = None
STATE = None


def set_up_module(*services):
    global VM, STATE
    if not CLI:
        raise unittest.SkipTest('LINSCP_CLI is not set')
    try:
        VM = TestVm()
    except TestVmError as error:
        raise unittest.SkipTest(str(error))
    if not VM.reachable():
        raise unittest.SkipTest('the test VM %s is not reachable' % VM.host)
    if not VM.services_active(*services):
        raise unittest.SkipTest('the test VM is not provisioned (tools/testenv/testvm.py provision)')
    STATE = Path(tempfile.mkdtemp(prefix='linscp-vm-'))


def tear_down_module():
    if STATE is not None:
        shutil.rmtree(STATE, ignore_errors=True)


def setting(name):
    return VM.settings[name]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def local_tree(path):
    """{relative path: sha256, or None for directories}"""
    result = {}
    for entry in sorted(Path(path).rglob('*')):
        name = str(entry.relative_to(path))
        result[name] = None if entry.is_dir() else sha256(entry.read_bytes())
    return result


class RemoteFiles:
    """The files on a server of the VM, at the paths the client uses."""

    def make_dir(self, path):
        raise NotImplementedError

    def remove(self, path):
        raise NotImplementedError

    def write(self, path, data):
        raise NotImplementedError

    def tree(self, path):
        """{relative path: sha256, or None for directories} under the path"""
        raise NotImplementedError

    def sha256(self, path):
        directory, name = path.rsplit('/', 1)
        return self.tree(directory).get(name)


class FileSystemFiles(RemoteFiles):
    """Files in the file system of the VM: remote paths below Root, owned by
    Owner."""

    def __init__(self, root, owner):
        self.root = root
        self.owner = owner

    def vm_path(self, path):
        return self.root + path

    def make_dir(self, path):
        VM.run('install -d -o {0} -g {0} {1}'.format(self.owner, shlex.quote(self.vm_path(path))))

    def remove(self, path):
        VM.run('rm -rf %s' % shlex.quote(self.vm_path(path)))

    def write(self, path, data, mode=None, mtime=None):
        target = shlex.quote(self.vm_path(path))
        command = 'cat > {0} && chown {1}:{1} {0}'.format(target, self.owner)
        if mode is not None:
            command += ' && chmod %o %s' % (mode, target)
        if mtime is not None:
            command += ' && touch -d @%d %s' % (mtime, target)
        VM.run(command, input=data)

    def tree(self, path):
        output = VM.output(
            'cd %s && find . -mindepth 1 -type d -printf "%%P\\n" | sed "s/$/ -/"; '
            'find . -type f -printf "%%P\\0" | xargs -0 -r sha256sum | sed "s/^\\([0-9a-f]*\\)  \\(.*\\)$/\\2 \\1/"'
            % shlex.quote(self.vm_path(path)))
        result = {}
        for line in output.splitlines():
            name, digest = line.rsplit(' ', 1)
            result[name] = None if digest == '-' else digest
        return result

    def stat(self, path, format):
        return VM.output('stat -c %s %s' % (shlex.quote(format), shlex.quote(self.vm_path(path)))).strip()


class S3Objects(RemoteFiles):
    """Objects of the S3 server of the VM (linscp-s3): the first component
    of a path is the bucket; directories are keys ending with a slash, or
    implied by the keys."""

    @staticmethod
    def split(path):
        bucket, _, key = path.strip('/').partition('/')
        return bucket, key

    def make_dir(self, path):
        bucket, key = self.split(path)
        if key:
            VM.run('linscp-s3 put %s %s' % (bucket, shlex.quote(key + '/')), input=b'')
        else:
            VM.run('linscp-s3 mb %s' % bucket)

    def remove(self, path):
        bucket, key = self.split(path)
        if key:
            raise NotImplementedError('only buckets are removed')
        VM.run('linscp-s3 rb %s' % bucket, check=False)

    def write(self, path, data):
        bucket, key = self.split(path)
        VM.run('linscp-s3 put %s %s' % (bucket, shlex.quote(key)), input=data)

    def tree(self, path):
        bucket, key = self.split(path)
        prefix = (key + '/') if key else ''
        result = {}
        for line in VM.output('linscp-s3 tree %s %s' % (bucket, shlex.quote(prefix))).splitlines():
            name, digest = line.rsplit(' ', 1)
            name = name[len(prefix):]
            parts = name.rstrip('/').split('/')
            for index in range(1, len(parts)):
                result['/'.join(parts[:index])] = None
            if name.endswith('/'):
                result[name.rstrip('/')] = None
            elif name:
                result[name] = digest
        return result


class VmTestCase(unittest.TestCase):
    """A test with an isolated CLI and a remote directory of its own
    (self.remote, at the path the client uses)."""

    # The URL scheme and port of the server
    protocol = None
    port = None
    # The credentials of the test user
    user = None
    password = None
    # The message of a failed authentication, and the password prompt
    authentication_failed = 'Authentication failed'
    password_prompt = '[Pp]assword: '

    def remote_base(self):
        """The directory in which the tests make their directories."""
        raise NotImplementedError

    def files(self):
        """The RemoteFiles of the server."""
        raise NotImplementedError

    def open_switches(self):
        """Switches of the open command (host key, certificate, ...)."""
        return []

    def setUp(self):
        self.work = Path(tempfile.mkdtemp(prefix='vm-', dir=STATE))
        self.local = self.work / 'local'
        self.local.mkdir()
        self.cli = LinScpCli(CLI, self.work / 'cli')
        self.remote = '%s/test-%s' % (self.remote_base().rstrip('/'), uuid.uuid4().hex[:8])
        self.files().make_dir(self.remote)
        self.addCleanup(self.files().remove, self.remote)

    def credentials(self, user=None, password=True):
        result = quote(user or self.user, safe='')
        if password:
            result += ':' + quote(password if isinstance(password, str) else self.password, safe='')
        return result

    def url(self, user=None, password=True):
        return '%s://%s@%s:%d/' % (self.protocol, self.credentials(user, password), VM.host, self.port)

    def open_command(self, user=None, password=True, extra=()):
        return ' '.join(['open', self.url(user, password)] + list(self.open_switches()) + list(extra))

    def session(self, *commands, check=True, args=(), **open_args):
        result = self.cli.commands(
            self.open_command(**open_args), *commands, 'exit', args=('/ini=nul',) + tuple(args))
        if check:
            self.assertEqual(result.returncode, 0, result)
        return result

    def write_local(self, name, data):
        path = self.local / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def assert_certificate_validity(self, output):
        """Checks the validity period in the details of the server's
        certificate that the CLI printed (in UTC, the time zone of the tests)
        against the certificate on the VM."""
        match = re.search(r'Valid: (\S+ \S+) \u2013 (\S+ \S+)', output)
        self.assertIsNotNone(match, output)
        shown = tuple(datetime.strptime(value, '%m/%d/%Y %H:%M:%S') for value in match.groups())
        self.assertEqual(shown, VM.certificate_validity(), output)


class TransferTests:
    """The tests that every protocol passes (a mixin of VmTestCase)."""

    # What the protocol supports
    supports_permissions = True
    supports_resume_upload = True
    supports_resume_download = True

    def test_session(self):
        result = self.session('pwd', 'ls')
        self.assertIn('Session started.', result.stdout)

    def test_wrong_password(self):
        result = self.cli.commands(
            self.open_command(password='wrong-%s' % self.password), 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertIn(self.authentication_failed, result.output)

    def test_password_prompt(self):
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect(self.password_prompt)
        console.send(self.password + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        self.assertNotIn(self.password, console.text)

    def test_transfer_over_network(self):
        data = os.urandom(32 * 1024 * 1024 + 17)
        source = self.write_local('big.bin', data)
        self.session(
            'put "%s" "%s/"' % (source, self.remote),
            'get "%s/big.bin" "%s/back.bin"' % (self.remote, self.local))
        self.assertEqual(self.files().sha256('%s/big.bin' % self.remote), sha256(data))
        self.assertEqual(sha256((self.local / 'back.bin').read_bytes()), sha256(data))

    def test_operations(self):
        source = self.write_local('file.txt', b'operations')
        self.session(
            'cd "%s"' % self.remote,
            'put "%s"' % source,
            'mkdir sub',
            'mkdir other',
            'mv file.txt sub/renamed.txt',
            'rmdir other',
            *(['chmod 604 sub/renamed.txt'] if self.supports_permissions else []))
        self.assertEqual(self.files().tree(self.remote), {
            'sub': None,
            'sub/renamed.txt': sha256(b'operations'),
        })
        if self.supports_permissions:
            self.assertEqual(self.files().stat('%s/sub/renamed.txt' % self.remote, '%a %U'), '604 %s' % self.user)
        self.session('rm "%s/sub/renamed.txt"' % self.remote, 'rmdir "%s/sub"' % self.remote)
        self.assertEqual(self.files().tree(self.remote), {})

    def test_names(self):
        names = ['with space.txt', 'za\u017c\u00f3\u0142\u0107 \U0001F600.txt', 'semi;colon\'quote.txt']
        for name in names:
            self.write_local('names/' + name, name.encode())
        self.session('put "%s/names" "%s/"' % (self.local, self.remote))
        expected = {'names': None}
        expected.update({'names/' + name: sha256(name.encode()) for name in names})
        self.assertEqual(self.files().tree(self.remote), expected)
        result = self.session('ls "%s/names"' % self.remote)
        for name in names:
            self.assertIn(name, result.stdout)
        back = self.local / 'back'
        back.mkdir()
        self.session('get "%s/names/*" "%s/"' % (self.remote, back))
        self.assertEqual(sorted(path.name for path in back.iterdir()), sorted(names))

    def test_listing(self):
        self.files().write('%s/listed.bin' % self.remote, os.urandom(12345))
        self.files().make_dir('%s/listed-dir' % self.remote)
        result = self.session('ls "%s"' % self.remote)
        lines = result.stdout.splitlines()
        self.assertTrue(any(line.endswith(' listed.bin') and ' 12345 ' in line for line in lines), result)
        # (D or the permissions of a directory)
        self.assertTrue(any(line.endswith(' listed-dir') and line[0] in 'Dd' for line in lines), result)

    def test_missing_file(self):
        result = self.session('get "%s/missing.txt" "%s/"' % (self.remote, self.local), check=False)
        self.assertEqual(result.returncode, 1, result)
        self.assertFalse((self.local / 'missing.txt').exists())

    def test_synchronize(self):
        tree = self.local / 'tree'
        (tree / 'sub' / 'deeper').mkdir(parents=True)
        (tree / 'a.bin').write_bytes(os.urandom(300000))
        (tree / 'sub' / 'b.txt').write_text('b')
        (tree / 'sub' / 'deeper' / 'c.txt').write_text('c')
        self.files().make_dir('%s/tree' % self.remote)
        self.files().write('%s/tree/obsolete' % self.remote, b'obsolete')
        self.session('synchronize remote -delete "%s" "%s/tree"' % (tree, self.remote))
        self.assertEqual(self.files().tree('%s/tree' % self.remote), local_tree(tree))
        # And back
        back = self.local / 'back'
        back.mkdir()
        self.session('synchronize local "%s" "%s/tree"' % (back, self.remote))
        self.assertEqual(local_tree(back), local_tree(tree))

    def test_resume_upload(self):
        if not self.supports_resume_upload:
            self.skipTest('the protocol does not resume uploads')
        data = os.urandom(8 * 1024 * 1024)
        source = self.write_local('resume.bin', data)
        target = '%s/resume.bin' % self.remote
        self.files().write(target, data[:3000000])
        self.session('put -resume "%s" "%s/"' % (source, self.remote))
        self.assertEqual(self.files().sha256(target), sha256(data))

    def test_resume_download(self):
        if not self.supports_resume_download:
            self.skipTest('the protocol does not resume downloads')
        data = os.urandom(8 * 1024 * 1024)
        self.files().write('%s/resume.bin' % self.remote, data)
        partial = self.write_local('resume.bin', data[:5000000])
        self.session('get -resume "%s/resume.bin" "%s/"' % (self.remote, self.local))
        self.assertEqual(partial.read_bytes(), data)
