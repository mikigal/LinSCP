"""End-to-end tests of the GUI against the test VM (milestones M5 and M6): the
authentication window with password and keyboard-interactive prompts, which
the local sshd cannot do without root, transfers over a real network, and
the properties of S3 objects (their ACL and tags).

Skipped without the VM (OPENSCP_TEST_VM, see vmtests) or the GUI
(OPENSCP_GUI).
"""

import os
import unittest
import uuid

import vmtests
from guitests import GuiTestCase
from vmtests import FileSystemFiles, S3Objects, setting

W = 'ScpCommanderForm/'
P = 'PropertiesDialog/'


def setUpModule():
    if not os.environ.get('OPENSCP_GUI'):
        raise unittest.SkipTest('OPENSCP_GUI is not set')
    vmtests.set_up_module('openscp-sshd')


def tearDownModule():
    vmtests.tear_down_module()


class GuiVmTest(GuiTestCase):
    def setUp(self):
        self.user = setting('SSH_PASSWORD_USER')
        # Absolute paths on the VM
        self.files = FileSystemFiles('', self.user)
        self.remote = '%s/test-%s' % (vmtests.VM.user_home(self.user), uuid.uuid4().hex[:8])
        self.files.make_dir(self.remote)
        self.addCleanup(self.files.remove, self.remote)

    def site_ini(self, user, remote_directory=None):
        lines = ['[Sessions\\vm]', 'HostName=%s' % vmtests.VM.host, 'PortNumber=%s' % setting('SSH_PORT'),
                 'UserName=%s' % user]
        if remote_directory is not None:
            lines.append('RemoteDirectory=%s' % remote_directory)
        return '\n'.join(lines) + '\n'

    def connect(self, gui, accept_host_key=True):
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 'vm')
        gui.click('LoginDialog/LoginButton')
        if accept_host_key:
            gui.wait_window('MessageForm', timeout=60)
            texts = gui.get('MessageForm/Message')['text']
            self.assertIn(vmtests.VM.host_key_fingerprint().split()[-1], texts)
            gui.click('MessageForm/Yes')
        # The authentication banner of the test server
        gui.wait_for(lambda: (gui.window('AuthenticateForm') is not None) and
                     gui.get('AuthenticateForm/BannerPanel')['visible'], 60, 'the banner')
        self.assertEqual(gui.get('AuthenticateForm/BannerMemo')['text'].strip(), 'OpenSCP test server')
        gui.click('AuthenticateForm/BannerCloseButton')

    def password_prompt(self, gui):
        gui.wait_for(lambda: (gui.window('AuthenticateForm') is not None) and
                     gui.get('AuthenticateForm/PasswordPanel')['visible'], 60, 'the password prompt')
        return gui.get('AuthenticateForm/PromptLabel1')['text']

    def test_password(self):
        self.files.write('%s/existing.txt' % self.remote, b'on the VM')
        gui = self.start_gui(ini=self.site_ini(self.user, self.remote))
        self.connect(gui)

        # A wrong password first: the prompt again
        self.assertRegex(self.password_prompt(gui), '[Pp]assword')
        self.assertTrue(gui.get('AuthenticateForm/PromptEdit1')['password'])
        gui.set('AuthenticateForm/PromptEdit1', 'wrong')
        gui.click('AuthenticateForm/PasswordOKButton')
        gui.wait_for(lambda: any('Access denied' in row['cells'][0] for row in gui.get('AuthenticateForm/LogView')['rows']),
                     60, 'the denied password')
        self.password_prompt(gui)
        self.screenshot('password')
        gui.set('AuthenticateForm/PromptEdit1', setting('SSH_PASSWORD'))
        gui.click('AuthenticateForm/PasswordOKButton')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        gui.wait_idle()
        self.assertIsNone(gui.window('AuthenticateForm'))
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'existing.txt'])

        # Upload and download over the network
        local = self.work_dir('local')
        data = os.urandom(2 * 1024 * 1024)
        (local / 'big.bin').write_bytes(data)
        gui.focus(W + 'LocalDirView')
        gui.call('wait-idle')
        gui.set(W + 'LocalPathComboBox', str(local))
        gui.key('Return', W + 'LocalPathComboBox')
        gui.wait_for(lambda: 'big.bin' in gui.names(W + 'LocalDirView'), what='the local directory')
        gui.select(W + 'LocalDirView', 'big.bin')
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_window('CopyDialog')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual(self.files.sha256('%s/big.bin' % self.remote), vmtests.sha256(data))

        gui.focus(W + 'RemoteDirView')
        gui.select(W + 'RemoteDirView', 'existing.txt')
        gui.wait_idle()
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_window('CopyDialog')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((local / 'existing.txt').read_bytes(), b'on the VM')
        self.assertEqual(gui.quit(), 0)

    def test_keyboard_interactive(self):
        gui = self.start_gui(ini=self.site_ini(setting('SSH_KBD_USER')))
        self.connect(gui)
        self.assertRegex(self.password_prompt(gui), '[Pp]assword')
        gui.set('AuthenticateForm/PromptEdit1', setting('SSH_PASSWORD'))
        gui.click('AuthenticateForm/PasswordOKButton')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        self.assertEqual(gui.get(W + 'RemotePathComboBox')['text'], vmtests.VM.user_home(setting('SSH_KBD_USER')))
        self.assertEqual(gui.quit(), 0)

    def test_cancelled_prompt(self):
        gui = self.start_gui(ini=self.site_ini(self.user))
        self.connect(gui)
        self.password_prompt(gui)
        gui.click('AuthenticateForm/PasswordCancelButton')
        # Cancelling the authentication ends the connection, silently
        gui.wait_for(lambda: gui.window('LoginDialog') is not None, 60, 'the site manager')
        self.assertIsNone(gui.window('MessageForm'))
        gui.click('LoginDialog/CloseButton')
        gui.wait_no_window('LoginDialog')
        self.assertTrue(gui.get(W + 'RemoteNoSessionPage')['visible'])
        self.assertEqual(gui.quit(), 0)


