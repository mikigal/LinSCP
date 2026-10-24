"""L2 tests of SFTP and SCP against the test VM (milestone M3): what the
local sshd cannot do without root (password, keyboard-interactive and
password change authentication, chroot), and a real network.

The VM is taken from OPENSCP_TEST_VM (tools/testenv/testvm.py, provisioned
with "testvm.py provision"); without it, the tests are skipped. The binary
is taken from OPENSCP_CLI. Results are checked on the VM as root.
"""

import hashlib
import os
import shlex
import shutil
import tempfile
import unittest
import uuid
from pathlib import Path

from openscp_cli import OpenScpCli
from testvm import TestVm, TestVmError

CLI = os.environ.get('OPENSCP_CLI')
VM = None
STATE = None


def setUpModule():
    global VM, STATE
    if not CLI:
        raise unittest.SkipTest('OPENSCP_CLI is not set')
    try:
        VM = TestVm()
    except TestVmError as error:
        raise unittest.SkipTest(str(error))
    if not VM.reachable():
        raise unittest.SkipTest('the test VM %s is not reachable' % VM.host)
    if VM.output('systemctl is-active openscp-sshd.service || true').strip() != 'active':
        raise unittest.SkipTest('the test VM is not provisioned (tools/testenv/testvm.py provision)')
    STATE = Path(tempfile.mkdtemp(prefix='openscp-vm-'))
    result = OpenScpCli(CLI, STATE / 'keygen').run(
        '/keygen', VM.client_key(), '/output=%s' % client_ppk())
    if result.returncode != 0:
        raise RuntimeError('cannot convert the client key: %r' % result)


def tearDownModule():
    if STATE is not None:
        shutil.rmtree(STATE, ignore_errors=True)


def client_ppk():
    return STATE / 'client.ppk'


def setting(name):
    return VM.settings[name]


class VmTestCase(unittest.TestCase):
    protocol = 'sftp'
    user = None

    def setUp(self):
        self.work = Path(tempfile.mkdtemp(prefix='vm-', dir=STATE))
        self.local = self.work / 'local'
        self.local.mkdir()
        self.cli = OpenScpCli(CLI, self.work / 'cli')
        self.user = self.user or setting('SSH_PASSWORD_USER')
        # A directory of the test in the user's home
        self.remote = '%s/test-%s' % (VM.user_home(self.user), uuid.uuid4().hex[:8])
        VM.run('install -d -o %s -g %s %s' % (self.user, self.user, shlex.quote(self.remote)))
        self.addCleanup(VM.run, 'rm -rf %s' % shlex.quote(self.remote))

    def url(self, user=None, password=True):
        credentials = user or self.user
        if password:
            credentials += ':' + VM.password
        return '%s://%s@%s:%d/' % (self.protocol, credentials, VM.host, VM.ssh_port)

    def open_command(self, user=None, password=True, extra=''):
        command = 'open %s -hostkey="%s"' % (self.url(user, password), VM.host_key_fingerprint())
        return (command + ' ' + extra) if extra else command

    def session(self, *commands, check=True, args=(), **open_args):
        result = self.cli.commands(self.open_command(**open_args), *commands, 'exit', args=('/ini=nul',) + tuple(args))
        if check:
            self.assertEqual(result.returncode, 0, result)
        return result

    def remote_sha256(self, path):
        return VM.output('sha256sum %s' % shlex.quote(path)).split()[0]


