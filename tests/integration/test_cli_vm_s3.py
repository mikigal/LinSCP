"""L2 tests of S3 against the test VM (milestone M4): libs3 with moto's S3
server (signatures verified), over HTTP and over HTTPS through nginx; each
test has a bucket of its own.

See vmtests for the VM (OPENSCP_TEST_VM; skipped without it) and the
matrix of tests that every protocol passes.
"""

import unittest

import vmtests
from vmtests import S3Objects, TransferTests, VmTestCase, sha256

CREDENTIALS = None


def setUpModule():
    global CREDENTIALS
    vmtests.set_up_module('openscp-s3', 'nginx')
    CREDENTIALS = vmtests.VM.output('cat /etc/openscp/s3-credentials').split()


def tearDownModule():
    vmtests.tear_down_module()


class S3TestCase(VmTestCase):
    protocol = 's3plain'
    port_setting = 'S3_PORT'
    tls = False
    password_prompt = 'Secret access key: '

    def setUp(self):
        self.user, self.password = CREDENTIALS
        self.port = int(vmtests.setting(self.port_setting))
        super().setUp()

    def remote_base(self):
        # (A bucket per test)
        return ''

    def files(self):
        return S3Objects()

    def open_switches(self):
        switches = ['-rawsettings', 'S3UrlStyle=1']
        if self.tls:
            switches.append('-certificate="%s"' % vmtests.VM.certificate_fingerprint())
        return switches


class S3TransferTests(TransferTests):
    supports_permissions = False
    supports_resume_upload = False
    supports_resume_download = False

    def test_buckets(self):
        bucket = 'openscp-%s' % self.remote.strip('/')[5:]
        self.addCleanup(self.files().remove, '/' + bucket)
        result = self.session('mkdir /%s' % bucket, 'ls /')
        self.assertIn(bucket, result.stdout)
        self.assertIn(self.remote.strip('/'), result.stdout)
        self.session('rmdir /%s' % bucket)
        self.assertNotIn(bucket, self.session('ls /').stdout)

    def test_multipart_upload(self):
        # Larger than the part size (5 MB), in parts
        data = bytes(range(256)) * (12 * 1024 * 1024 // 256 + 3)
        source = self.write_local('multipart.bin', data)
        self.session(
            'put "%s" "%s/"' % (source, self.remote), args=('/loglevel=1', '/log=%s' % (self.work / 's3.log')))
        self.assertEqual(self.files().sha256('%s/multipart.bin' % self.remote), sha256(data))
        self.assertIn('multipart', (self.work / 's3.log').read_text(errors='replace').lower())


class S3Test(S3TransferTests, S3TestCase):
    pass


class S3TlsTest(S3TransferTests, S3TestCase):
    protocol = 's3'
    port_setting = 'S3S_PORT'
    tls = True


if __name__ == '__main__':
    unittest.main()
