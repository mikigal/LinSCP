"""L2 tests of SFTP and SCP: openscp-cli against the local test sshd
(milestone M3, docs/port/PLAN.md section 6.2).

The local sshd runs as the current user with public key authentication
only, so the remote side is the local file system: the results are checked
on the disk directly. The binary is taken from OPENSCP_CLI.
"""

import hashlib
import os
import re
import shutil
import signal
import subprocess
import tempfile
import threading
import time
import unittest
from pathlib import Path

from local_sshd import LocalSshd
from openscp_cli import OpenScpCli

CLI = os.environ.get('OPENSCP_CLI')
SSHD = None
STATE = None
STATE_KEPT = False


def setUpModule():
    global SSHD, STATE
    if not CLI:
        raise unittest.SkipTest('OPENSCP_CLI is not set')
    STATE = Path(tempfile.mkdtemp(prefix='openscp-l2-'))
    SSHD = LocalSshd(STATE / 'sshd').start()
    # The key in PuTTY's format, converted by the CLI itself
    cli = OpenScpCli(CLI, STATE / 'keygen')
    result = cli.run('/keygen', SSHD.client_key, '/output=%s' % key_file())
    if result.returncode != 0:
        raise RuntimeError('cannot convert the client key: %r' % result)


def tearDownModule():
    if SSHD is not None:
        SSHD.stop()
    if (STATE is not None) and not STATE_KEPT:
        shutil.rmtree(STATE, ignore_errors=True)


def key_file():
    return STATE / 'client.ppk'


def host_key_fingerprint(algorithm='sha256'):
    """The fingerprint of the server's host key as WinSCP takes it."""
    output = subprocess.run(
        ['ssh-keygen', '-l', '-E', algorithm, '-f', '%s.pub' % SSHD.host_key],
        capture_output=True, text=True, check=True).stdout.split()
    checksum = output[1].split(':', 1)[1]
    return 'ssh-ed25519 255 %s' % checksum


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def tree(root):
    """{relative path: content or None for directories}"""
    root = Path(root)
    result = {}
    for path in sorted(root.rglob('*')):
        name = str(path.relative_to(root))
        result[name] = None if path.is_dir() else path.read_bytes()
    return result


class CliTestCase(unittest.TestCase):
    """The common part of the protocol tests; protocol is set by the subclasses."""
    protocol = None

    def setUp(self):
        self.work = Path(tempfile.mkdtemp(prefix='cli-', dir=STATE))
        self.local = self.work / 'local'
        self.local.mkdir()
        self.remote = SSHD.root / self.work.name
        self.remote.mkdir()
        self.cli = OpenScpCli(CLI, self.work / 'cli')

    def run(self, result=None):
        # The directories of a failed test are kept for inspection
        result = result or self.defaultTestResult()
        problems = len(result.failures) + len(result.errors)
        super().run(result)
        if (len(result.failures) + len(result.errors) == problems) and hasattr(self, 'work'):
            shutil.rmtree(self.work, ignore_errors=True)
            shutil.rmtree(self.remote, ignore_errors=True)
        elif hasattr(self, 'work'):
            global STATE_KEPT
            STATE_KEPT = True
            print('keeping %s and %s for inspection' % (self.work, self.remote))
        return result

    def url(self, user=None):
        return '%s://%s@%s:%d/' % (self.protocol, user or SSHD.user, SSHD.host, SSHD.port)

    def open_command(self, user=None, hostkey=None, privatekey=None, extra=''):
        command = 'open %s -hostkey="%s"' % (self.url(user), hostkey or host_key_fingerprint())
        if privatekey is not False:
            command += ' -privatekey="%s"' % (privatekey or key_file())
        if extra:
            command += ' ' + extra
        return command

    def session(self, *commands, args=(), check=True, env=None, input=None, open_args=None):
        """Runs the commands in a session (open ... exit); /ini=nul unless args say otherwise."""
        if not any(str(arg).lower().startswith('/ini=') for arg in args):
            args = ('/ini=nul',) + tuple(args)
        result = self.cli.commands(
            self.open_command(**(open_args or {})), *commands, 'exit', args=args, env=env, input=input)
        if check:
            self.assertEqual(result.returncode, 0, result)
        return result

    def write_local(self, name, data):
        path = self.local / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data if isinstance(data, bytes) else data.encode())
        return path