class PasswordTests:
    def test_password_in_url(self):
        result = self.session('pwd')
        self.assertIn('Authenticating with pre-entered password.', result.stdout)
        self.assertIn(VM.user_home(self.user), result.stdout)

    def test_password_switch(self):
        result = self.session('pwd', password=False, extra='-password=%s' % VM.password)
        self.assertIn('Authenticated.', result.stdout)

    def test_wrong_password(self):
        result = self.cli.commands(
            'open %s://%s:wrong@%s:%d/ -hostkey="%s"' % (
                self.protocol, self.user, VM.host, VM.ssh_port, VM.host_key_fingerprint()),
            'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Authentication failed', result.output)

    def test_password_prompt(self):
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect('[Pp]assword: ')
        console.send(VM.password + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        self.assertNotIn(VM.password, console.text)

    def test_transfer_over_network(self):
        data = os.urandom(32 * 1024 * 1024 + 17)
        source = self.local / 'big.bin'
        source.write_bytes(data)
        self.session(
            'put "%s" "%s/"' % (source, self.remote),
            'get "%s/big.bin" "%s/back.bin"' % (self.remote, self.local))
        digest = hashlib.sha256(data).hexdigest()
        self.assertEqual(self.remote_sha256('%s/big.bin' % self.remote), digest)
        self.assertEqual(hashlib.sha256((self.local / 'back.bin').read_bytes()).hexdigest(), digest)

    def test_operations_as_user(self):
        source = self.local / 'owned.txt'
        source.write_text('owned')
        self.session(
            'cd "%s"' % self.remote,
            'put "%s"' % source,
            'chmod 604 owned.txt',
            'mkdir sub',
            'mv owned.txt sub/')
        output = VM.output('stat -c "%%U %%a" %s/sub/owned.txt' % shlex.quote(self.remote)).split()
        self.assertEqual(output, [self.user, '604'])

    def test_permission_denied(self):
        result = self.session('put "%s" /root/' % (self.work / 'cli'), check=False)
        self.assertEqual(result.returncode, 1, result)


    def remote_tree(self, path):
        """{relative path: sha256 or None for directories} on the VM"""
        output = VM.output(
            'cd %s && find . -mindepth 1 -type d -printf "%%P\\n" | sed "s/$/ -/"; '
            'find . -type f -printf "%%P\\0" | xargs -0 -r sha256sum | sed "s/^\\([0-9a-f]*\\)  \\(.*\\)$/\\2 \\1/"'
            % shlex.quote(path))
        result = {}
        for line in output.splitlines():
            name, digest = line.rsplit(' ', 1)
            result[name] = None if digest == '-' else digest
        return result

    def local_tree(self, path):
        result = {}
        for entry in sorted(Path(path).rglob('*')):
            name = str(entry.relative_to(path))
            result[name] = None if entry.is_dir() else hashlib.sha256(entry.read_bytes()).hexdigest()
        return result

    def test_synchronize(self):
        tree = self.local / 'tree'
        (tree / 'sub' / 'deeper').mkdir(parents=True)
        (tree / 'a.bin').write_bytes(os.urandom(300000))
        (tree / 'sub' / 'b.txt').write_text('b')
        (tree / 'sub' / 'deeper' / 'c.txt').write_text('c')
        VM.run('install -d -o {0} -g {0} {1}/tree && touch {1}/tree/obsolete && chown {0} {1}/tree/obsolete'.format(
            self.user, shlex.quote(self.remote)))
        self.session('synchronize remote -delete "%s" "%s/tree"' % (tree, self.remote))
        self.assertEqual(self.remote_tree('%s/tree' % self.remote), self.local_tree(tree))
        # And back
        back = self.local / 'back'
        back.mkdir()
        self.session('synchronize local "%s" "%s/tree"' % (back, self.remote))
        self.assertEqual(self.local_tree(back), self.local_tree(tree))


class SftpPasswordTest(PasswordTests, VmTestCase):
    protocol = 'sftp'

    def test_resume_upload(self):
        data = os.urandom(8 * 1024 * 1024)
        source = self.local / 'resume.bin'
        source.write_bytes(data)
        target = '%s/resume.bin' % self.remote
        VM.run('cat > {0} && chown {1} {0}'.format(shlex.quote(target), self.user), input=data[:3000000])
        self.session('put -resume "%s" "%s/"' % (source, self.remote))
        self.assertEqual(self.remote_sha256(target), hashlib.sha256(data).hexdigest())

    def test_resume_download(self):
        data = os.urandom(8 * 1024 * 1024)
        source = '%s/resume.bin' % self.remote
        VM.run('cat > {0} && chown {1} {0}'.format(shlex.quote(source), self.user), input=data)
        partial = self.local / 'resume.bin'
        partial.write_bytes(data[:5000000])
        self.session('get -resume "%s" "%s/"' % (source, self.local))
        self.assertEqual(partial.read_bytes(), data)


class ScpPasswordTest(PasswordTests, VmTestCase):
    protocol = 'scp'


class KeyboardInteractiveTest(VmTestCase):
    def setUp(self):
        self.user = setting('SSH_KBD_USER')
        super().setUp()

    def test_pre_entered_password(self):
        # The password answers the prompt of PAM
        result = self.session('pwd')
        self.assertIn('Authenticated.', result.stdout)

    def test_prompt(self):
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect('[Pp]assword: ')
        console.send(VM.password + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)


class PasswordChangeTest(VmTestCase):
    NEW_PASSWORD = 'Changed-pass-2'

    def setUp(self):
        self.user = setting('SSH_EXPIRED_USER')
        super().setUp()
        self.addCleanup(self.expire)

    def expire(self):
        VM.run('echo %s:%s | chpasswd && chage -d 0 %s' % (self.user, VM.password, self.user))

    def test_change(self):
        # Interactively: after a pre-entered password, scripting asks for no
        # other credentials (as upstream)
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect('Password: ')
        console.send(VM.password + '\n')
        # PAM asks for the current password again and a new one
        console.expect('You are required to change your password')
        console.expect('Current password: ')
        console.send(VM.password + '\n')
        console.expect('New password: ')
        console.send(self.NEW_PASSWORD + '\n')
        console.expect(r'Retype new password: ')
        console.send(self.NEW_PASSWORD + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        self.assertNotIn(self.NEW_PASSWORD, console.text)
        # The new password works now
        result = self.cli.commands(
            'open %s://%s:%s@%s:%d/ -hostkey="%s"' % (
                self.protocol, self.user, self.NEW_PASSWORD, VM.host, VM.ssh_port, VM.host_key_fingerprint()),
            'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 0, result)

    def test_batch_fails(self):
        result = self.session(check=False)
        self.assertEqual(result.returncode, 1, result)


class TwoFactorTest(VmTestCase):
    def setUp(self):
        self.user = setting('SSH_TWOFACTOR_USER')
        super().setUp()

    def test_key_and_password(self):
        result = self.session('pwd', extra='-privatekey="%s"' % client_ppk())
        self.assertIn('Authenticated.', result.stdout)

    def test_key_only_fails(self):
        result = self.session('pwd', check=False, password=False, extra='-privatekey="%s"' % client_ppk())
        self.assertEqual(result.returncode, 1, result)


class ChrootTest(VmTestCase):
    def setUp(self):
        self.user = setting('SSH_CHROOT_USER')
        # (The home is not reachable in the chroot)
        self.work = Path(tempfile.mkdtemp(prefix='vm-', dir=STATE))
        self.local = self.work / 'local'
        self.local.mkdir()
        self.cli = OpenScpCli(CLI, self.work / 'cli')
        self.addCleanup(VM.run, 'rm -rf /srv/openscp/chroot/upload/*')

    def test_chroot(self):
        source = self.local / 'jailed.txt'
        source.write_text('jailed')
        result = self.session('pwd', 'put "%s" /upload/' % source)
        self.assertIn('/', result.stdout.splitlines())
        self.assertEqual(VM.output('cat /srv/openscp/chroot/upload/jailed.txt'), 'jailed')

    def test_no_shell(self):
        result = self.session('call echo hello', check=False)
        self.assertEqual(result.returncode, 1, result)


class CopyIdTest(VmTestCase):
    def setUp(self):
        self.user = setting('SSH_COPYID_USER')
        super().setUp()
        self.ssh_dir = '%s/.ssh' % VM.user_home(self.user)
        VM.run('rm -rf %s' % shlex.quote(self.ssh_dir))
        self.addCleanup(VM.run, 'rm -rf %s' % shlex.quote(self.ssh_dir))

    def test_copyid(self):
        result = self.cli.run(
            '/ini=nul', '/copyid', '/identity=%s' % client_ppk(), self.url(),
            '-hostkey=%s' % VM.host_key_fingerprint())
        self.assertEqual(result.returncode, 0, result)
        public_key = Path('%s.pub' % VM.client_key()).read_text().split()[1]
        self.assertIn(public_key, VM.output('cat %s/authorized_keys' % shlex.quote(self.ssh_dir)))
        self.assertEqual(VM.output('stat -c %%a %s' % shlex.quote(self.ssh_dir)).strip(), '700')
        # The key works now
        result = self.session('pwd', password=False, extra='-privatekey="%s"' % client_ppk())
        self.assertIn('Authenticated.', result.stdout)


if __name__ == '__main__':
    unittest.main()
