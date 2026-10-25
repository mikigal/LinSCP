"""L2 tests of WebDAV against the test VM (milestone M4): neon with Apache
mod_dav and nginx, over HTTP and HTTPS.

See vmtests for the VM (LINSCP_TEST_VM; skipped without it) and the
matrix of tests that every protocol passes.
"""

import calendar
import os
import unittest

import vmtests
from vmtests import FileSystemFiles, TransferTests, VmTestCase


def setUpModule():
    vmtests.set_up_module('apache2', 'nginx')


def tearDownModule():
    vmtests.tear_down_module()


class WebDavTestCase(VmTestCase):
    protocol = 'dav'
    server = 'apache'
    port_setting = 'APACHE_DAV_PORT'
    tls = False

    def setUp(self):
        self.user = vmtests.setting('SSH_PASSWORD_USER')
        self.password = vmtests.VM.password
        self.port = int(vmtests.setting(self.port_setting))
        super().setUp()

    def remote_base(self):
        return ''

    def files(self):
        return FileSystemFiles('/srv/linscp/dav/%s' % self.server, 'www-data')

    def open_switches(self):
        return ['-certificate="%s"' % vmtests.VM.certificate_fingerprint()] if self.tls else []


class WebDavTransferTests(TransferTests):
    supports_permissions = False
    supports_resume_upload = False
    supports_resume_download = False

    def test_timestamps(self):
        # The server's modification time comes to a download; WebDAV cannot
        # set the time of an upload
        mtime = calendar.timegm((2020, 1, 2, 3, 4, 5))
        self.files().write('%s/timed.txt' % self.remote, b'timed', mtime=mtime)
        self.session('get "%s/timed.txt" "%s/"' % (self.remote, self.local))
        self.assertEqual(int((self.local / 'timed.txt').stat().st_mtime), mtime)


class ApacheTest(WebDavTransferTests, WebDavTestCase):
    pass


class ApacheTlsTest(WebDavTransferTests, WebDavTestCase):
    protocol = 'davs'
    port_setting = 'APACHE_DAVS_PORT'
    tls = True

    def test_wrong_certificate(self):
        result = self.session('pwd', check=False, extra=['-certificate="%s"' % ':'.join(['00'] * 32)])
        self.assertEqual(result.returncode, 1, result)
        # Without a certificate given, an unknown certificate is not
        # accepted in batch mode
        result = self.cli.commands('open %s' % self.url(), 'exit', args=('/ini=nul',))
        self.assertEqual(result.returncode, 1, result)


class NginxTest(WebDavTransferTests, WebDavTestCase):
    server = 'nginx'
    port_setting = 'NGINX_DAV_PORT'


class NginxTlsTest(WebDavTransferTests, WebDavTestCase):
    protocol = 'davs'
    server = 'nginx'
    port_setting = 'NGINX_DAVS_PORT'
    tls = True


if __name__ == '__main__':
    unittest.main()