class CommonTests:
    """Tests that run for both SFTP and SCP."""

    def test_connect(self):
        result = self.session('cd "%s"' % self.remote, 'pwd')
        self.assertIn('Authenticated.', result.stdout)
        self.assertEqual(result.stdout.splitlines()[-1], str(self.remote))

    def test_put_and_get(self):
        data = os.urandom(3 * 1024 * 1024 + 123)
        source = self.write_local('data.bin', data)
        mtime = 1600000000
        os.utime(source, (mtime, mtime))
        self.session(
            'put "%s" "%s/"' % (source, self.remote),
            'get "%s/data.bin" "%s/copy.bin"' % (self.remote, self.local))
        self.assertEqual((self.remote / 'data.bin').read_bytes(), data)
        self.assertEqual((self.local / 'copy.bin').read_bytes(), data)
        # Timestamps are preserved by default
        self.assertEqual(int((self.remote / 'data.bin').stat().st_mtime), mtime)
        self.assertEqual(int((self.local / 'copy.bin').stat().st_mtime), mtime)

    def test_directories(self):
        self.write_local('dir/a.txt', 'a')
        self.write_local('dir/sub/b.txt', 'b')
        self.write_local('dir/sub/deeper/c.bin', os.urandom(1000))
        (self.local / 'dir' / 'empty').mkdir()
        self.session(
            'put "%s/dir" "%s/"' % (self.local, self.remote),
            'get "%s/dir" "%s/back"' % (self.remote, self.local))
        self.assertEqual(tree(self.remote / 'dir'), tree(self.local / 'dir'))
        self.assertEqual(tree(self.local / 'back'), tree(self.local / 'dir'))

    def test_file_operations(self):
        self.write_local('f.txt', 'content')
        self.session(
            'cd "%s"' % self.remote,
            'mkdir newdir',
            'put "%s/f.txt"' % self.local,
            'mv f.txt newdir/g.txt',
            'chmod 640 newdir/g.txt',
            'mkdir gone',
            'rmdir gone')
        moved = self.remote / 'newdir' / 'g.txt'
        self.assertEqual(moved.read_text(), 'content')
        self.assertEqual(moved.stat().st_mode & 0o777, 0o640)
        self.assertFalse((self.remote / 'f.txt').exists())
        self.assertFalse((self.remote / 'gone').exists())
        self.session('rm "%s/newdir/g.txt"' % self.remote)
        self.assertFalse(moved.exists())

    def test_listing(self):
        (self.remote / 'listed.txt').write_text('12345')
        (self.remote / 'subdir').mkdir()
        result = self.session('ls "%s"' % self.remote)
        lines = {line.split()[-1]: line for line in result.stdout.splitlines()
                 if line.endswith((' listed.txt', ' subdir'))}
        self.assertEqual(len(lines), 2, result)
        self.assertTrue(lines['listed.txt'].startswith('-rw'), lines)
        self.assertIn(' 5 ', lines['listed.txt'])
        self.assertTrue(lines['subdir'].startswith('d'), lines)

    def test_permissions(self):
        source = self.write_local('p.txt', 'x')
        self.session('put -permissions=600 "%s" "%s/"' % (source, self.remote))
        self.assertEqual((self.remote / 'p.txt').stat().st_mode & 0o777, 0o600)

    def test_file_mask(self):
        self.write_local('m/keep.txt', '1')
        self.write_local('m/skip.bin', '2')
        self.session('put -filemask="*.txt" "%s/m" "%s/"' % (self.local, self.remote))
        self.assertTrue((self.remote / 'm' / 'keep.txt').exists())
        self.assertFalse((self.remote / 'm' / 'skip.bin').exists())

    def test_delete_after_transfer(self):
        source = self.write_local('moved.txt', 'move me')
        self.session('put -delete "%s" "%s/"' % (source, self.remote))
        self.assertFalse(source.exists())
        self.assertEqual((self.remote / 'moved.txt').read_text(), 'move me')

    def test_unicode_names(self):
        name = 'za\u017c\u00f3\u0142\u0107 g\u0119\u015bl\u0105 \u2603 \U0001F600 "q" \'s\'.txt'
        source = self.write_local(name, 'unicode')
        (self.local / 'back').mkdir()
        result = self.session(
            'put "%s" "%s/"' % (str(source).replace('"', '""'), self.remote),
            'ls "%s"' % self.remote,
            'get "%s/*" "%s/back/"' % (self.remote, self.local))
        self.assertEqual((self.remote / name).read_text(), 'unicode')
        self.assertEqual((self.local / 'back' / name).read_text(), 'unicode')
        self.assertIn(name, result.stdout)

    def test_names_valid_on_linux_only(self):
        # Invalid on Windows (and so encoded there), not on Linux
        names = ['a:b.txt', 'q?.txt', 'star*.txt', 'pipe|.txt', 'back\\slash.txt', 'trailing.', 'trailing ', 'CON',
                 'lt<gt>.txt']
        for name in names:
            (self.remote / name).write_text(name)
        (self.local / 'back').mkdir()
        self.session('get "%s/*" "%s/back/"' % (self.remote, self.local))
        self.assertEqual(sorted(p.name for p in (self.local / 'back').iterdir()), sorted(names))

    def test_neweronly_download(self):
        (self.remote / 'n.txt').write_text('remote')
        os.utime(self.remote / 'n.txt', (1000000000, 1000000000))
        newer = self.write_local('n.txt', 'local')
        self.session('get -neweronly "%s/n.txt" "%s/"' % (self.remote, self.local))
        self.assertEqual(newer.read_text(), 'local')

    def test_synchronize_remote(self):
        self.write_local('s/a.txt', 'a')
        self.write_local('s/sub/b.txt', 'b')
        (self.remote / 's').mkdir()
        (self.remote / 's' / 'obsolete.txt').write_text('old')
        self.session('synchronize remote -delete "%s/s" "%s/s"' % (self.local, self.remote))
        self.assertEqual(tree(self.remote / 's'), tree(self.local / 's'))

    def test_synchronize_local(self):
        (self.remote / 's' / 'sub').mkdir(parents=True)
        (self.remote / 's' / 'a.txt').write_text('a')
        (self.remote / 's' / 'sub' / 'b.txt').write_text('b')
        self.write_local('s/stale.txt', 'stale')
        self.session('synchronize local -delete "%s/s" "%s/s"' % (self.local, self.remote))
        self.assertEqual(tree(self.local / 's'), tree(self.remote / 's'))

    def test_synchronize_both(self):
        self.write_local('s/local.txt', 'l')
        (self.remote / 's').mkdir()
        (self.remote / 's' / 'remote.txt').write_text('r')
        self.session('synchronize both "%s/s" "%s/s"' % (self.local, self.remote))
        self.assertEqual(sorted(tree(self.local / 's')), ['local.txt', 'remote.txt'])
        self.assertEqual(tree(self.local / 's'), tree(self.remote / 's'))

    def test_synchronize_preview(self):
        self.write_local('s/new.txt', 'n')
        (self.remote / 's').mkdir()
        result = self.session('synchronize remote -preview "%s/s" "%s/s"' % (self.local, self.remote))
        self.assertIn('new.txt', result.stdout)
        self.assertFalse((self.remote / 's' / 'new.txt').exists())

    def test_error_exit_code(self):
        result = self.session('get "%s/missing.txt" "%s/"' % (self.remote, self.local), check=False)
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('missing.txt', result.output)

    def test_batch_continue(self):
        source = self.write_local('after.txt', 'after')
        result = self.session(
            'option batch continue',
            'get "%s/missing.txt" "%s/"' % (self.remote, self.local),
            'put "%s" "%s/"' % (source, self.remote),
            check=False)
        self.assertEqual(result.returncode, 1, result)
        self.assertTrue((self.remote / 'after.txt').exists())

    def test_xml_log(self):
        source = self.write_local('logged.txt', 'x')
        log = self.work / 'log.xml'
        self.session('put "%s" "%s/"' % (source, self.remote), args=('/xmllog=%s' % log,))
        text = log.read_text()
        self.assertIn('<upload>', text)
        self.assertIn('logged.txt', text)
        self.assertIn('<result success="true" />', text)

    def test_session_log(self):
        log = self.work / 'session.log'
        self.session('pwd', args=('/log=%s' % log, '/loglevel=1'))
        text = log.read_text(errors='replace')
        self.assertIn('Authenticated.', text)

    def test_wrong_host_key(self):
        result = self.session(
            'pwd', check=False, open_args={'hostkey': 'ssh-ed25519 255 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'})
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Host key does not match', result.output)

    def test_unknown_host_key(self):
        result = self.cli.commands(
            'open %s -privatekey="%s"' % (self.url(), key_file()), 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertIn(host_key_fingerprint().split()[-1], result.output)

    def test_any_host_key(self):
        self.session('pwd', open_args={'hostkey': '*'})

    def test_md5_host_key(self):
        md5 = host_key_fingerprint('md5').split()[-1]
        self.session('pwd', open_args={'hostkey': md5})

    def test_wrong_user(self):
        result = self.session('pwd', check=False, open_args={'user': 'nobody-here'})
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Authentication failed', result.output)

    def test_agent(self):
        socket = self.work / 'agent.sock'
        agent = subprocess.run(['ssh-agent', '-a', str(socket), '-s'], capture_output=True, text=True, check=True)
        pid = int(agent.stdout.split('SSH_AGENT_PID=')[1].split(';')[0])
        try:
            env = {'SSH_AUTH_SOCK': str(socket)}
            subprocess.run(['ssh-add', str(SSHD.client_key)], env=dict(os.environ, **env),
                           capture_output=True, check=True)
            result = self.session('pwd', env=env, open_args={'privatekey': False})
            self.assertIn('Authenticated', result.stdout)
        finally:
            os.kill(pid, signal.SIGTERM)

    def test_passphrase(self):
        protected = self.work / 'protected.ppk'
        result = self.cli.run('/keygen', key_file(), '/output=%s' % protected, '/changepassphrase=secret')
        self.assertEqual(result.returncode, 0, result)
        self.session('pwd', open_args={'privatekey': protected, 'extra': '-passphrase=secret'})
        # Without the passphrase, the prompt fails in batch mode
        result = self.session('pwd', check=False, open_args={'privatekey': protected})
        self.assertEqual(result.returncode, 1, result)

    def test_script_file_and_parameters(self):
        source = self.write_local('scripted.txt', 'scripted')
        script = self.work / 'script.txt'
        script.write_text('\n'.join([
            self.open_command(),
            'put "%1%" "%2%/"',
            'exit', '']))
        result = self.cli.run('/ini=nul', '/script=%s' % script, '/parameter', source, self.remote)
        self.assertEqual(result.returncode, 0, result)
        self.assertEqual((self.remote / 'scripted.txt').read_text(), 'scripted')

    def test_commands_from_input(self):
        source = self.write_local('piped.txt', 'piped')
        commands = '\n'.join([self.open_command(), 'put "%s" "%s/"' % (source, self.remote), 'exit', ''])
        result = self.cli.run('/ini=nul', input=commands)
        self.assertEqual(result.returncode, 0, result)
        # The commands are echoed after the prompt
        self.assertIn('winscp> put', result.stdout)
        self.assertTrue((self.remote / 'piped.txt').exists())


class SftpTest(CommonTests, CliTestCase):
    protocol = 'sftp'

    def test_stdout_stream(self):
        data = os.urandom(100000)
        (self.remote / 'stream.bin').write_bytes(data)
        result = self.session('get "%s/stream.bin" -' % self.remote, args=('/stdout',))
        self.assertEqual(result.stdout_bytes, data)
        self.assertIn('Authenticated.', result.stderr)

    def test_stdin_stream(self):
        data = os.urandom(100000)
        self.session('put - "%s/fromstdin.bin"' % self.remote, args=('/stdin',), input=data)
        self.assertEqual((self.remote / 'fromstdin.bin').read_bytes(), data)

    def test_neweronly_upload(self):
        old = self.write_local('n.txt', 'local')
        os.utime(old, (1000000000, 1000000000))
        (self.remote / 'n.txt').write_text('remote')
        self.session('put -neweronly "%s" "%s/"' % (old, self.remote))
        self.assertEqual((self.remote / 'n.txt').read_text(), 'remote')

    def test_resume_upload(self):
        data = os.urandom(2 * 1024 * 1024)
        source = self.write_local('resume.bin', data)
        (self.remote / 'resume.bin').write_bytes(data[:700000])
        self.session('put -resume "%s" "%s/"' % (source, self.remote))
        self.assertEqual(sha256(self.remote / 'resume.bin'), sha256(source))

    def test_resume_download(self):
        data = os.urandom(2 * 1024 * 1024)
        (self.remote / 'resume.bin').write_bytes(data)
        partial = self.write_local('resume.bin', data[:900000])
        self.session('get -resume "%s/resume.bin" "%s/"' % (self.remote, self.local))
        self.assertEqual(partial.read_bytes(), data)

    def test_symlink(self):
        (self.remote / 'target.txt').write_text('t')
        self.session('cd "%s"' % self.remote, 'ln target.txt link.txt')
        self.assertTrue((self.remote / 'link.txt').is_symlink())
        self.assertEqual(os.readlink(self.remote / 'link.txt'), 'target.txt')

    def test_stat(self):
        (self.remote / 'stat.txt').write_text('1234567')
        result = self.session('stat "%s/stat.txt"' % self.remote)
        self.assertIn('stat.txt', result.stdout)
        self.assertIn('7', result.stdout)

    def test_call(self):
        result = self.session('call echo openscp-call-$((6*7))')
        self.assertIn('openscp-call-42', result.stdout)


class ScpTest(CommonTests, CliTestCase):
    protocol = 'scp'

    def test_streams_not_supported(self):
        (self.remote / 'stream.bin').write_bytes(b'data')
        result = self.session('get "%s/stream.bin" -' % self.remote, args=('/stdout',), check=False)
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Operation not supported', result.stderr)

    def test_call(self):
        result = self.session('call echo openscp-call-$((6*7))')
        self.assertIn('openscp-call-42', result.stdout)


class KeepUpToDateTest(CliTestCase):
    protocol = 'sftp'

    def test_keepuptodate(self):
        (self.local / 'watched').mkdir()
        (self.remote / 'watched').mkdir()
        process = self.cli.popen(
            '/ini=nul', '/command', self.open_command(),
            'keepuptodate "%s/watched" "%s/watched"' % (self.local, self.remote), 'exit',
            stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        lines = []
        reader = threading.Thread(target=lambda: lines.extend(iter(process.stdout.readline, b'')))
        reader.start()
        try:
            deadline = time.monotonic() + 30
            while not any(b'Watching for changes in' in line for line in lines) and (time.monotonic() < deadline):
                time.sleep(0.05)
            (self.local / 'watched' / 'new.txt').write_text('new')
            (self.local / 'watched' / 'sub').mkdir()
            while time.monotonic() < deadline:
                if (self.remote / 'watched' / 'new.txt').exists() and (self.remote / 'watched' / 'sub').exists():
                    break
                time.sleep(0.1)
            # A file in the new subdirectory, which is watched since its creation
            (self.local / 'watched' / 'sub' / 'deep.txt').write_text('deep')
            while time.monotonic() < deadline:
                if (self.remote / 'watched' / 'sub' / 'deep.txt').exists():
                    break
                time.sleep(0.1)
            time.sleep(0.5)
        finally:
            process.send_signal(signal.SIGINT)
            process.wait(timeout=30)
            reader.join()
            process.stdout.close()
        output = b''.join(lines).decode(errors='replace')
        self.assertTrue((self.remote / 'watched' / 'new.txt').exists(), output)
        self.assertTrue((self.remote / 'watched' / 'sub' / 'deep.txt').exists(), output)
        # Ctrl+C stops watching and the script continues (with exit)
        self.assertIn('Terminated by user.', output)
        self.assertEqual(process.returncode, 0, output)


# A cancelled host key prompt abandons the key exchange (a coroutine of PuTTY)
# with an exception, and with it a packet (as upstream), which has no frame of
# its own for a suppression
ABANDONED_KEY_EXCHANGE = {'ASAN_OPTIONS': 'detect_leaks=0'}


class InteractiveTest(CliTestCase):
    """The console on a terminal."""
    protocol = 'sftp'

    def test_prompt(self):
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command() + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('cd "%s"\n' % self.remote)
        console.expect('winscp> ')
        console.send('pwd\n')
        console.expect(str(self.remote))
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)

    def test_unknown_host_key_accepted(self):
        ini = self.work / 'openscp.ini'
        console = self.cli.spawn_pty('/ini=%s' % ini)
        console.expect('winscp> ')
        console.send('open %s -privatekey="%s"\n' % (self.url(), key_file()))
        console.expect(re.escape(host_key_fingerprint().split()[-1]))
        # Accepts and caches the key
        console.expect(r'\(A\)ccept, Connect \(O\)nce, Ca\(n\)cel.*: ')
        console.send('a')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        self.assertIn('[SshHostKeys]', ini.read_text())
        # Known now
        result = self.cli.commands(
            'open %s -privatekey="%s"' % (self.url(), key_file()), 'exit', args=('/ini=%s' % ini,))
        self.assertEqual(result.returncode, 0, result)

    def test_unknown_host_key_escape(self):
        # (With /ini=nul, the key could not be cached, so there is no prompt)
        console = self.cli.spawn_pty('/ini=%s' % (self.work / 'openscp.ini'), env=ABANDONED_KEY_EXCHANGE)
        console.expect('winscp> ')
        console.send('open %s -privatekey="%s"\n' % (self.url(), key_file()))
        console.expect(r'\(A\)ccept.*: ')
        # Escape cancels
        console.send('\x1b')
        console.expect('Cancel')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 1, console.text)

    def test_unknown_host_key_in_batch(self):
        # No prompt for the commands of the command line
        console = self.cli.spawn_pty(
            '/ini=%s' % (self.work / 'openscp.ini'), '/command', 'open %s -privatekey="%s"' % (self.url(), key_file()),
            env=ABANDONED_KEY_EXCHANGE)
        # In batch mode, the prompt times out in 10 seconds (Cancel)
        console.expect(r'Ca\(n\)cel \(10 s\)')
        console.expect(r'Ca\(n\)cel \(8 s\)')
        console.send('n')
        console.expect("Host key wasn't verified")
        # The batch is aborted
        self.assertEqual(console.wait(), 1, console.text)

    def test_passphrase_prompt(self):
        protected = self.work / 'protected.ppk'
        result = self.cli.run('/keygen', key_file(), '/output=%s' % protected, '/changepassphrase=secret-phrase')
        self.assertEqual(result.returncode, 0, result)
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(privatekey=protected) + '\n')
        console.expect('Passphrase for key')
        console.send('secret-phrase\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        # Not echoed
        self.assertNotIn('secret-phrase', console.text)

    def test_interrupt_at_prompt(self):
        # Ends the console (as the end of the input), as winscp.com does
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send('\x03')
        self.assertEqual(console.wait(), 0, console.text)

    def test_interrupt_transfer(self):
        (self.remote / 'big.bin').write_bytes(os.urandom(64 * 1024 * 1024))
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command() + '\n')
        console.expect('winscp> ')
        # Slowly, not to finish before the interrupt
        console.send('get -speed=1024 "%s/big.bin" "%s/"\n' % (self.remote, self.local))
        # The progress of the transfer
        console.expect(r'big\.bin +\| +[0-9]+ KB')
        console.send('\x03')
        console.expect('Terminated by user')
        console.expect('winscp> ')
        console.send('exit\n')
        # Interactively, a cancelled command is not an error
        self.assertEqual(console.wait(), 0, console.text)
        self.assertLess((self.local / 'big.bin.filepart').stat().st_size if (self.local / 'big.bin.filepart').exists()
                        else 0, 64 * 1024 * 1024)