class GuiS3Test(GuiTestCase):
    @classmethod
    def setUpClass(cls):
        if not vmtests.VM.services_active('openscp-s3'):
            raise unittest.SkipTest('the S3 server of the test VM is not active')
        super().setUpClass()

    def setUp(self):
        self.access_key, self.secret_key = vmtests.VM.output('cat /etc/openscp/s3-credentials').split()
        self.bucket = 'gui-%s' % uuid.uuid4().hex[:8]
        self.objects = S3Objects()
        self.objects.make_dir('/' + self.bucket)
        self.addCleanup(self.objects.remove, '/' + self.bucket)

    def connect(self):
        ini = '\n'.join([
            '[Sessions\\s3]', 'HostName=%s' % vmtests.VM.host, 'PortNumber=%s' % setting('S3_PORT'),
            'FSProtocol=7', 'UserName=%s' % self.access_key, 'S3UrlStyle=1',
            'RemoteDirectory=/%s' % self.bucket]) + '\n'
        gui = self.start_gui(ini=ini)
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 's3')
        gui.click('LoginDialog/LoginButton')
        gui.wait_for(lambda: (gui.window('AuthenticateForm') is not None) and
                     gui.get('AuthenticateForm/PasswordPanel')['visible'], 60, 'the secret key prompt')
        gui.set('AuthenticateForm/PromptEdit1', self.secret_key)
        gui.click('AuthenticateForm/PasswordOKButton')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        gui.wait_idle()
        return gui

    def open_properties(self, gui, name):
        gui.focus(W + 'RemoteDirView')
        gui.select(W + 'RemoteDirView', name)
        gui.wait_idle()
        gui.trigger(W + 'CurrentPropertiesAction')
        gui.wait_window('PropertiesDialog')

    def submit_properties(self, gui):
        gui.click(P + 'OkButton')
        gui.wait_no_window('PropertiesDialog')
        gui.wait_idle()

    def test_acl_and_tags(self):
        self.objects.write('/%s/file.txt' % self.bucket, b'data')
        gui = self.connect()
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'file.txt'])

        # Everyone may read the object
        self.open_properties(gui, 'file.txt')
        self.assertEqual(gui.get(P + 'RightsLabel')['text'], 'ACL:')
        self.assertEqual(gui.get(P + 'OthersButton')['text'], '&Everyone')
        self.assertFalse(gui.get(P + 'OthersWriteCheck')['visible'])
        self.assertFalse(gui.get(P + 'OctalEdit')['visible'])
        self.assertIn('Tags', gui.get(P + 'PageControl')['items'])
        self.assertFalse(gui.get(P + 'OthersReadCheck')['checked'])
        gui.click(P + 'OthersReadCheck')
        self.screenshot('acl')
        self.submit_properties(gui)
        acl = vmtests.VM.output('openscp-s3 acl %s file.txt' % self.bucket).splitlines()
        self.assertIn('http://acs.amazonaws.com/groups/global/AllUsers READ', acl)

        # A tag
        self.open_properties(gui, 'file.txt')
        self.assertTrue(gui.get(P + 'OthersReadCheck')['checked'])
        gui.set(P + 'PageControl', 'Tags')
        self.assertEqual(gui.rows(P + 'TagsView'), [])
        gui.click(P + 'AddTagButton')
        gui.wait_window('TagDialog')
        gui.set('TagDialog/KeyEdit', 'project')
        gui.set('TagDialog/ValueEdit', 'openscp')
        gui.click('TagDialog/OKButton')
        gui.wait_no_window('TagDialog')
        self.assertEqual(gui.rows(P + 'TagsView'), [['project', 'openscp']])
        self.submit_properties(gui)
        tags = vmtests.VM.output('openscp-s3 tags %s file.txt' % self.bucket).splitlines()
        self.assertEqual(tags, ['project=openscp'])
        self.assertEqual(gui.quit(), 0)


if __name__ == '__main__':
    unittest.main()
