"""L2 tests of SFTP and SCP against the test VM (milestone M3): what the
local sshd cannot do without root (password, keyboard-interactive and
password change authentication, chroot), and a real network.

See vmtests for the VM (LINSCP_TEST_VM; skipped without it) and the
matrix of tests that every protocol passes.
"""

import shlex
import tempfile
import unittest
from pathlib import Path

import vmtests
from linscp_cli import LinScpCli
from vmtests import FileSystemFiles, TransferTests, VmTestCase, setting


def setUpModule():
    vmtests.set_up_module('linscp-sshd')
    result = LinScpCli(vmtests.CLI, vmtests.STATE / 'keygen').run(
        '/keygen', vmtests.VM.client_key(), '/output=%s' % client_ppk())
    if result.returncode != 0:
        raise RuntimeError('cannot convert the client key: %r' % result)


def tearDownModule():
    vmtests.tear_down_module()


def client_ppk():
    return vmtests.STATE / 'client.ppk'


class SshTestCase(VmTestCase):
    protocol = 'sftp'
    # The test user, SSH_PASSWORD_USER unless a test sets another one
    user_setting = 'SSH_PASSWORD_USER'

    def setUp(self):
        self.user = setting(self.user_setting)
        self.password = vmtests.VM.password
        self.port = vmtests.VM.ssh_port
        super().setUp()

    def remote_base(self):
        return vmtests.VM.user_home(self.user)

    def files(self):
        return FileSystemFiles('', self.user)

    def open_switches(self):
        return ['-hostkey="%s"' % vmtests.VM.host_key_fingerprint()]


class SshTransferTests(TransferTests):
    def test_password_in_url(self):
        result = self.session('pwd')
        self.assertIn('Authenticating with pre-entered password.', result.stdout)
        self.assertIn(vmtests.VM.user_home(self.user), result.stdout)

    def test_password_switch(self):
        result = self.session('pwd', password=False, extra=['-password=%s' % self.password])
        self.assertIn('Authenticated.', result.stdout)

    def test_permission_denied(self):
        result = self.session('put "%s" /root/' % (self.work / 'cli'), check=False)
        self.assertEqual(result.returncode, 1, result)


class SftpPasswordTest(SshTransferTests, SshTestCase):
    protocol = 'sftp'


class ScpPasswordTest(SshTransferTests, SshTestCase):
    protocol = 'scp'
    supports_resume_upload = False
    supports_resume_download = False


class KeyboardInteractiveTest(SshTestCase):
    user_setting = 'SSH_KBD_USER'

    def test_pre_entered_password(self):
        # The password answers the prompt of PAM
        result = self.session('pwd')
        self.assertIn('Authenticated.', result.stdout)

    def test_prompt(self):
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect('[Pp]assword: ')
        console.send(self.password + '\n')
        console.expect('Session started')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)


class PasswordChangeTest(SshTestCase):
    user_setting = 'SSH_EXPIRED_USER'
    NEW_PASSWORD = 'Changed-pass-2'

    def setUp(self):
        super().setUp()
        self.addCleanup(self.expire)

    def expire(self):
        vmtests.VM.run('echo %s:%s | chpasswd && chage -d 0 %s' % (self.user, self.password, self.user))

    def test_change(self):
        # Interactively: after a pre-entered password, scripting asks for no
        # other credentials (as upstream)
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command(password=False) + '\n')
        console.expect('Password: ')
        console.send(self.password + '\n')
        # PAM asks for the current password again and a new one
        console.expect('You are required to change your password')
        console.expect('Current password: ')
        console.send(self.password + '\n')
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
            self.open_command(password=self.NEW_PASSWORD), 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 0, result)

    def test_batch_fails(self):
        result = self.session(check=False)
        self.assertEqual(result.returncode, 1, result)


class TwoFactorTest(SshTestCase):
    user_setting = 'SSH_TWOFACTOR_USER'

    def test_key_and_password(self):
        result = self.session('pwd', extra=['-privatekey="%s"' % client_ppk()])
        self.assertIn('Authenticated.', result.stdout)

    def test_key_only_fails(self):
        result = self.session('pwd', check=False, password=False, extra=['-privatekey="%s"' % client_ppk()])
        self.assertEqual(result.returncode, 1, result)


class ChrootTest(SshTestCase):
    user_setting = 'SSH_CHROOT_USER'

    def setUp(self):
        # (The home is not reachable in the chroot, the test has no remote
        # directory of its own)
        self.user = setting(self.user_setting)
        self.password = vmtests.VM.password
        self.port = vmtests.VM.ssh_port
        self.work = Path(tempfile.mkdtemp(prefix='vm-', dir=vmtests.STATE))
        self.local = self.work / 'local'
        self.local.mkdir()
        self.cli = LinScpCli(vmtests.CLI, self.work / 'cli')
        self.addCleanup(vmtests.VM.run, 'rm -rf /srv/linscp/chroot/upload/*')

    def test_chroot(self):
        source = self.write_local('jailed.txt', b'jailed')
        result = self.session('pwd', 'put "%s" /upload/' % source)
        self.assertIn('/', result.stdout.splitlines())
        self.assertEqual(vmtests.VM.output('cat /srv/linscp/chroot/upload/jailed.txt'), 'jailed')

    def test_no_shell(self):
        result = self.session('call echo hello', check=False)
        self.assertEqual(result.returncode, 1, result)


class CopyIdTest(SshTestCase):
    user_setting = 'SSH_COPYID_USER'

    def setUp(self):
        super().setUp()
        self.ssh_dir = '%s/.ssh' % vmtests.VM.user_home(self.user)
        vmtests.VM.run('rm -rf %s' % shlex.quote(self.ssh_dir))
        self.addCleanup(vmtests.VM.run, 'rm -rf %s' % shlex.quote(self.ssh_dir))

    def test_copyid(self):
        result = self.cli.run(
            '/ini=nul', '/copyid', '/identity=%s' % client_ppk(), self.url(),
            '-hostkey=%s' % vmtests.VM.host_key_fingerprint())
        self.assertEqual(result.returncode, 0, result)
        public_key = Path('%s.pub' % vmtests.VM.client_key()).read_text().split()[1]
        self.assertIn(public_key, vmtests.VM.output('cat %s/authorized_keys' % shlex.quote(self.ssh_dir)))
        self.assertEqual(vmtests.VM.output('stat -c %%a %s' % shlex.quote(self.ssh_dir)).strip(), '700')
        # The key works now
        result = self.session('pwd', password=False, extra=['-privatekey="%s"' % client_ppk()])
        self.assertIn('Authenticated.', result.stdout)


if __name__ == '__main__':
    unittest.main()
