"""L2 tests of FTP and FTPS against the test VM (milestone M4): the FileZilla
engine with vsftpd (LIST listings), ProFTPD and Pure-FTPd (MLSD); plain
FTP, explicit and implicit TLS; passive and active mode.

See vmtests for the VM (LINSCP_TEST_VM; skipped without it) and the
matrix of tests that every protocol passes.
"""

import calendar
import os
import time
import unittest

import vmtests
from vmtests import FileSystemFiles, TransferTests, VmTestCase, sha256


def setUpModule():
    vmtests.set_up_module('linscp-vsftpd', 'linscp-vsftpd-implicit', 'linscp-proftpd', 'linscp-pureftpd')


def tearDownModule():
    vmtests.tear_down_module()


class FtpTestCase(VmTestCase):
    protocol = 'ftp'
    port_setting = 'VSFTPD_PORT'
    tls = False

    def setUp(self):
        self.user = vmtests.setting('SSH_PASSWORD_USER')
        self.password = vmtests.VM.password
        self.port = int(vmtests.setting(self.port_setting))
        super().setUp()

    def remote_base(self):
        return vmtests.VM.user_home(self.user)

    def files(self):
        return FileSystemFiles('', self.user)

    def open_switches(self):
        return ['-certificate="%s"' % vmtests.VM.certificate_fingerprint()] if self.tls else []


class VsftpdTest(TransferTests, FtpTestCase):
    pass


class VsftpdExplicitTlsTest(TransferTests, FtpTestCase):
    protocol = 'ftpes'
    tls = True


class VsftpdImplicitTlsTest(TransferTests, FtpTestCase):
    protocol = 'ftps'
    port_setting = 'VSFTPD_IMPLICIT_PORT'
    tls = True


class ProftpdTest(TransferTests, FtpTestCase):
    port_setting = 'PROFTPD_PORT'


class ProftpdExplicitTlsTest(TransferTests, FtpTestCase):
    protocol = 'ftpes'
    port_setting = 'PROFTPD_PORT'
    tls = True


class PureFtpdTest(TransferTests, FtpTestCase):
    port_setting = 'PUREFTPD_PORT'


class PureFtpdExplicitTlsTest(TransferTests, FtpTestCase):
    protocol = 'ftpes'
    port_setting = 'PUREFTPD_PORT'
    tls = True


