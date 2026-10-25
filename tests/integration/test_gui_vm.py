"""End-to-end tests of the GUI against the test VM (milestone M5): the
authentication window with password and keyboard-interactive prompts, which
the local sshd cannot do without root, and transfers over a real network.

Skipped without the VM (OPENSCP_TEST_VM, see vmtests) or the GUI
(OPENSCP_GUI).
"""

import os
import unittest
import uuid

import vmtests
from guitests import GuiTestCase
from vmtests import FileSystemFiles, setting

W = 'ScpCommanderForm/'


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


if __name__ == '__main__':
    unittest.main()