class ConfigurationTest(CliTestCase):
    protocol = 'sftp'

    def test_default_configuration_file(self):
        result = self.cli.commands(self.open_command(), 'exit')
        self.assertEqual(result.returncode, 0, result)
        text = self.cli.config_file.read_text()
        # The sections of WinSCP.ini
        self.assertIn('[Configuration\\Interface]', text)

    def test_stored_session(self):
        ini = self.work / 'sessions.ini'
        ini.write_text('\n'.join([
            '[Sessions\\Test%20site]',
            'HostName=%s' % SSHD.host,
            'PortNumber=%d' % SSHD.port,
            'UserName=%s' % SSHD.user,
            'PublicKeyFile=%s' % key_file(),
            'FSProtocol=2',
            '',
            '[SshHostKeys]',
            '']))
        result = self.cli.commands(
            'open "Test site" -hostkey="%s"' % host_key_fingerprint(), 'pwd', 'exit',
            args=('/ini=%s' % ini,))
        self.assertEqual(result.returncode, 0, result)

    def test_batchsettings(self):
        ini = self.work / 'sessions.ini'
        ini.write_text('[Sessions\\a%20one]\nHostName=one\n\n[Sessions\\b]\nHostName=two\n')
        result = self.cli.run('/ini=%s' % ini, '/batchsettings', 'a*', 'PortNumber=2222')
        self.assertEqual(result.returncode, 0, result)
        self.assertIn('a one', result.stdout)
        sections = ('\n' + ini.read_text()).split('\n[')
        session_a = next(section for section in sections if section.startswith('Sessions\\a%20one]'))
        session_b = next(section for section in sections if section.startswith('Sessions\\b]'))
        self.assertIn('\nPortNumber=2222\n', session_a)
        self.assertNotIn('\nPortNumber=', session_b)

    def test_rawtransfersettings(self):
        source = self.write_local('t.txt', 't')
        os.utime(source, (1000000000, 1000000000))
        result = self.cli.commands(
            self.open_command(), 'put "%s" "%s/"' % (source, self.remote), 'exit',
            args=('/ini=nul', '/rawtransfersettings', 'PreserveTime=0'))
        self.assertEqual(result.returncode, 0, result)
        self.assertGreater((self.remote / 't.txt').stat().st_mtime, 1000000000 + 1000)

    def test_help_and_info(self):
        result = self.cli.run('/help')
        self.assertEqual(result.returncode, 0, result)
        self.assertIn('OpenSCP, Version', result.stdout)
        self.assertIn('/keygen', result.stdout)
        result = self.cli.run('/info')
        self.assertEqual(result.returncode, 0, result)
        self.assertIn('SSH implementation: PuTTY', result.stdout)
        self.assertIn('TLS/SSL implementation: OpenSSL', result.stdout)


if __name__ == '__main__':
    unittest.main()