class FtpFeaturesTest(FtpTestCase):
    """What is specific to FTP, against each server."""

    SERVERS = ['VSFTPD_PORT', 'PROFTPD_PORT', 'PUREFTPD_PORT']

    def on_each_server(self, test):
        for port_setting in self.SERVERS:
            with self.subTest(port_setting):
                self.port = int(vmtests.setting(port_setting))
                test()

    def test_active_mode(self):
        data = os.urandom(3 * 1024 * 1024)
        source = self.write_local('active.bin', data)

        def test():
            back = self.local / ('back-%d.bin' % self.port)
            result = self.session(
                'put "%s" "%s/"' % (source, self.remote),
                'ls "%s"' % self.remote,
                'get "%s/active.bin" "%s"' % (self.remote, back),
                'rm "%s/active.bin"' % self.remote,
                extra=['-passive=0'])
            self.assertIn('active.bin', result.stdout)
            self.assertEqual(back.read_bytes(), data)

        self.on_each_server(test)

    def test_active_mode_with_tls(self):
        source = self.write_local('active.txt', b'active with TLS')
        self.protocol = 'ftpes'
        self.tls = True

        def test():
            self.session('put "%s" "%s/"' % (source, self.remote), extra=['-passive=0'])
            self.assertEqual(self.files().sha256('%s/active.txt' % self.remote), sha256(b'active with TLS'))
            self.files().remove('%s/active.txt' % self.remote)

        self.on_each_server(test)

    def test_listing_without_mlsd(self):
        # LIST also on the servers that support MLSD
        self.files().write('%s/listed.bin' % self.remote, os.urandom(4321), mtime=calendar.timegm((2019, 7, 1, 8, 9, 10)))

        def test():
            result = self.session('ls "%s"' % self.remote, extra=['-rawsettings', 'FtpUseMlsd=1'])
            line = next(line for line in result.stdout.splitlines() if line.endswith(' listed.bin'))
            self.assertIn(' 4321 ', line)
            self.assertIn('Jul 1', ' '.join(line.split()))
            self.assertIn('2019', line)

        self.on_each_server(test)

    def test_preserve_time(self):
        source = self.write_local('timed.txt', b'timed')
        mtime = calendar.timegm((2021, 3, 4, 5, 6, 7))
        os.utime(source, (mtime, mtime))
        remote_mtime = calendar.timegm((2020, 1, 2, 3, 4, 5))

        def test():
            target = '%s/timed.txt' % self.remote
            self.session('put "%s" "%s/"' % (source, self.remote))
            self.assertEqual(int(self.files().stat(target, '%Y')), mtime)
            self.files().write(target, b'remote', mtime=remote_mtime)
            back = self.local / ('timed-%d.txt' % self.port)
            self.session('get "%s" "%s"' % (target, back))
            self.assertEqual(int(back.stat().st_mtime), remote_mtime)
            self.files().remove(target)

        self.on_each_server(test)

    def test_ascii_mode(self):
        # Lines of all lengths, so that line ends meet the ends of buffers
        lines = b''.join(b'x' * (index % 97) + b'\n' for index in range(20000))
        # A CRLF line end stays one (servers may drop a CR alone)
        source = self.write_local('text.txt', lines + b'crlf\r\nlast')
        uploaded = lines + b'crlf\nlast'
        # A CR alone stays in a download
        downloaded = lines + b'cr\ralone\nlast\n'

        def test():
            self.session('put -transfer=ascii "%s" "%s/"' % (source, self.remote))
            # CRLF on the wire, Unix line endings on both sides
            self.assertEqual(self.files().sha256('%s/text.txt' % self.remote), sha256(uploaded))
            self.files().write('%s/text.txt' % self.remote, downloaded)
            back = self.local / ('text-%d.txt' % self.port)
            self.session('get -transfer=ascii "%s/text.txt" "%s"' % (self.remote, back))
            self.assertEqual(back.read_bytes(), downloaded)
            self.files().remove('%s/text.txt' % self.remote)

        self.on_each_server(test)

    def test_wrong_certificate(self):
        self.protocol = 'ftpes'

        def test():
            result = self.session(
                'pwd', check=False,
                extra=['-certificate="%s"' % ':'.join(['00'] * 32)])
            self.assertEqual(result.returncode, 1, result)
            self.assertIn('certificate', result.output)
            # Without a certificate given, an unknown certificate is not
            # accepted in batch mode
            result = self.session('pwd', check=False)
            self.assertEqual(result.returncode, 1, result)

        self.on_each_server(test)

    def test_tls_session_info(self):
        self.protocol = 'ftpes'
        self.tls = True
        self.session('ls', args=('/loglevel=1', '/log=%s' % (self.work / 'session.log')))
        log = (self.work / 'session.log').read_text(errors='replace')
        self.assertIn('TLS connection established', log)
        self.assertIn('Session ID reused', log, 'vsftpd requires the reuse of the TLS session for data')


class FtpEngineTest(FtpTestCase):
    """The FileZilla engine's thread, sockets, name resolution and timers."""

    def test_connection_refused(self):
        self.port = 2999
        result = self.cli.commands(self.open_command(), 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Connection refused', result.output)

    def test_ipv4_name_resolution(self):
        # With IPv4 only, the engine resolves names asynchronously
        # (WSAAsyncGetHostByName); nothing listens on the port
        result = self.cli.commands(
            'open ftp://user:password@localhost:1/ -rawsettings AddressFamily=1', 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertIn('Connection refused', result.output)
        result = self.cli.commands(
            'open ftp://user:password@linscp.invalid/ -rawsettings AddressFamily=1', 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)
        self.assertNotIn('Connection refused', result.output)

    def test_interrupt_transfer(self):
        self.files().write('%s/big.bin' % self.remote, os.urandom(64 * 1024 * 1024))
        console = self.cli.spawn_pty('/ini=nul')
        console.expect('winscp> ')
        console.send(self.open_command() + '\n')
        console.expect('winscp> ')
        # Slowly, not to finish before the interrupt
        console.send('get -speed=1024 "%s/big.bin" "%s/"\n' % (self.remote, self.local))
        console.expect(r'big\.bin +\| +[0-9]+ KB')
        console.send('\x03')
        console.expect('winscp> ')
        # The session still works
        console.send('ls "%s"\n' % self.remote)
        console.expect('big\\.bin')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        partial = [path for path in self.local.iterdir() if path.name.startswith('big.bin')]
        self.assertLess(sum(path.stat().st_size for path in partial), 64 * 1024 * 1024)

    def test_keepalive(self):
        log = self.work / 'session.log'
        console = self.cli.spawn_pty('/ini=nul', '/loglevel=1', '/log=%s' % log)
        console.expect('winscp> ')
        console.send(self.open_command(extra=['-rawsettings', 'FtpPingInterval=1']) + '\n')
        console.expect('winscp> ')
        time.sleep(4)
        console.send('pwd\n')
        console.expect('winscp> ')
        console.send('exit\n')
        self.assertEqual(console.wait(), 0, console.text)
        self.assertIn('Sending dummy command to keep session alive.', log.read_text(errors='replace'))


if __name__ == '__main__':
    unittest.main()
