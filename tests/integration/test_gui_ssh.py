"""End-to-end tests of the GUI with SFTP against the local test sshd
(milestone M5): connecting from the site manager, browsing, transfers and
file operations.

The local sshd runs as the current user, so the remote side is the local
file system: the results are checked on the disk directly. The binaries are
taken from OPENSCP_GUI and OPENSCP_CLI (which converts the client key).
"""

import hashlib
import os
import shutil
import signal
import stat
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

from guitests import GuiTestCase
from local_sshd import LocalSshd
from openscp_cli import OpenScpCli

CLI = os.environ.get('OPENSCP_CLI')
SSHD = None
STATE = None
W = 'ScpCommanderForm/'
P = 'PropertiesDialog/'
C = 'CopyDialog/'
Q = W + 'QueueView3'


def setUpModule():
    global SSHD, STATE
    if not CLI or not os.environ.get('OPENSCP_GUI'):
        raise unittest.SkipTest('OPENSCP_CLI or OPENSCP_GUI is not set')
    STATE = Path(tempfile.mkdtemp(prefix='openscp-l4-'))
    SSHD = LocalSshd(STATE / 'sshd').start()
    result = OpenScpCli(CLI, STATE / 'keygen').run('/keygen', SSHD.client_key, '/output=%s' % key_file())
    if result.returncode != 0:
        raise RuntimeError('cannot convert the client key: %r' % result)


def tearDownModule():
    if SSHD is not None:
        SSHD.stop()
    if STATE is not None:
        shutil.rmtree(STATE, ignore_errors=True)


def key_file():
    return STATE / 'client.ppk'


def site_ini(name='local', port=None, remote=None, local=None):
    lines = ['[Sessions\\%s]' % name, 'HostName=127.0.0.1', 'PortNumber=%d' % (port or SSHD.port),
             'UserName=%s' % SSHD.user, 'PublicKeyFile=%s' % key_file()]
    if remote is not None:
        lines.append('RemoteDirectory=%s' % remote)
    if local is not None:
        lines.append('LocalDirectory=%s' % local)
    return '\n'.join(lines) + '\n'


class GuiSshTestCase(GuiTestCase):
    """The helpers of the tests with the local sshd (also of other modules)."""

    def login(self, gui, site, accept_host_key=True):
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', site)
        gui.click('LoginDialog/LoginButton')
        if accept_host_key:
            gui.wait_window('MessageForm')
            self.assertIn('unknown server', gui.get('MessageForm/MainMessage')['text'])
            gui.click('MessageForm/Yes')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        gui.wait_idle()

    def select(self, gui, side, *names):
        """Selects files in a panel and makes it the current panel."""
        view = W + side + 'DirView'
        gui.focus(view)
        gui.select(view, *names)
        gui.wait_idle()

    def confirm_copy(self, gui, expected_label):
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/DirectoryLabel')['text'], expected_label)
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()


class GuiSshTest(GuiSshTestCase):
    def test_new_site_with_agent(self):
        socket = self.session.dir / 'agent.sock'
        agent = subprocess.run(['ssh-agent', '-a', str(socket), '-s'], capture_output=True, text=True, check=True)
        pid = int(agent.stdout.split('SSH_AGENT_PID=')[1].split(';')[0])
        self.addCleanup(os.kill, pid, signal.SIGTERM)
        env = {'SSH_AUTH_SOCK': str(socket)}
        subprocess.run(['ssh-add', str(SSHD.client_key)], env=dict(os.environ, **env), capture_output=True, check=True)

        gui = self.start_gui(env=env)
        gui.wait_window('LoginDialog')
        self.assertEqual(gui.names('LoginDialog/SessionTree'), ['New Site'])
        gui.set('LoginDialog/HostNameEdit', '127.0.0.1')
        gui.set('LoginDialog/PortNumberEdit', SSHD.port)
        gui.set('LoginDialog/UserNameEdit', SSHD.user)
        self.screenshot('login')
        gui.click('LoginDialog/SaveButton')
        gui.wait_window('SaveSessionDialog')
        self.assertEqual(gui.get('SaveSessionDialog/SessionNameEdit')['text'], '%s@127.0.0.1' % SSHD.user)
        gui.set('SaveSessionDialog/SessionNameEdit', 'agent site')
        gui.click('SaveSessionDialog/OKButton')
        gui.wait_no_window('SaveSessionDialog')
        gui.wait_for(lambda: 'agent site' in gui.names('LoginDialog/SessionTree'), what='the saved site')
        ini = gui.config_file.read_text()
        self.assertIn('[Sessions\\agent%20site]', ini)
        self.assertIn('PortNumber=%d' % SSHD.port, ini)

        self.login(gui, 'agent site')
        self.assertIn('agent site \u2013 OpenSCP', gui.window('ScpCommanderForm')['title'])
        self.assertEqual(gui.get(W + 'SessionStatusLabel')['text'], 'SFTP-3')

        # Into a directory by its path, and back
        remote = self.work_dir('remote')
        (remote / 'file.txt').write_text('content')
        combo = W + 'RemotePathComboBox'
        gui.set(combo, str(remote))
        gui.key('Return', combo)
        gui.wait_for(lambda: gui.get(combo)['text'] == str(remote), what='the remote directory')
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'file.txt'])
        gui.trigger(W + 'RemoteBackAction')
        gui.wait_for(lambda: gui.get(combo)['text'] != str(remote), what='going back')

        # A disconnected session keeps its tab; the host key is cached: no
        # prompt the next time
        gui.trigger(W + 'DisconnectSessionAction')
        gui.wait_for(lambda: gui.get(W + 'RemoteNoSessionPage')['visible'], what='the disconnected panel')
        self.assertTrue(gui.get(W + 'ReconnectItem')['visible'])
        self.assertFalse(gui.get(W + 'RemoteNewSessionButton')['visible'])
        self.assertEqual(gui.get(W + 'SessionsPageControl')['items'], ['agent site', ''])
        self.assertIn('agent site \u2013 OpenSCP', gui.window('ScpCommanderForm')['title'])
        gui.click(W + 'ReconnectItem')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        self.assertIsNone(gui.window('MessageForm'))

        # Closing the last tab shows the site manager, with the last
        # session's site
        gui.trigger(W + 'CloseTabAction')
        gui.wait_window('LoginDialog')
        self.assertEqual(gui.window('ScpCommanderForm')['title'], 'OpenSCP')
        self.assertEqual(gui.get(W + 'SessionsPageControl')['items'], [''])
        selected = [row['cells'][0] for row in gui.get('LoginDialog/SessionTree')['rows'] if row['current']]
        self.assertEqual(selected, ['agent site'])
        gui.click('LoginDialog/LoginButton')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        self.assertIsNone(gui.window('MessageForm'))
        self.assertEqual(gui.quit(), 0)

    def test_file_operations(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'sub').mkdir()
        (local / 'sub' / 'nested.txt').write_text('nested')
        (local / 'upload.txt').write_bytes(os.urandom(300 * 1024))
        (remote / 'download.txt').write_text('remote content')

        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')
        self.assertEqual(gui.get(W + 'RemotePathComboBox')['text'], str(remote))
        self.assertEqual(gui.get(W + 'LocalPathComboBox')['text'], str(local))
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'download.txt'])
        self.assertEqual(gui.names(W + 'LocalDirView'), ['..', 'sub', 'upload.txt'])
        self.assertEqual(gui.get(W + 'LocalStatusBar')['text'], '0 B of 300 KB in 0 of 2')

        # Upload of a file and a directory
        self.select(gui, 'Local', 'upload.txt', 'sub')
        gui.trigger(W + 'CurrentCopyAction')
        self.confirm_copy(gui, 'Upload 2 files to remote directory:')
        self.assertEqual((remote / 'upload.txt').read_bytes(), (local / 'upload.txt').read_bytes())
        self.assertEqual((remote / 'sub' / 'nested.txt').read_text(), 'nested')
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'sub', 'download.txt', 'upload.txt'])

        # Download
        self.select(gui, 'Remote', 'download.txt')
        gui.trigger(W + 'CurrentCopyAction')
        self.confirm_copy(gui, "Download file 'download.txt' to local directory:")
        self.assertEqual((local / 'download.txt').read_text(), 'remote content')
        self.assertIn('download.txt', gui.names(W + 'LocalDirView'))

        # Create directory
        gui.trigger(W + 'CurrentCreateDirAction')
        gui.wait_window('CreateDirectoryDialog')
        self.assertEqual(gui.get('CreateDirectoryDialog/DirectoryEdit')['text'], 'New folder')
        gui.set('CreateDirectoryDialog/DirectoryEdit', 'created')
        gui.click('CreateDirectoryDialog/OKButton')
        gui.wait_no_window('CreateDirectoryDialog')
        gui.wait_idle()
        self.assertTrue((remote / 'created').is_dir())

        # Rename in place
        self.select(gui, 'Remote', 'upload.txt')
        gui.trigger(W + 'CurrentRenameAction')
        gui.wait_idle()
        editor = W + 'RemoteDirView/RenameEdit'
        self.assertEqual(gui.get(editor)['text'], 'upload.txt')
        gui.set(editor, 'renamed.txt')
        gui.key('Return', editor)
        gui.wait_for(lambda: 'renamed.txt' in gui.names(W + 'RemoteDirView'), what='the renamed file')
        self.assertTrue((remote / 'renamed.txt').exists())
        self.assertFalse((remote / 'upload.txt').exists())

        # Delete, confirmed
        self.select(gui, 'Remote', 'renamed.txt', 'sub')
        gui.trigger(W + 'CurrentDeleteAction')
        gui.wait_window('MessageForm')
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'], 'Are you sure you want to delete 2 selected files?')
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        gui.wait_idle()
        self.assertFalse((remote / 'renamed.txt').exists())
        self.assertFalse((remote / 'sub').exists())
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'created', 'download.txt'])

        # Into a directory and up: the directory is focused
        gui.activate(W + 'RemoteDirView', 'created')
        gui.wait_idle()
        self.assertEqual(gui.get(W + 'RemotePathComboBox')['text'], str(remote / 'created'))
        gui.activate(W + 'RemoteDirView', '..')
        gui.wait_idle()
        current = [row['cells'][0] for row in gui.get(W + 'RemoteDirView')['rows'] if row['current']]
        self.assertEqual(current, ['created'])
        self.screenshot('commander')

        # Local delete goes to the trash
        self.select(gui, 'Local', 'download.txt')
        gui.trigger(W + 'CurrentDeleteAction')
        gui.wait_window('MessageForm')
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'],
                         "Are you sure you want to move file 'download.txt' to recycle bin?")
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        gui.wait_idle()
        self.assertFalse((local / 'download.txt').exists())
        trash = self.session.home / '.local' / 'share' / 'Trash' / 'files'
        self.assertEqual((trash / 'download.txt').read_text(), 'remote content')
        self.assertEqual(gui.names(W + 'LocalDirView'), ['..', 'sub', 'upload.txt'])
        self.assertEqual(gui.quit(), 0)

    def row(self, gui, side, name):
        """The cells of the row of a file in a panel."""
        return next(row for row in gui.rows(W + side + 'DirView') if row[0] == name)

    def open_properties(self, gui, side, *names):
        self.select(gui, side, *names)
        gui.trigger(W + 'CurrentPropertiesAction')
        gui.wait_window('PropertiesDialog')

    def submit_properties(self, gui):
        gui.click(P + 'OkButton')
        gui.wait_no_window('PropertiesDialog')
        gui.wait_idle()

    def test_properties(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (remote / 'file.txt').write_text('checksum me')
        os.chmod(remote / 'file.txt', 0o644)
        (remote / 'dir' / 'sub').mkdir(parents=True)
        (remote / 'dir' / 'sub' / 'nested.txt').write_text('x' * 3000)
        (local / 'local.txt').write_text('local')
        os.chmod(local / 'local.txt', 0o644)
        mode = lambda path: stat.S_IMODE(os.stat(path).st_mode)

        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        # The permissions of a remote file; SFTP changes the owner by its ID
        self.open_properties(gui, 'Remote', 'file.txt')
        self.assertEqual(gui.window('PropertiesDialog')['title'], 'file.txt Properties')
        self.assertEqual(gui.get(P + 'LocationLabel')['text'], str(remote))
        self.assertEqual(gui.get(P + 'SizeLabel')['text'], '11 B')
        self.assertEqual(gui.get(P + 'OctalEdit')['text'], '0644')
        self.assertEqual(gui.get(P + 'OwnerComboBox')['text'], '%s [%d]' % (SSHD.user, os.getuid()))
        self.assertFalse(gui.get(P + 'OkButton')['enabled'])
        gui.click(P + 'GroupWriteCheck')
        self.assertEqual(gui.get(P + 'OctalEdit')['text'], '0664')
        self.assertTrue(gui.get(P + 'OkButton')['enabled'])
        self.screenshot('properties')
        self.submit_properties(gui)
        self.assertEqual(mode(remote / 'file.txt'), 0o664)
        self.assertIn('rw-rw-r--', self.row(gui, 'Remote', 'file.txt'))

        # A directory, recursively, with the size calculated
        self.open_properties(gui, 'Remote', 'dir')
        self.assertEqual(gui.get(P + 'SizeLabel')['text'], 'Unknown')
        gui.click(P + 'CalculateSizeButton')
        gui.wait_idle()
        gui.wait_for(lambda: gui.get(P + 'SizeLabel')['text'] != 'Unknown', what='the size')
        self.assertEqual(gui.get(P + 'SizeLabel')['text'], '2.92 KB (3,000 B)')
        self.assertEqual(gui.get(P + 'FileLabel')['text'], '1 file, 2 folders')
        gui.set(P + 'OctalEdit', '750')
        gui.set(P + 'RecursiveCheck2', True)
        self.assertTrue(gui.get(P + 'OthersReadCheck')['checked'] is False)
        self.submit_properties(gui)
        self.assertEqual(mode(remote / 'dir'), 0o750)
        self.assertEqual(mode(remote / 'dir' / 'sub'), 0o750)
        self.assertEqual(mode(remote / 'dir' / 'sub' / 'nested.txt'), 0o750)

        # The checksum needs a separate shell session (SFTP of OpenSSH has
        # no checksums)
        self.open_properties(gui, 'Remote', 'file.txt')
        gui.set(P + 'PageControl', 'Checksum')
        gui.wait_window('MessageForm')
        self.assertIn('separate shell session', gui.get('MessageForm/MainMessage')['text'])
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        gui.wait_idle()
        gui.wait_for(lambda: gui.get(P + 'ChecksumEdit')['text'], 60, 'the checksum')
        digest = lambda alg: hashlib.new(alg.replace('-', ''), b'checksum me').hexdigest()
        self.assertEqual(gui.get(P + 'ChecksumEdit')['text'], digest(gui.get(P + 'ChecksumAlgEdit')['text']))
        gui.set(P + 'ChecksumAlgEdit', 'md5')
        self.assertFalse(gui.get(P + 'ChecksumEdit')['visible'])
        gui.click(P + 'ChecksumButton')
        gui.wait_idle()
        gui.wait_for(lambda: gui.get(P + 'ChecksumEdit')['text'], 60, 'the md5 checksum')
        self.assertEqual(gui.get(P + 'ChecksumEdit')['text'], digest('md5'))
        self.screenshot('checksum')
        gui.click(P + 'CancelButton')
        gui.wait_no_window('PropertiesDialog')

        # A local file: its owner cannot be changed
        self.open_properties(gui, 'Local', 'local.txt')
        self.assertEqual(gui.get(P + 'OwnerView')['text'], SSHD.user)
        self.assertFalse(gui.get(P + 'OwnerComboBox')['visible'])
        gui.set(P + 'OctalEdit', '600')
        self.submit_properties(gui)
        self.assertEqual(mode(local / 'local.txt'), 0o600)
        self.assertIn('rw-------', self.row(gui, 'Local', 'local.txt'))
        self.assertEqual(gui.quit(), 0)

    def open_copy(self, gui, side, *names, move=False):
        self.select(gui, side, *names)
        gui.trigger(W + ('CurrentMoveAction' if move else 'CurrentCopyAction'))
        gui.wait_window('CopyDialog')

    def submit_copy(self, gui):
        gui.click(C + 'OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()

    def test_transfer_settings(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'a.txt').write_text('a')
        (local / 'b.bin').write_bytes(b'b')
        (local / 'sub').mkdir()
        (local / 'sub' / 'c.txt').write_text('c')
        old = 1000000000
        for path in (local / 'a.txt', local / 'sub' / 'c.txt'):
            os.utime(path, (old, old))

        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        # A file mask and no timestamps, in the transfer settings
        self.open_copy(gui, 'Local', 'a.txt', 'b.bin', 'sub')
        self.assertEqual(gui.get(C + 'RemoteDirectoryEdit')['text'], str(remote) + '/*.*')
        self.assertTrue(gui.get(C + 'QueueCheck2')['enabled'])
        self.assertFalse(gui.get(C + 'QueueCheck2')['checked'])
        gui.click(C + 'TransferSettingsButton')
        gui.wait_window('CopyParamCustomDialog')
        T = 'CopyParamCustomDialog/'
        self.assertTrue(gui.get(T + 'PreserveTimeCheck')['checked'])
        gui.set(T + 'IncludeFileMaskCombo', '*.txt')
        gui.set(T + 'PreserveTimeCheck', False)
        self.assertFalse(gui.get(T + 'PreserveTimeDirsCheck')['enabled'])
        self.screenshot('transfer-settings')
        gui.click(T + 'OkButton')
        gui.wait_no_window('CopyParamCustomDialog')
        info = gui.get(C + 'CopyParamLabel')['text']
        self.assertIn('*.txt', info)
        self.assertIn('Do not preserve timestamp', info)
        self.screenshot('copy')
        self.submit_copy(gui)
        self.assertEqual((remote / 'a.txt').read_text(), 'a')
        self.assertEqual((remote / 'sub' / 'c.txt').read_text(), 'c')
        self.assertFalse((remote / 'b.bin').exists())
        self.assertGreater((remote / 'a.txt').stat().st_mtime, old + 1000)

        # The settings were for that transfer only: a new name, timestamps
        self.open_copy(gui, 'Local', 'b.bin')
        self.assertNotIn('*.txt', gui.get(C + 'CopyParamLabel')['text'])
        gui.set(C + 'RemoteDirectoryEdit', str(remote) + '/renamed.bin')
        self.submit_copy(gui)
        self.assertEqual((remote / 'renamed.bin').read_bytes(), b'b')
        self.assertFalse((remote / 'b.bin').exists())

        # Multiple files to a single name are confirmed
        self.open_copy(gui, 'Local', 'a.txt', 'b.bin')
        gui.set(C + 'RemoteDirectoryEdit', str(remote) + '/one.txt')
        gui.click(C + 'OkButton')
        gui.wait_window('MessageForm')
        gui.click('MessageForm/Cancel')
        gui.wait_no_window('MessageForm')
        self.assertIsNotNone(gui.window('CopyDialog'))
        gui.click(C + 'CancelButton')
        gui.wait_no_window('CopyDialog')

        # Download and delete (F6) into a new local directory
        (remote / 'moved.txt').write_text('moved')
        gui.trigger(W + 'RemoteRefreshAction')
        gui.wait_idle()
        self.open_copy(gui, 'Remote', 'moved.txt', move=True)
        self.assertEqual(gui.window('CopyDialog')['title'], 'Download and Delete')
        self.assertEqual(gui.get(C + 'DirectoryLabel')['text'],
                         "Download file 'moved.txt' to local directory and delete original:")
        gui.set(C + 'LocalDirectoryEdit', str(local / 'new') + '/*.*')
        gui.click(C + 'OkButton')
        gui.wait_window('MessageForm')
        self.assertIn("doesn't exist", gui.get('MessageForm/MainMessage')['text'])
        gui.click('MessageForm/OK')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((local / 'new' / 'moved.txt').read_text(), 'moved')
        self.assertFalse((remote / 'moved.txt').exists())
        gui.wait_for(lambda: 'moved.txt' not in gui.names(W + 'RemoteDirView'), what='the moved file')

        # Do not show the dialog again
        self.open_copy(gui, 'Local', 'b.bin')
        gui.set(C + 'NeverShowAgainCheck', True)
        self.submit_copy(gui)
        self.assertEqual((remote / 'b.bin').read_bytes(), b'b')
        (local / 'd.txt').write_text('d')
        gui.trigger(W + 'LocalRefreshAction')
        gui.wait_idle()
        self.select(gui, 'Local', 'd.txt')
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_idle()
        self.assertIsNone(gui.window('CopyDialog'))
        self.assertEqual((remote / 'd.txt').read_text(), 'd')
        self.assertEqual(gui.quit(), 0)
        self.assertIn('ConfirmTransferring=0', gui.config_file.read_text())

    def test_transfer_presets(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'a.txt').write_text('a')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        # A preset saved from the copy dialog
        self.open_copy(gui, 'Local', 'a.txt')
        # Popup menus need the real input
        gui.click_real(C + 'TransferSettingsDropDownButton')
        gui.wait_window('CopyParamListMenu')
        self.assertTrue(gui.get('CopyParamListMenu/CustomItem')['checked'] is False)
        self.assertTrue(gui.get('CopyParamListMenu/DefaultItem')['checked'])
        gui.trigger('CopyParamListMenu/SavePresetItem')
        gui.wait_window('CopyParamPresetDialog')
        R = 'CopyParamPresetDialog/'
        self.assertFalse(gui.get(R + 'OkButton')['enabled'])
        gui.set(R + 'DescriptionEdit', 'No time')
        gui.set(R + 'PreserveTimeCheck', False)
        gui.set(R + 'HasRuleCheck', True)
        gui.set(R + 'HostNameEdit', '127.0.0.1')
        gui.click(R + 'OkButton')
        gui.wait_no_window('CopyParamPresetDialog')
        # The settings of the dialog are not the preset's
        self.assertNotIn('timestamp', gui.get(C + 'CopyParamLabel')['text'])

        # The preset selected
        gui.click_real(C + 'TransferSettingsDropDownButton')
        gui.wait_window('CopyParamListMenu')
        items = gui.get('CopyParamListMenu', children=True)['children']
        preset = 'CopyParamListMenu/' + next(item['name'] for item in items if item.get('text') == 'No time')
        self.assertFalse(gui.get(preset)['checked'])
        gui.trigger(preset)
        gui.wait_no_window('CopyParamListMenu')
        self.assertIn('Do not preserve timestamp', gui.get(C + 'CopyParamLabel')['text'])
        gui.click_real(C + 'TransferSettingsDropDownButton')
        gui.wait_window('CopyParamListMenu')
        self.assertTrue(gui.get(preset)['checked'])
        gui.key('Escape', 'CopyParamListMenu')
        gui.wait_no_window('CopyParamListMenu')
        self.submit_copy(gui)
        self.assertEqual((remote / 'a.txt').read_text(), 'a')

        # The preset is selected by its rule (the host) once a directory
        # changes
        self.assertEqual(gui.get(W + 'TransferList')['text'], 'Default')
        gui.activate(W + 'RemoteDirView', '..')
        gui.wait_window('MessageForm')
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'],
                         "Transfer settings preset 'No time' was automatically selected.")
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        self.assertEqual(gui.get(W + 'TransferList')['text'], 'No time')
        self.assertIn('No time', gui.get(W + 'TransferList')['items'])
        self.assertEqual(gui.quit(), 0)
        ini = gui.config_file.read_text()
        preset = ini[ini.index('Name=No%20time'):]
        self.assertIn('PreserveTime=0', preset)
        self.assertIn('HostName=127.0.0.1', preset)

    def test_remote_transfer(self):
        remote = self.work_dir('remote')
        (remote / 'file.txt').write_text('file')
        (remote / 'dir').mkdir()
        (remote / 'dir' / 'nested.txt').write_text('nested')
        (remote / 'target').mkdir()
        gui = self.start_gui(ini=site_ini(remote=remote))
        self.login(gui, 'local')

        # A file duplicated directly (copy-data of the SFTP server)
        self.select(gui, 'Remote', 'file.txt')
        gui.trigger(W + 'RemoteCopyToAction')
        gui.wait_window('RemoteTransferDialog')
        D = 'RemoteTransferDialog/'
        self.assertEqual(gui.get(D + 'DirectoryEdit')['text'], str(remote) + '/file.txt')
        self.assertFalse(gui.get(D + 'NotDirectCopyCheck')['checked'])
        gui.set(D + 'DirectoryEdit', str(remote) + '/copy.txt')
        gui.click(D + 'OkButton')
        gui.wait_no_window('RemoteTransferDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'copy.txt').read_text(), 'file')
        self.assertIn('copy.txt', gui.names(W + 'RemoteDirView'))

        # Via a local temporary copy
        self.select(gui, 'Remote', 'file.txt')
        gui.trigger(W + 'RemoteCopyToAction')
        gui.wait_window('RemoteTransferDialog')
        gui.set(D + 'DirectoryEdit', str(remote / 'target') + '/')
        gui.set(D + 'NotDirectCopyCheck', True)
        gui.click(D + 'OkButton')
        gui.wait_no_window('RemoteTransferDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'target' / 'file.txt').read_text(), 'file')

        # A directory needs the separate shell session
        self.select(gui, 'Remote', 'dir')
        gui.trigger(W + 'RemoteCopyToAction')
        gui.wait_window('RemoteTransferDialog')
        gui.set(D + 'DirectoryEdit', str(remote) + '/dircopy')
        gui.click(D + 'OkButton')
        gui.wait_window('MessageForm')
        self.assertIn('separate shell session', gui.get('MessageForm/MainMessage')['text'])
        gui.click('MessageForm/OK')
        gui.wait_no_window('RemoteTransferDialog')
        gui.wait_idle()
        gui.wait_for(lambda: (remote / 'dircopy' / 'nested.txt').exists(), 60, 'the duplicated directory')
        self.assertEqual((remote / 'dircopy' / 'nested.txt').read_text(), 'nested')

        # Moved into a directory (an existing directory is the target)
        self.select(gui, 'Remote', 'copy.txt')
        gui.trigger(W + 'RemoteMoveToAction')
        gui.wait_window('RemoteMoveDialog')
        gui.set('RemoteMoveDialog/Combo', str(remote / 'target'))
        gui.click('RemoteMoveDialog/OKButton')
        gui.wait_no_window('RemoteMoveDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'target' / 'copy.txt').read_text(), 'file')
        self.assertFalse((remote / 'copy.txt').exists())
        self.assertEqual(gui.quit(), 0)

    def queue_copy(self, gui, side, *names):
        """A transfer in the background."""
        self.open_copy(gui, side, *names)
        self.assertTrue(gui.get(C + 'QueueCheck2')['enabled'])
        gui.set(C + 'QueueCheck2', True)
        self.submit_copy(gui)

    def queue_rows(self, gui):
        return gui.rows(Q)

    def test_queue(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        for name in ('a.txt', 'b.txt', 'c.txt'):
            (local / name).write_text(name)
        (remote / 'existing.txt').write_text('remote')
        (local / 'existing.txt').write_text('local')
        # Queue items confirm overwrites
        ini = site_ini(remote=remote, local=local) + '[Configuration\\Interface\\CopyParam]\nQueueNoConfirmation=0\n'
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local')
        self.assertFalse(gui.get(W + 'QueuePanel')['visible'])

        # An upload in the background: the queue shows, the item completes
        self.queue_copy(gui, 'Local', 'a.txt', 'b.txt')
        gui.wait_for(lambda: (remote / 'b.txt').exists() and (remote / 'b.txt').read_text() == 'b.txt', 60, 'the upload')
        gui.wait_for(lambda: self.queue_rows(gui) and self.queue_rows(gui)[0][6] == 'Completed', 30, 'the completed item')
        self.assertTrue(gui.get(W + 'QueuePanel')['visible'])
        row = self.queue_rows(gui)[0]
        self.assertEqual(row[1], str(local))
        self.assertEqual(row[2], str(remote) + '/*.*')
        self.assertEqual(gui.get(W + 'QueueLabel')['text'], 'Queue')
        gui.wait_for(lambda: 'b.txt' in gui.names(W + 'RemoteDirView'), what='the reloaded remote panel')
        self.screenshot('queue')

        # With the queue disabled, the item waits
        gui.trigger(W + 'QueueDeleteAllDoneAction')
        gui.wait_for(lambda: not self.queue_rows(gui), what='the queue to be cleared')
        gui.trigger(W + 'QueueEnableAction')
        self.assertFalse(gui.get(W + 'QueueEnableAction')['checked'])
        self.queue_copy(gui, 'Local', 'c.txt')
        gui.wait_for(lambda: self.queue_rows(gui) and self.queue_rows(gui)[0][6] == 'Waiting...', 30, 'the pending item')
        self.assertEqual(gui.get(W + 'QueueLabel')['text'], 'Queue (1)')
        self.assertFalse((remote / 'c.txt').exists())
        # Cancelled
        gui.select(Q, '')
        gui.wait_idle()
        self.assertTrue(gui.get(W + 'QueueItemExecuteAction')['enabled'])
        gui.trigger(W + 'QueueItemDeleteAction')
        gui.wait_for(lambda: not self.queue_rows(gui), what='the cancelled item')
        gui.wait_for(lambda: not gui.get(W + 'QueuePanel')['visible'], what='the hidden queue')
        # Executed now
        self.queue_copy(gui, 'Local', 'c.txt')
        gui.wait_for(lambda: self.queue_rows(gui), 30, 'the pending item')
        gui.select(Q, '')
        gui.wait_idle()
        gui.trigger(W + 'QueueItemExecuteAction')
        gui.wait_for(lambda: (remote / 'c.txt').exists(), 60, 'the executed upload')
        gui.trigger(W + 'QueueEnableAction')
        self.assertTrue(gui.get(W + 'QueueEnableAction')['checked'])

        # A query of the queue (overwrite) pops up automatically
        self.queue_copy(gui, 'Remote', 'existing.txt')
        gui.wait_window('MessageForm', 60)
        self.assertIn('existing.txt', gui.get('MessageForm', children=True)['children'][0].get('text', '') +
                      ' '.join(child.get('text', '') for child in gui.get('MessageForm', children=True)['children']))
        gui.click('MessageForm/Yes')
        gui.wait_no_window('MessageForm')
        gui.wait_for(lambda: (local / 'existing.txt').read_text() == 'remote', 60, 'the overwritten file')

        # Disconnect once the queue is empty
        gui.trigger(W + 'QueueDeleteAllDoneAction')
        gui.trigger(W + 'QueueEnableAction')
        (local / 'd.txt').write_text('d')
        gui.trigger(W + 'LocalRefreshAction')
        gui.wait_idle()
        self.queue_copy(gui, 'Local', 'd.txt')
        gui.wait_for(lambda: self.queue_rows(gui), 30, 'the pending item')
        gui.trigger(W + 'QueueDisconnectOnceEmptyAction2')
        gui.trigger(W + 'QueueEnableAction')
        gui.wait_window('MessageForm', 60)
        self.assertIn('All background transfers were finished', ' '.join(
            child.get('text', '') for child in gui.get('MessageForm', children=True)['children']))
        self.screenshot('once-empty')
        gui.click('MessageForm/No')
        gui.wait_no_window('MessageForm')
        gui.wait_for(lambda: gui.get(W + 'RemoteNoSessionPage')['visible'], what='the disconnected session')
        self.assertEqual((remote / 'd.txt').read_text(), 'd')
        self.assertEqual(gui.quit(), 0)

    def test_find_files(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (remote / 'a.txt').write_text('a')
        (remote / 'b.log').write_text('b')
        (remote / 'sub' / 'deeper').mkdir(parents=True)
        (remote / 'sub' / 'c.txt').write_text('c')
        (remote / 'sub' / 'deeper' / 'd.txt').write_text('d')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        F = 'FileFindDialog/'
        gui.trigger(W + 'RemoteFindFilesAction2')
        gui.wait_window('FileFindDialog')
        self.assertEqual(gui.get(F + 'MaskEdit')['text'], '*.*')
        self.assertEqual(gui.get(F + 'RemoteDirectoryEdit')['text'], str(remote))
        gui.set(F + 'MaskEdit', '*.txt')
        gui.click(F + 'StartStopButton')
        gui.wait_for(lambda: gui.window('FileFindDialog')['title'].startswith('Find ') and
                     gui.get(F + 'StartStopButton')['text'] == '&Start', 60, 'the find')
        rows = {row[0]: row[1] for row in gui.rows(F + 'FileView2')}
        self.assertEqual(rows, {'a.txt': './', 'c.txt': './sub/', 'd.txt': './sub/deeper/'})
        self.screenshot('find')

        # Focus of a file found in a subdirectory
        gui.select(F + 'FileView2', 'd.txt')
        gui.wait_idle()
        gui.trigger(F + 'FocusAction')
        gui.wait_idle()
        self.assertEqual(gui.get(W + 'RemotePathComboBox')['text'], str(remote / 'sub' / 'deeper'))
        current = [row['cells'][0] for row in gui.get(W + 'RemoteDirView')['rows'] if row['current']]
        self.assertEqual(current, ['d.txt'])

        # Download of files found
        gui.select(F + 'FileView2', 'a.txt', 'c.txt')
        gui.wait_idle()
        gui.trigger(F + 'DownloadAction')
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get(C + 'DirectoryLabel')['text'], 'Download 2 files to local directory:')
        self.submit_copy(gui)
        self.assertEqual((local / 'a.txt').read_text(), 'a')
        self.assertEqual((local / 'c.txt').read_text(), 'c')
        self.assertFalse(any(row['selected'] for row in gui.get(F + 'FileView2')['rows']))

        # Delete of a file found: it leaves the results
        gui.select(F + 'FileView2', 'a.txt')
        gui.wait_idle()
        gui.trigger(F + 'DeleteAction')
        gui.wait_window('MessageForm')
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        gui.wait_idle()
        self.assertFalse((remote / 'a.txt').exists())
        self.assertEqual(sorted(gui.names(F + 'FileView2')), ['c.txt', 'd.txt'])
        gui.call('close', target='FileFindDialog')
        gui.wait_no_window('FileFindDialog')
        self.assertEqual(gui.quit(), 0)

    def test_synchronize(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        old, new = 1500000000, 1600000000
        for directory in (local, remote):
            (directory / 'same.txt').write_text('same')
            os.utime(directory / 'same.txt', (old, old))
        (local / 'changed.txt').write_text('local version')
        os.utime(local / 'changed.txt', (new, new))
        (remote / 'changed.txt').write_text('remote')
        os.utime(remote / 'changed.txt', (old, old))
        (local / 'new.txt').write_text('new')
        (remote / 'obsolete.txt').write_text('obsolete')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        S = 'FullSynchronizeDialog/'
        gui.focus(W + 'LocalDirView')
        gui.wait_idle()
        gui.trigger(W + 'FullSynchronizeAction2')
        gui.wait_window('FullSynchronizeDialog')
        self.assertEqual(gui.get(S + 'LocalDirectoryEdit')['text'], str(local))
        self.assertEqual(gui.get(S + 'RemoteDirectoryEdit')['text'], str(remote))
        self.assertTrue(gui.get(S + 'SynchronizeRemoteButton')['checked'])
        self.assertTrue(gui.get(S + 'SynchronizePreviewChangesCheck')['checked'])
        gui.set(S + 'SynchronizeDeleteCheck', True)
        self.screenshot('synchronize')
        gui.click(S + 'OkButton')
        gui.wait_window('SynchronizeChecklistDialog', 60)
        K = 'SynchronizeChecklistDialog/'
        rows = {row[0]: row for row in gui.rows(K + 'ListView2')}
        self.assertEqual(sorted(rows), ['changed.txt', 'new.txt', 'obsolete.txt'])
        self.assertEqual(gui.get(K + 'StatusText0')['text'], '3 of 3 (16 B)')
        self.assertEqual(gui.get(K + 'StatusText1')['text'], '1 of 1 (3 B)')
        self.assertEqual(gui.get(K + 'StatusText5')['text'], '1 of 1')
        self.assertEqual(gui.get(K + 'StatusText2')['text'], 'N/A')

        # The obsolete file stays
        gui.select(K + 'ListView2', 'obsolete.txt')
        gui.wait_idle()
        gui.click(K + 'UncheckButton')
        self.assertEqual(gui.get(K + 'StatusText0')['text'], '2 of 3 (16 B)')
        # Reversed and back
        gui.select(K + 'ListView2', 'changed.txt')
        gui.wait_idle()
        gui.click(K + 'ReverseButton')
        self.assertEqual(gui.get(K + 'StatusText4')['text'], '1 of 1 (6 B)')
        gui.click(K + 'ReverseButton')
        self.assertEqual(gui.get(K + 'StatusText3')['text'], '1 of 1 (13 B)')
        self.screenshot('checklist')
        gui.click(K + 'OkButton')
        gui.wait_window('MessageForm', 60)
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'], 'Synchronization was completed.')
        gui.click('MessageForm/OK')
        gui.wait_no_window('MessageForm')
        gui.wait_no_window('SynchronizeChecklistDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'new.txt').read_text(), 'new')
        self.assertEqual((remote / 'changed.txt').read_text(), 'local version')
        self.assertEqual(int((remote / 'changed.txt').stat().st_mtime), new)
        self.assertTrue((remote / 'obsolete.txt').exists())

        # Again, in the background (the queue)
        (local / 'later.txt').write_text('later')
        gui.trigger(W + 'LocalRefreshAction')
        gui.wait_idle()
        gui.trigger(W + 'FullSynchronizeAction2')
        gui.wait_window('FullSynchronizeDialog')
        gui.set(S + 'SynchronizeDeleteCheck', False)
        gui.click(S + 'OkButton')
        gui.wait_window('SynchronizeChecklistDialog', 60)
        # Without deleting, the obsolete file is listed unchecked
        checked = {row['cells'][0]: row['checked'] for row in gui.get(K + 'ListView2')['rows']}
        self.assertEqual(checked, {'later.txt': True, 'obsolete.txt': False})
        gui.click_real(K + 'OkDropDownButton')
        gui.wait_window('OkPopupMenu')
        gui.trigger(K + 'StartQueueItem')
        gui.wait_no_window('SynchronizeChecklistDialog')
        gui.wait_for(lambda: (remote / 'later.txt').exists(), 60, 'the queued synchronization')
        self.assertEqual((remote / 'later.txt').read_text(), 'later')
        self.assertEqual(gui.quit(), 0)

    def test_keep_up_to_date(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'initial.txt').write_text('initial')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        D = 'SynchronizeDialog/'
        gui.trigger(W + 'SynchronizeAction')
        gui.wait_window('SynchronizeDialog')
        self.assertEqual(gui.get(D + 'LocalDirectoryEdit')['text'], str(local))
        self.assertEqual(gui.get(D + 'RemoteDirectoryEdit')['text'], str(remote))
        self.assertTrue(gui.get(D + 'SynchronizeRecursiveCheck')['checked'])
        # Asks whether to synchronize first, by default
        self.assertTrue(gui.get(D + 'SynchronizeSynchronizeCheck')['partial'])
        self.assertFalse(gui.get(D + 'StopButton')['visible'])
        self.assertFalse(gui.get(D + 'LogPanel')['visible'])
        gui.set(D + 'SynchronizeDeleteCheck', True)
        gui.click(D + 'StartButton')
        gui.wait_window('MessageForm')
        self.assertIn('full synchronisation', gui.get('MessageForm/MainMessage')['text'])
        gui.click('MessageForm/Yes')
        gui.wait_no_window('MessageForm')

        def messages():
            return [row[1] for row in gui.rows(D + 'LogView')]

        def wait_message(text, what):
            gui.wait_for(lambda: any(text in message for message in messages()), 60, what)

        # Synchronized first, then watching
        wait_message('Watching for changes in 1 directories...', 'watching')
        self.assertEqual((remote / 'initial.txt').read_text(), 'initial')
        self.assertTrue(gui.get(D + 'StopButton')['visible'])
        self.assertFalse(gui.get(D + 'StartButton')['visible'])
        self.assertTrue(gui.get(D + 'LogPanel')['visible'])
        self.assertFalse(gui.get(D + 'OptionsGroup')['enabled'])

        # A new file
        (local / 'new.txt').write_text('new')
        gui.wait_for(lambda: (remote / 'new.txt').exists(), 60, 'the upload of the new file')
        wait_message("File '%s' uploaded." % (local / 'new.txt'), 'the log of the upload')
        self.assertIn("Change in '%s' detected." % local, messages())
        self.assertEqual((remote / 'new.txt').read_text(), 'new')

        # A new subdirectory, then watched too
        (local / 'sub').mkdir()
        (local / 'sub' / 'inner.txt').write_text('inner')
        gui.wait_for(lambda: (remote / 'sub' / 'inner.txt').exists(), 60, 'the upload of the subdirectory')
        wait_message('Watching for changes in 2 directories...', 'watching the subdirectory')
        # Compared by the modification time only, which is to differ (in
        # seconds, as by SFTP)
        inner = local / 'sub' / 'inner.txt'
        later = inner.stat().st_mtime + 100
        inner.write_text('inner changed')
        os.utime(inner, (later, later))
        gui.wait_for(
            lambda: (remote / 'sub' / 'inner.txt').read_text() == 'inner changed', 60,
            'the upload of the changed file')

        # A deleted file
        (local / 'new.txt').unlink()
        gui.wait_for(lambda: not (remote / 'new.txt').exists(), 60, 'the deletion')
        wait_message("File '%s' deleted." % (remote / 'new.txt'), 'the log of the deletion')
        self.screenshot('keep_up_to_date')

        gui.click(D + 'StopButton')
        gui.wait_for(lambda: gui.get(D + 'StartButton')['visible'], 10, 'stopping')
        self.assertFalse(gui.get(D + 'LogPanel')['visible'])
        self.assertTrue(gui.get(D + 'OptionsGroup')['enabled'])
        # Not watching anymore
        (local / 'after.txt').write_text('after')
        time.sleep(2)
        self.assertFalse((remote / 'after.txt').exists())
        gui.click(D + 'CancelButton')
        gui.wait_no_window('SynchronizeDialog')
        self.assertEqual(gui.quit(), 0)

    def test_bookmarks(self):
        remote = self.work_dir('remote')
        (remote / 'deep').mkdir()
        local = self.work_dir('local')
        gui = self.start_gui(ini=site_ini(remote=remote / 'deep', local=local))
        self.login(gui, 'local')

        def path(side):
            return gui.get(W + side + 'PathComboBox')['text']

        def wait_path(side, expected):
            gui.wait_for(lambda: path(side) == str(expected), 30, 'the %s directory %s' % (side, expected))

        # A bookmark of the site
        O = 'OpenDirectoryDialog/'
        gui.focus(W + 'RemoteDirView')
        gui.wait_idle()
        gui.trigger(W + 'RemoteAddBookmarkAction2')
        gui.wait_window('MessageForm')
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'],
                         "Add directory '%s' to bookmarks?" % (remote / 'deep'))
        gui.click('MessageForm/OK')
        gui.wait_window('OpenDirectoryDialog')
        self.assertEqual(gui.names(O + 'SessionBookmarksList'), [str(remote / 'deep')])
        gui.click(O + 'OKBtn')
        gui.wait_no_window('OpenDirectoryDialog')

        # Elsewhere, then back by the bookmark
        gui.trigger(W + 'RemoteParentDirAction')
        wait_path('Remote', remote)
        gui.trigger(W + 'RemoteOpenDirAction')
        gui.wait_window('OpenDirectoryDialog')
        self.assertEqual(gui.get(O + 'RemoteDirectoryEdit')['text'], str(remote))
        gui.select(O + 'SessionBookmarksList', [str(remote / 'deep')])
        self.assertEqual(gui.get(O + 'RemoteDirectoryEdit')['text'], str(remote / 'deep'))
        gui.click(O + 'OKBtn')
        gui.wait_no_window('OpenDirectoryDialog')
        wait_path('Remote', remote / 'deep')

        # A shared bookmark with a shortcut
        gui.focus(W + 'LocalDirView')
        gui.wait_idle()
        gui.trigger(W + 'LocalAddBookmarkAction2')
        gui.wait_window('MessageForm')
        self.assertEqual(gui.get('MessageForm/NeverAskAgainCheck')['text'], 'Add to &shared bookmarks')
        gui.set('MessageForm/NeverAskAgainCheck', True)
        gui.click('MessageForm/OK')
        gui.wait_window('OpenDirectoryDialog')
        self.assertEqual(gui.names(O + 'SharedBookmarksList'), [str(local)])
        gui.click(O + 'ShortCutSharedBookmarkButton')
        gui.wait_window('ShortCutDialog')
        gui.set('ShortCutDialog/ShortCutCombo', 'Ctrl+1')
        gui.click('ShortCutDialog/OKButton')
        gui.wait_no_window('ShortCutDialog')
        self.assertEqual(gui.names(O + 'SharedBookmarksList'), ['%s (Ctrl+1)' % local])
        self.screenshot('bookmarks')
        gui.click(O + 'OKBtn')
        gui.wait_no_window('OpenDirectoryDialog')
        gui.trigger(W + 'LocalParentDirAction')
        wait_path('Local', local.parent)
        gui.focus(W + 'LocalDirView')
        gui.wait_idle()
        self.session.input('key', 'ctrl+1')
        wait_path('Local', local)

        # Location profiles: both directories at once
        L = 'LocationProfilesDialog/'
        gui.trigger(W + 'LocalOpenDirAction')
        gui.wait_window('OpenDirectoryDialog')
        gui.click(O + 'SwitchButton')
        gui.wait_window('LocationProfilesDialog')
        self.assertEqual(gui.get(L + 'LocalDirectoryEdit')['text'], str(local))
        self.assertEqual(gui.get(L + 'RemoteDirectoryEdit')['text'], str(remote / 'deep'))
        # The shared page was the last one used
        self.assertEqual(gui.get(L + 'PageControl')['text'], 'Shared Location profiles')
        gui.set(L + 'PageControl', 'Site Location profiles')
        gui.click(L + 'AddSessionBookmarkButton')
        gui.wait_window('BookmarkNameDialog')
        gui.set('BookmarkNameDialog/NameCombo', 'both')
        gui.click('BookmarkNameDialog/OKButton')
        gui.wait_no_window('BookmarkNameDialog')
        # The bookmark of the site is a profile too
        self.assertEqual(gui.names(L + 'SessionProfilesView'), [str(remote / 'deep'), 'both'])
        self.screenshot('location_profiles')
        gui.click(L + 'OKBtn')
        gui.wait_no_window('LocationProfilesDialog')
        gui.trigger(W + 'LocalParentDirAction')
        wait_path('Local', local.parent)
        gui.trigger(W + 'RemoteParentDirAction')
        wait_path('Remote', remote)
        gui.trigger(W + 'RemoteOpenDirAction')
        gui.wait_window('LocationProfilesDialog')
        gui.select(L + 'SessionProfilesView', 'both')
        self.assertEqual(gui.get(L + 'LocalDirectoryEdit')['text'], str(local))
        gui.click(L + 'OKBtn')
        gui.wait_no_window('LocationProfilesDialog')
        wait_path('Local', local)
        wait_path('Remote', remote / 'deep')
        self.assertEqual(gui.quit(), 0)

        # Kept in the configuration
        config = gui.config_file.read_text()
        self.assertIn('UseLocationProfiles=1', config)
        self.assertIn('Bookmarks', config)
        self.assertIn('both', config)

    def test_console(self):
        remote = self.work_dir('remote')
        (remote / 'sub').mkdir()
        (remote / 'sub' / 'file.txt').write_text('content\n')
        gui = self.start_gui(ini=site_ini(remote=remote))
        self.login(gui, 'local')

        gui.trigger(W + 'ConsoleAction')
        # SFTP has no shell; the separate shell session is confirmed
        gui.wait_window('MessageForm')
        gui.click('MessageForm/OK')
        gui.wait_window('ConsoleDialog', 60)
        C = 'ConsoleDialog/'
        self.assertEqual(gui.get(C + 'DirectoryLabel')['text'], str(remote))
        gui.set(C + 'CommandEdit', 'cd sub')
        gui.click(C + 'ExecuteButton')
        gui.wait_for(lambda: gui.get(C + 'DirectoryLabel')['text'] == str(remote / 'sub'), 30, 'the directory')
        gui.set(C + 'CommandEdit', 'cat file.txt; echo done')
        gui.click(C + 'ExecuteButton')
        gui.wait_for(lambda: 'done' in gui.get(C + 'OutputMemo')['text'], 30, 'the output')
        output = gui.get(C + 'OutputMemo')['text'].splitlines()
        self.assertEqual(output[-3:], ['%s$ cat file.txt; echo done' % (remote / 'sub'), 'content', 'done'])
        self.screenshot('console')
        gui.click(C + 'CancelBtn')
        gui.wait_no_window('ConsoleDialog')
        # The panel follows the directory changed in the console
        gui.wait_for(lambda: gui.get(W + 'RemotePathComboBox')['text'] == str(remote / 'sub'), 30,
                     'the remote directory')
        self.assertEqual(gui.quit(), 0)

    def test_custom_commands(self):
        remote = self.work_dir('remote')
        (remote / 'a.txt').write_text('one\ntwo\nthree\n')
        local = self.work_dir('local')
        (local / 'l.txt').write_text('local')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        # The defaults, a new command with a shortcut (the Commands page of
        # the preferences)
        L = 'PreferencesDialog/'
        D = 'CustomCommandDialog/'
        gui.trigger(W + 'CustomCommandsCustomizeAction')
        gui.wait_window('PreferencesDialog')
        self.assertTrue(gui.get(L + 'CustomCommandsSheet')['visible'])
        self.assertEqual(gui.names(L + 'CustomCommandsView')[:3], ['Execute', 'Touch', 'Tar/GZip...'])
        self.assertEqual(gui.rows(L + 'CustomCommandsView')[-1], ['Print', 'lp "!"', 'L'])
        gui.click(L + 'AddCommandButton')
        gui.wait_window('CustomCommandDialog')
        gui.set(D + 'DescriptionEdit', 'Lines')
        gui.set(D + 'CommandEdit', 'wc -l "!"')
        gui.set(D + 'RemoteCommandButton', True)
        gui.set(D + 'ShowResultsCheck', True)
        gui.set(D + 'ShortCutCombo', 'Ctrl+2')
        self.screenshot('custom_command')
        gui.click(D + 'OkButton')
        gui.wait_no_window('CustomCommandDialog')
        self.assertEqual(gui.rows(L + 'CustomCommandsView')[-1], ['Lines', 'wc -l "!"', 'R'])
        gui.click(L + 'OKButton')
        gui.wait_no_window('PreferencesDialog')

        # By its shortcut, the results in the console
        self.select(gui, 'Remote', 'a.txt')
        self.session.input('key', 'ctrl+2')
        # SFTP has no shell; the separate shell session is confirmed
        gui.wait_window('MessageForm')
        gui.click('MessageForm/OK')
        gui.wait_window('ConsoleDialog', 60)
        self.assertIn('3 a.txt', gui.get('ConsoleDialog/OutputMemo')['text'])
        gui.click('ConsoleDialog/CancelBtn')
        gui.wait_no_window('ConsoleDialog')

        # An ad hoc remote command
        self.select(gui, 'Remote', 'a.txt')
        gui.trigger(W + 'CustomCommandsEnterAction')
        gui.wait_window('CustomCommandDialog')
        self.assertFalse(gui.get(D + 'DescriptionEdit')['visible'])
        gui.set(D + 'RemoteCommandButton', True)
        gui.set(D + 'CommandEdit', 'cp "!" "!.copy"')
        gui.click(D + 'OkButton')
        gui.wait_no_window('CustomCommandDialog')
        gui.wait_for(lambda: (remote / 'a.txt.copy').exists(), 60, 'the remote copy')

        # A local command on a remote file, with a parameter: downloaded,
        # changed and uploaded back
        self.select(gui, 'Remote', 'a.txt')
        gui.trigger(W + 'CustomCommandsEnterAction')
        gui.wait_window('CustomCommandDialog')
        gui.set(D + 'LocalCommandButton', True)
        gui.set(D + 'CommandEdit', 'sed -i "s/two/!?Replacement:?X!/" "!"')
        gui.click(D + 'OkButton')
        gui.wait_window('CustomCommandPromptsDialog')
        self.assertEqual(gui.get('CustomCommandPromptsDialog/PromptEdit0')['text'], 'X')
        gui.set('CustomCommandPromptsDialog/PromptEdit0', 'TWO')
        gui.click('CustomCommandPromptsDialog/OKButton')
        # The changed file overwrites the remote one (as upstream asks)
        gui.wait_window('MessageForm', 60)
        self.assertEqual(gui.get('MessageForm/MainMessage')['text'], "Overwrite remote file 'a.txt'?")
        gui.click('MessageForm/Yes')
        gui.wait_for(lambda: (remote / 'a.txt').read_text() == 'one\nTWO\nthree\n', 60, 'the changed file')

        # A local command on the local files
        self.select(gui, 'Local', 'l.txt')
        gui.trigger(W + 'CustomCommandsEnterAction')
        gui.wait_window('CustomCommandDialog')
        self.assertFalse(gui.get(D + 'RemoteCommandButton')['enabled'])
        self.assertTrue(gui.get(D + 'LocalCommandButton')['checked'])
        gui.set(D + 'CommandEdit', 'cp "!" "!.bak"')
        gui.click(D + 'OkButton')
        gui.wait_for(lambda: (local / 'l.txt.bak').exists(), 60, 'the local copy')
        self.assertEqual(gui.quit(), 0)

        config = gui.config_file.read_text()
        self.assertIn('Lines=wc%20-l%20"!"', config)
        self.assertIn('Lines=16434', config)

    def test_edit(self):
        remote = self.work_dir('remote')
        (remote / 'r.txt').write_text('line one\nline two\n')
        (remote / 'x.ext').write_text('external\n')
        local = self.work_dir('local')
        (local / 'l.txt').write_text('local\n')
        # An external editor of *.ext files: changes the file, stays a while
        editor = self.work_dir('editor') / 'editor.sh'
        editor.write_text('#!/bin/sh\nsleep 1\necho changed >> "$1"\nsleep 3\n')
        editor.chmod(0o755)
        ini = site_ini(remote=remote, local=local) + (
            '[Configuration\\Interface\\Editor\\0]\nFileMask=*.ext\nEditor=1\n'
            'ExternalEditor=%s !.!\nSDIExternalEditor=1\n'
            '[Configuration\\Interface\\Editor\\1]\nFileMask=*.*\nEditor=0\n' % editor)
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local')
        E = 'EditorForm/'

        # The internal editor of a remote file, uploaded once saved
        self.select(gui, 'Remote', 'r.txt')
        gui.trigger(W + 'CurrentEditAction')
        gui.wait_window('EditorForm', 60)
        self.assertEqual(gui.window('EditorForm')['title'], '%s \u2013 local \u2013 Editor \u2013 OpenSCP' % (remote / 'r.txt'))
        self.assertEqual(gui.get(E + 'EditorMemo')['text'], 'line one\nline two\n')
        self.assertEqual(gui.get(E + 'EncodingStatus')['text'], 'Encoding: UTF-8')
        gui.focus(E + 'EditorMemo')
        self.session.input('key', 'ctrl+end', 'type', 'line three', 'key', 'ctrl+s')
        gui.wait_for(lambda: (remote / 'r.txt').read_text() == 'line one\nline two\nline three', 60, 'the upload')
        gui.wait_for(lambda: gui.get(E + 'ModifiedStatus')['text'] == '', 30, 'the saved state')
        self.screenshot('editor')
        gui.key('Escape', E + 'EditorMemo')
        gui.wait_no_window('EditorForm')

        # An external editor, watched while it runs
        self.select(gui, 'Remote', 'x.ext')
        gui.trigger(W + 'CurrentEditAction')
        gui.wait_for(lambda: (remote / 'x.ext').read_text() == 'external\nchanged\n', 60, 'the external change')

        # A local file opens in the internal editor too, saved directly
        gui.focus(W + 'LocalDirView')
        gui.activate(W + 'LocalDirView', 'l.txt')
        gui.wait_window('EditorForm', 60)
        gui.focus(E + 'EditorMemo')
        self.session.input('key', 'ctrl+end', 'type', 'more', 'key', 'ctrl+s')
        gui.wait_for(lambda: (local / 'l.txt').read_text() == 'local\nmore', 30, 'the local save')
        gui.key('Escape', E + 'EditorMemo')
        gui.wait_no_window('EditorForm')

        # A new remote file
        gui.focus(W + 'RemoteDirView')
        gui.wait_idle()
        gui.trigger(W + 'NewFileAction')
        gui.wait_window('InputDialog')
        # The focused file is offered
        self.assertEqual(gui.get('InputDialog/HistoryCombo')['text'], 'x.ext')
        gui.set('InputDialog/HistoryCombo', 'new.txt')
        gui.click('InputDialog/OKButton')
        gui.wait_window('EditorForm', 60)
        self.assertEqual(gui.get(E + 'ModifiedStatus')['text'], 'New')
        gui.focus(E + 'EditorMemo')
        self.session.input('type', 'brand new', 'key', 'ctrl+s')
        gui.wait_for(lambda: (remote / 'new.txt').exists() and (remote / 'new.txt').read_text() == 'brand new',
                     60, 'the new file')
        gui.key('Escape', E + 'EditorMemo')
        gui.wait_no_window('EditorForm')
        self.assertEqual(gui.quit(), 0)

    def tab_point(self, gui, index):
        """The center of a session tab on the screen."""
        rect = gui.get(W + 'SessionsPageControl')['rects'][index]
        x, y = gui.client_origin('ScpCommanderForm')
        return x + rect['x'] + rect['width'] // 2, y + rect['y'] + rect['height'] // 2

    def test_tabs(self):
        one = self.work_dir('one')
        (one / 'a.txt').write_text('a')
        two = self.work_dir('two')
        (two / 'dir').mkdir()
        (two / 'dir' / 'b.txt').write_text('b')
        local1 = self.work_dir('local1')
        local2 = self.work_dir('local2')
        (local2 / 'sub').mkdir()
        gui = self.start_gui(ini=site_ini('first', remote=one, local=local1) + site_ini('second', remote=two, local=local2))
        tabs = W + 'SessionsPageControl'
        remote_path = W + 'RemotePathComboBox'
        local_path = W + 'LocalPathComboBox'
        self.login(gui, 'first')
        self.assertEqual(gui.get(tabs)['items'], ['first', ''])
        self.assertEqual(gui.get(tabs)['index'], 0)

        # Another session in a new tab (the host key is known already)
        gui.trigger(W + 'NewSessionAction')
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 'second')
        gui.click('LoginDialog/LoginButton')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', 'second', ''], 60, 'the second tab')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two), 60, 'the second session')
        gui.wait_idle()
        self.assertEqual(gui.get(tabs)['index'], 1)
        self.assertEqual(gui.get(local_path)['text'], str(local2))
        self.assertIn('second \u2013 OpenSCP', gui.window('ScpCommanderForm')['title'])
        self.assertIn('%s@127.0.0.1' % SSHD.user, gui.get(tabs)['tooltips'][1])
        # Its directories change
        gui.activate(W + 'RemoteDirView', 'dir')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two / 'dir'), what='the remote subdirectory')
        gui.activate(W + 'LocalDirView', 'sub')
        gui.wait_for(lambda: gui.get(local_path)['text'] == str(local2 / 'sub'), what='the local subdirectory')

        # Each tab shows its session with its directories
        gui.set(tabs, 0)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(one), what='the first session')
        gui.wait_idle()
        self.assertEqual(gui.get(local_path)['text'], str(local1))
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'a.txt'])
        self.assertIn('first \u2013 OpenSCP', gui.window('ScpCommanderForm')['title'])
        gui.set(tabs, 1)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two / 'dir'), what='the second session again')
        gui.wait_idle()
        self.assertEqual(gui.get(local_path)['text'], str(local2 / 'sub'))
        # with the history of its panels
        self.assertTrue(gui.get(W + 'RemoteBackAction')['enabled'])
        gui.trigger(W + 'RemoteBackAction')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two), what='going back')
        self.screenshot('tabs')

        # Ctrl+Tab cycles the tabs
        gui.focus(W + 'LocalDirView')
        self.session.input('key', 'ctrl+tab')
        gui.wait_for(lambda: gui.get(tabs)['index'] == 0, what='the next tab')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(one), what='the first session by the key')

        # A duplicate opens the same directory, the tabs of the same name
        # tell their directories
        gui.trigger(W + 'DuplicateTabAction')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first (one)', 'second', 'first (one)', ''], 60,
                     'the duplicate tab')
        self.assertEqual(gui.get(tabs)['index'], 2)
        gui.wait_idle()
        self.assertEqual(gui.get(remote_path)['text'], str(one))
        gui.trigger(W + 'RenameTabAction')
        gui.wait_window('InputDialog')
        self.assertEqual(gui.get('InputDialog/Edit')['text'], 'first')
        gui.set('InputDialog/Edit', 'renamed')
        gui.click('InputDialog/OKButton')
        gui.wait_no_window('InputDialog')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', 'second', 'renamed', ''], what='the renamed tab')
        self.assertIn('renamed \u2013 OpenSCP', gui.window('ScpCommanderForm')['title'])

        # Tabs are reordered by dragging them
        gui.drag_real(self.tab_point(gui, 2), self.tab_point(gui, 0))
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['renamed', 'first', 'second', ''], what='the moved tab')
        self.assertEqual(gui.get(tabs)['index'], 0)
        gui.set(tabs, 2)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two), what='the second session after the move')

        # A disconnected session stays disconnected when its tab is shown again
        gui.trigger(W + 'DisconnectSessionAction')
        gui.wait_for(lambda: gui.get(W + 'ReconnectItem')['visible'], what='the disconnected session')
        self.assertEqual(gui.get(tabs)['items'], ['renamed', 'first', 'second', ''])
        gui.set(tabs, 1)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(one), what='the first session again')
        gui.set(tabs, 2)
        gui.wait_for(lambda: gui.get(W + 'ReconnectItem')['visible'], what='the disconnected tab')
        gui.wait_idle()
        self.assertFalse(gui.get(W + 'RemoteDirView')['visible'])

        # A middle click closes a tab that is not active, closing the active
        # one shows its neighbour
        x, y = self.tab_point(gui, 0)
        self.session.input('move', x, y, 'click', 'middle')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', 'second', ''], what='the closed tab')
        self.assertEqual(gui.get(tabs)['index'], 1)
        gui.trigger(W + 'CloseTabAction')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', ''], what='the closed active tab')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(one), what='the remaining session')
        self.assertEqual(gui.quit(), 0)

    def open_menu(self, gui, menu):
        """Opens a menu of the menu bar with the real input."""
        menus = gui.get(W + 'MenuToolbar')['menus']
        rect = next(item['rect'] for item in menus if item['name'] == menu)
        x, y = gui.client_origin('ScpCommanderForm')
        self.session.input('move', x + rect['x'] + rect['width'] // 2, y + rect['y'] + rect['height'] // 2, 'click')
        gui.wait_window(menu)

    def message_text(self, gui):
        return ' '.join(child.get('text', '') for child in gui.get('MessageForm', children=True)['children'])

    def test_workspaces(self):
        one = self.work_dir('one')
        two = self.work_dir('two')
        (two / 'dir').mkdir()
        gui = self.start_gui(ini=site_ini('first', remote=one) + site_ini('second', remote=two))
        tabs = W + 'SessionsPageControl'
        remote_path = W + 'RemotePathComboBox'
        self.login(gui, 'first')
        gui.trigger(W + 'NewSessionAction')
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 'second')
        gui.click('LoginDialog/LoginButton')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two), 60, 'the second session')
        gui.wait_idle()
        gui.activate(W + 'RemoteDirView', 'dir')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two / 'dir'), what='the subdirectory')

        # The sessions saved as a workspace, with a desktop shortcut
        gui.trigger(W + 'SaveWorkspaceAction')
        gui.wait_window('SaveWorkspaceDialog')
        D = 'SaveWorkspaceDialog/'
        self.assertEqual(gui.get(D + 'WorkspaceNameCombo')['text'], 'My Workspace')
        self.assertFalse(gui.get(D + 'EnableAutoSaveCheck')['checked'])
        gui.set(D + 'WorkspaceNameCombo', 'work')
        gui.set(D + 'CreateShortcutCheck', True)
        gui.click(D + 'OKButton')
        gui.wait_no_window('SaveWorkspaceDialog')
        ini = gui.config_file.read_text()
        self.assertIn('[Sessions\\work/0000]', ini)
        self.assertIn('[Sessions\\work/0001]', ini)
        self.assertIn('RemoteDirectory=%s' % (two / 'dir'), ini)
        shortcut = gui.home / 'Desktop' / 'work.desktop'
        entry = shortcut.read_text()
        self.assertIn('Name=work\n', entry)
        self.assertRegex(entry, r'\nExec="[^"]*openscp" "work"\n')
        self.assertTrue(os.access(shortcut, os.X_OK))

        # Once the tabs are closed, the site manager has the workspace
        gui.trigger(W + 'CloseTabAction')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', ''], what='the closed tab')
        gui.trigger(W + 'CloseTabAction')
        gui.wait_window('LoginDialog')
        self.assertIn('work', gui.names('LoginDialog/SessionTree'))
        gui.click('LoginDialog/CloseButton')
        gui.wait_no_window('LoginDialog')

        # Opened from the menu: the first session connects, the other one
        # once its tab is shown, in its directory
        self.open_menu(gui, 'TabsMenu')
        gui.trigger('TabsMenu/WorkspacesMenu')
        gui.wait_window('WorkspacesMenu')
        gui.trigger('WorkspacesMenu/Workspace0')
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', 'second', ''], 60, 'the workspace')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(one), 60, 'its first session')
        gui.wait_idle()
        self.assertEqual(gui.get(tabs)['index'], 0)
        gui.set(tabs, 1)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two / 'dir'), 60, 'its second session')
        gui.wait_idle()

        # A site from the menu opens in a new tab
        self.open_menu(gui, 'TabsMenu')
        gui.trigger('TabsMenu/SavedSessionsMenu')
        gui.wait_window('SavedSessionsMenu')
        items = gui.get('SavedSessionsMenu', children=True)['children']
        site = next(item['name'] for item in items if item.get('text') == 'second')
        gui.trigger('SavedSessionsMenu/' + site)
        # The site remembers its last directory (of its closed session)
        gui.wait_for(lambda: gui.get(tabs)['items'] == ['first', 'second (dir)', 'second (dir)', ''], 60,
                     'the site in a new tab')
        self.assertEqual(gui.get(tabs)['index'], 2)
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(two / 'dir'), 60, 'the site')

        # Closing the application with the sessions is confirmed, the
        # workspace can be saved automatically
        gui.trigger(W + 'CloseApplicationAction')
        gui.wait_window('MessageForm')
        self.assertIn('Terminate all sessions and close application without saving a workspace?',
                      self.message_text(gui))
        gui.click('MessageForm/No')
        gui.wait_window('SaveWorkspaceDialog')
        self.assertEqual(gui.get(D + 'WorkspaceNameCombo')['text'], 'work')
        self.assertTrue(gui.get(D + 'EnableAutoSaveCheck')['checked'])
        gui.click(D + 'OKButton')
        self.assertEqual(gui.process.wait(timeout=60), 0)
        ini = gui.config_file.read_text()
        self.assertIn('AutoSaveWorkspace=1', ini)
        self.assertIn('AutoWorkspace=work', ini)
        self.assertIn('LastStoredSession=work', ini)
        self.assertIn('[Sessions\\work/0002]', ini)

        # The site manager selects the workspace
        gui = self.start_gui(ini=ini)
        gui.wait_window('LoginDialog')
        selected = [row['cells'][0] for row in gui.get('LoginDialog/SessionTree')['rows'] if row['current']]
        self.assertEqual(selected, ['work'])
        self.assertEqual(gui.quit(), 0)

    def test_preferences(self):
        remote = self.work_dir('remote')
        (remote / 'big.bin').write_bytes(b'x' * 3000)
        local = self.work_dir('local')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')
        # The short path in the title by default
        self.assertEqual(gui.window('ScpCommanderForm')['title'], 'remote \u2013 local \u2013 OpenSCP')
        self.assertEqual(self.row(gui, 'Remote', 'big.bin')[1], '3 KB')

        R = 'PreferencesDialog/'
        gui.trigger(W + 'PreferencesAction')
        gui.wait_window('PreferencesDialog')
        self.assertTrue(gui.get(R + 'PreferencesSheet')['visible'])
        gui.set(R + 'ConfirmDeletingCheck', False)
        gui.select(R + 'NavigationTree', ['Environment', 'Window'])
        gui.wait_for(lambda: gui.get(R + 'WindowSheet')['visible'], what='the Window page')
        gui.set(R + 'PathInCaptionFullButton', True)
        gui.select(R + 'NavigationTree', 'Panels')
        gui.set(R + 'FormatSizeBytesCombo', 'Bytes')
        gui.set(R + 'DoubleClickActionCombo', 'Copy')
        self.screenshot('preferences')
        gui.click(R + 'OKButton')
        gui.wait_no_window('PreferencesDialog')
        gui.wait_idle()
        ini = gui.config_file.read_text()
        for key in ['ConfirmDeleting=0', 'PathInCaption=1', 'FormatSizeBytes=0', 'DoubleClickAction=1']:
            self.assertIn(key, ini)

        # The full path in the title, the sizes in bytes
        self.assertEqual(gui.window('ScpCommanderForm')['title'], '%s \u2013 local \u2013 OpenSCP' % remote)
        self.assertEqual(self.row(gui, 'Remote', 'big.bin')[1], '3,000')
        # A double-click copies, without the copy dialog
        gui.focus(W + 'RemoteDirView')
        gui.activate(W + 'RemoteDirView', 'big.bin')
        gui.wait_for(lambda: (local / 'big.bin').exists() and (local / 'big.bin').stat().st_size == 3000, 60, 'the copy')
        self.assertIsNone(gui.window('CopyDialog'))
        # Deleted without the confirmation
        self.select(gui, 'Remote', 'big.bin')
        gui.trigger(W + 'CurrentDeleteAction')
        gui.wait_for(lambda: not (remote / 'big.bin').exists(), 30, 'the deletion')
        self.assertIsNone(gui.window('MessageForm'))
        self.assertEqual(gui.quit(), 0)

    def test_mark(self):
        remote = self.work_dir('remote')
        for name in ['a.txt', 'b.txt', 'c.log', 'd.log']:
            (remote / name).write_text(name)
        local = self.work_dir('local')
        (local / 'a.txt').write_text('a.txt')
        (local / 'b.txt').write_text('b.txt')
        os.utime(local / 'b.txt', (time.time() + 3600, time.time() + 3600))
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        def selected(side):
            return sorted(row['cells'][0] for row in gui.get(W + side + 'DirView')['rows'] if row['selected'])

        def current(side):
            return [row['cells'][0] for row in gui.get(W + side + 'DirView')['rows'] if row['current']]

        # Num+ selects by a mask
        gui.focus(W + 'RemoteDirView')
        self.session.input('key', 'KP_Add')
        gui.wait_window('SelectMaskDialog')
        self.assertEqual(gui.window('SelectMaskDialog')['title'], 'Select')
        gui.set('SelectMaskDialog/MaskEdit', '*.txt')
        gui.click('SelectMaskDialog/OKBtn')
        gui.wait_no_window('SelectMaskDialog')
        gui.wait_for(lambda: selected('Remote') == ['a.txt', 'b.txt'], what='the files of the mask')
        # Num* inverts, Ctrl+Shift+L clears, Ctrl+Shift+S restores the
        # selection of the last operation
        self.session.input('key', 'KP_Multiply')
        gui.wait_for(lambda: selected('Remote') == ['c.log', 'd.log'], what='the inverted selection')
        gui.trigger(W + 'CurrentPropertiesAction')
        gui.wait_window('PropertiesDialog')
        gui.click('PropertiesDialog/CancelButton')
        gui.wait_no_window('PropertiesDialog')
        self.session.input('key', 'ctrl+shift+l')
        gui.wait_for(lambda: selected('Remote') == [], what='no selection')
        self.session.input('key', 'ctrl+shift+s')
        gui.wait_for(lambda: selected('Remote') == ['c.log', 'd.log'], what='the restored selection')
        gui.trigger(W + 'ClearSelectionAction')

        # The keyboard moves the focus without selecting, Insert selects
        gui.select(W + 'RemoteDirView', 'a.txt')
        gui.trigger(W + 'ClearSelectionAction')
        gui.wait_idle()
        self.session.input('key', 'Down')
        gui.wait_for(lambda: current('Remote') == ['b.txt'], what='the focus')
        self.assertEqual(selected('Remote'), [])
        self.session.input('key', 'Insert', 'key', 'Insert')
        gui.wait_for(lambda: selected('Remote') == ['b.txt', 'c.log'], what='the files selected by Insert')
        self.assertEqual(current('Remote'), ['d.log'])
        self.screenshot('norton_like')
        # Alt+Num- unselects the files of the same extension
        self.session.input('key', 'alt+KP_Subtract')
        gui.wait_for(lambda: selected('Remote') == ['b.txt'], what='the files of other extensions')

        # The filter of the panel
        self.session.input('key', 'ctrl+alt+f')
        gui.wait_window('SelectMaskDialog')
        self.assertEqual(gui.window('SelectMaskDialog')['title'], 'Filter')
        gui.set('SelectMaskDialog/MaskEdit', '*.log')
        gui.click('SelectMaskDialog/OKBtn')
        gui.wait_no_window('SelectMaskDialog')
        gui.wait_for(lambda: gui.names(W + 'RemoteDirView') == ['..', 'c.log', 'd.log'], what='the filtered files')
        self.assertTrue(gui.get(W + 'RemoteFilteredFilesLabel')['visible'])
        self.assertEqual(gui.get(W + 'RemoteFilteredFilesLabel')['text'], '2 filtered')
        gui.trigger(W + 'RemoteFilterAction')
        gui.wait_window('SelectMaskDialog')
        gui.click('SelectMaskDialog/ClearButton')
        gui.click('SelectMaskDialog/OKBtn')
        gui.wait_for(lambda: len(gui.names(W + 'RemoteDirView')) == 5, what='all the files')
        self.assertFalse(gui.get(W + 'RemoteFilteredFilesLabel')['visible'])

        # Comparing the directories marks the newer and the missing files
        gui.trigger(W + 'CompareDirectoriesAction2')
        gui.wait_for(lambda: selected('Remote') == ['c.log', 'd.log'], what='the remote differences')
        self.assertEqual(selected('Local'), ['b.txt'])

        # A color of the files
        R = 'PreferencesDialog/'
        gui.trigger(W + 'PreferencesAction')
        gui.wait_window('PreferencesDialog')
        gui.select(R + 'NavigationTree', ['Panels', 'File colors'])
        gui.click(R + 'AddFileColorButton')
        gui.wait_window('SelectMaskDialog')
        gui.set('SelectMaskDialog/MaskEdit', '*.log')
        gui.click('SelectMaskDialog/ColorButton')
        gui.wait_window('ColorMenu')
        self.screenshot('color_menu')
        gui.click('ColorMenu/Color0000FF')
        gui.wait_no_window('ColorMenu')
        gui.click('SelectMaskDialog/OKBtn')
        gui.wait_no_window('SelectMaskDialog')
        self.assertEqual(gui.names(R + 'FileColorsView'), ['*.log'])
        # The Commander's preferences
        gui.select(R + 'NavigationTree', ['Environment', 'Commander'])
        gui.set(R + 'SwappedPanelsCheck', True)
        gui.click(R + 'OKButton')
        gui.wait_no_window('PreferencesDialog')
        gui.wait_idle()
        ini = gui.config_file.read_text()
        self.assertIn('FileColors=0000FF:*.log', ini)
        self.assertIn('SwappedPanels=1', ini)
        # The remote panel on the left
        self.assertLess(gui.get(W + 'RemotePanel')['geometry']['x'], gui.get(W + 'LocalPanel')['geometry']['x'])
        self.screenshot('file_colors')
        self.assertEqual(gui.quit(), 0)

    def test_site_advanced(self):
        remote = self.work_dir('remote')
        (remote / 'sub').mkdir()
        (remote / 'sub' / 'inner.txt').write_text('inner')
        local = self.work_dir('local')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 'local')
        # The advanced settings of a stored site are edited with it
        self.assertFalse(gui.get('LoginDialog/AdvancedButton')['enabled'])
        gui.click('LoginDialog/EditButton')
        gui.click('LoginDialog/AdvancedButton')
        gui.wait_window('SiteAdvancedDialog')
        A = 'SiteAdvancedDialog/'
        tree = gui.names(A + 'NavigationTree')
        self.assertEqual(tree, ['Environment', 'Connection', 'SSH', 'Note'])
        gui.select(A + 'NavigationTree', ['Environment', 'Directories'])
        self.assertEqual(gui.get(A + 'RemoteDirectoryEdit')['text'], str(remote))
        gui.set(A + 'RemoteDirectoryEdit', str(remote / 'sub'))
        gui.set(A + 'UpdateDirectoriesCheck', False)
        gui.select(A + 'NavigationTree', 'Note')
        gui.set(A + 'NoteMemo', 'The test site')
        # Popups need real input on Wayland
        gui.click_real(A + 'ColorButton')
        gui.wait_window('ColorMenu')
        gui.click('ColorMenu/ColorCC99FF')
        gui.wait_no_window('ColorMenu')
        self.screenshot('site_advanced')
        gui.click(A + 'OKBtn')
        gui.wait_no_window('SiteAdvancedDialog')
        # The note of the site
        gui.wait_for(lambda: gui.get('LoginDialog/NoteGroup')['visible'], what='the note')
        self.assertEqual(gui.get('LoginDialog/NoteMemo')['text'], 'The test site')
        gui.click('LoginDialog/SaveButton')
        gui.wait_idle()
        ini = gui.config_file.read_text()
        self.assertIn('RemoteDirectory=%s\n' % (remote / 'sub'), ini)
        self.assertIn('Color=13408767', ini)
        self.screenshot('site_color')

        self.login(gui, 'local')
        self.assertEqual(gui.get(W + 'RemotePathComboBox')['text'], str(remote / 'sub'))
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'inner.txt'])
        self.assertEqual(gui.quit(), 0)

    def test_panel_layout(self):
        remote = self.work_dir('remote')
        (remote / 'one').mkdir()
        (remote / 'one' / 'deep').mkdir()
        (remote / 'two').mkdir()
        (remote / 'big.bin').write_bytes(b'x' * 3000)
        (remote / 'small.bin').write_bytes(b'x' * 10)
        local = self.work_dir('local')
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')

        # The directory tree of the remote panel shows the visited
        # directories and those in the cache
        self.assertFalse(gui.get(W + 'RemoteDriveView')['visible'])
        gui.trigger(W + 'RemoteTreeAction')
        gui.wait_for(lambda: gui.get(W + 'RemoteDriveView')['visible'], what='the remote tree')
        def tree():
            return gui.names(W + 'RemoteDriveView')
        self.assertEqual(tree(), ['/'])
        self.screenshot('remote_tree')
        gui.activate(W + 'RemoteDirView', 'one')
        gui.wait_for(lambda: gui.get(W + 'RemotePathComboBox')['text'] == str(remote / 'one'), what='the directory')
        # A directory of the tree is opened
        gui.select(W + 'RemoteDriveView', ['/'] + list(remote.parts[1:]))
        gui.wait_for(lambda: gui.get(W + 'RemotePathComboBox')['text'] == str(remote), what='the directory of the tree')

        # Sorted by size, the extension column shown
        gui.focus(W + 'RemoteDirView')
        gui.trigger(W + 'RemoteSortBySizeAction2')
        gui.wait_for(lambda: gui.names(W + 'RemoteDirView')[-2:] == ['small.bin', 'big.bin'], what='the sort by size')
        gui.trigger(W + 'RemoteStatusBarAction2')
        gui.wait_for(lambda: not gui.get(W + 'RemoteStatusBar')['visible'], what='no status bar')
        self.assertEqual(gui.quit(), 0)

        ini = gui.config_file.read_text()
        section = ini[ini.index('Interface\\Commander\\RemotePanel]'):]
        self.assertIn('DriveView=1', section)
        self.assertIn('StatusBar=0', section)
        self.assertIn('DirViewParams=1;1;0|', section)

        # Restored with the next start
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local', accept_host_key=False)
        self.assertTrue(gui.get(W + 'RemoteDriveView')['visible'])
        self.assertFalse(gui.get(W + 'RemoteStatusBar')['visible'])
        self.assertEqual(gui.names(W + 'RemoteDirView')[-2:], ['small.bin', 'big.bin'])
        self.assertEqual(gui.quit(), 0)

    def test_toolbars(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'up.txt').write_text('uploaded')
        ini = site_ini(remote=remote, local=local) + (
            '[Configuration\\Interface\\Commander]\nToolbarsButtons=Commands=Console:0\n')
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local')

        # upstream's default layout: the commands, the preferences and the
        # transfer settings, the toolbars of the panels; the others hidden
        for name in ('CommandsToolbar', 'PreferencesToolbar', 'TransferToolbar', 'LocalPathToolbar',
                     'LocalHistoryToolbar', 'LocalNavigationToolbar', 'LocalFileToolbar', 'LocalSelectionToolbar',
                     'RemoteFileToolbar', 'RemoteSelectionToolbar'):
            self.assertTrue(gui.get(W + name)['visible'], name)
        for name in ('SessionToolbar2', 'SortToolbar', 'CustomCommandsToolbar', 'Toolbar2Toolbar'):
            self.assertFalse(gui.get(W + name)['visible'], name)
        commands = [child['name'] for child in gui.get(W + 'CommandsToolbar', children=True)['children']]
        self.assertIn('FullSynchronizeAction2', commands)
        self.assertIn('PuttyAction', commands)
        # Hidden by the configuration
        self.assertNotIn('ConsoleAction', commands)
        self.assertEqual(gui.get(W + 'RemoteNavigationToolbar/RemoteFindFilesAction2')['text'], 'Find Files')
        self.assertEqual(gui.get(W + 'CommandsToolbar/FullSynchronizeAction2')['text'], 'Synchronize')
        # The layout of the rows of a panel
        path = gui.get(W + 'LocalPathToolbar')['geometry']
        history = gui.get(W + 'LocalHistoryToolbar')['geometry']
        files = gui.get(W + 'LocalFileToolbar')['geometry']
        self.assertEqual(path['y'], history['y'])
        self.assertLess(path['x'], history['x'])
        self.assertGreater(files['y'], path['y'])
        self.screenshot('toolbars')

        # Uploaded by the button of the local panel's toolbar
        self.select(gui, 'Local', 'up.txt')
        gui.focus(W + 'RemoteDirView')
        gui.wait_idle()
        gui.click(W + 'LocalFileToolbar/LocalCopyAction')
        self.confirm_copy(gui, 'Upload file \'up.txt\' to remote directory:')
        gui.wait_for(lambda: (remote / 'up.txt').exists(), 30, 'the upload')

        # Shown and hidden by upstream's actions
        gui.trigger(W + 'CommanderSortBandAction')
        self.assertTrue(gui.get(W + 'SortToolbar')['visible'])
        gui.trigger(W + 'CommanderLocalFileBandAction2')
        self.assertFalse(gui.get(W + 'LocalFileToolbar')['visible'])
        gui.trigger(W + 'ToolBar2Action')
        self.assertTrue(gui.get(W + 'Toolbar2Toolbar')['visible'])
        gui.trigger(W + 'LockToolbarsAction')

        # A button hidden in the popup menu of the toolbar
        gui.session.input('move', *gui.screen_point(W + 'CommandsToolbar/PuttyAction'), 'click', 'right')
        gui.wait_window('CommanderBarPopup')
        self.assertTrue(gui.get('CommanderBarPopup/CustomizeToolbarMenu/PuttyCustomizeItem')['checked'])
        gui.trigger('CommanderBarPopup/CustomizeToolbarMenu/PuttyCustomizeItem')
        gui.key('Escape', 'CommanderBarPopup')
        gui.wait_no_window('CommanderBarPopup')
        commands = [child['name'] for child in gui.get(W + 'CommandsToolbar', children=True)['children']]
        self.assertNotIn('PuttyAction', commands)

        # Kept in upstream's format
        self.assertEqual(gui.quit(), 0)
        ini = gui.config_file.read_text()
        self.assertIn('Sort=1:TopDock:', ini)
        self.assertIn('LocalFile=0:LocalTopDock:1+0', ini)
        self.assertIn('Toolbar2=1:BottomDock:', ini)
        self.assertIn('ToolbarsButtons=Commands=Console:0,Putty:0', ini)
        self.assertIn('LockToolbars=1', ini)

        # And restored
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local', accept_host_key=False)
        self.assertTrue(gui.get(W + 'SortToolbar')['visible'])
        self.assertFalse(gui.get(W + 'LocalFileToolbar')['visible'])
        self.assertTrue(gui.get(W + 'LockToolbarsAction')['checked'])
        self.assertEqual(gui.quit(), 0)

    def test_explorer(self):
        remote = self.work_dir('remote')
        (remote / 'sub').mkdir()
        (remote / 'file.txt').write_text('content')
        local = self.work_dir('local')
        ini = site_ini(remote=remote) + (
            '[Configuration\\Interface]\nInterface=1\n'
            '[Configuration\\Interface\\Explorer]\nLastLocalTargetDirectory=%s\n' % local)
        gui = self.start_gui(ini=ini)
        self.login(gui, 'local')

        # Only the remote panel, with its tree on the left
        self.assertFalse(gui.get(W + 'LocalPanel')['visible'])
        self.assertTrue(gui.get(W + 'RemoteDriveView')['visible'])
        tree = gui.get(W + 'RemoteDriveView')['geometry']
        view = gui.get(W + 'RemoteDirView')['geometry']
        self.assertLess(tree['x'], view['x'])
        # The Explorer's toolbars
        self.assertTrue(gui.get(W + 'ButtonsToolbar')['visible'])
        self.assertTrue(gui.get(W + 'TransferToolbar')['visible'])
        menus = [menu['name'] for menu in gui.get(W + 'MenuToolbar')['menus']]
        self.assertEqual(menus, ['ExplorerFilesMenu', 'ExplorerCommandsMenu', 'MarkMenu', 'TabsMenu', 'ViewMenu',
                                 'HelpMenu'])
        self.assertEqual(gui.get(W + 'CurrentCopyAction')['shortcut'], 'Ctrl+T')
        self.screenshot('explorer')

        # Downloaded to the last local directory
        self.select(gui, 'Remote', 'file.txt')
        gui.trigger(W + 'CurrentCopyAction')
        self.confirm_copy(gui, 'Download file \'file.txt\' to local directory:')
        gui.wait_for(lambda: (local / 'file.txt').exists(), 30, 'the download')

        # Back to the Commander
        R = 'PreferencesDialog/'
        gui.trigger(W + 'PreferencesAction')
        gui.wait_window('PreferencesDialog')
        gui.select(R + 'NavigationTree', ['Environment', 'Interface'])
        self.assertTrue(gui.get(R + 'ExplorerInterfaceButton2')['checked'])
        gui.set(R + 'CommanderInterfaceButton2', True)
        gui.click(R + 'OKButton')
        gui.wait_no_window('PreferencesDialog')
        gui.wait_for(lambda: gui.get(W + 'LocalPanel')['visible'], what='the local panel')
        # The Commander's, the hot keys hidden by default (as upstream)
        self.assertTrue(gui.get(W + 'CommandsToolbar')['visible'])
        self.assertFalse(gui.get(W + 'Toolbar2Toolbar')['visible'])
        self.assertEqual(gui.get(W + 'CurrentCopyAction')['shortcut'], 'F5')
        menus = [menu['name'] for menu in gui.get(W + 'MenuToolbar')['menus']]
        self.assertEqual(menus[0], 'LocalMenuButton')
        self.assertEqual(gui.quit(), 0)
        self.assertIn('Interface=0', gui.config_file.read_text())

    def test_import_sites(self):
        home = self.session.home
        putty = home / '.putty'
        (putty / 'sessions').mkdir(parents=True)
        self.addCleanup(shutil.rmtree, putty, True)
        (putty / 'sessions' / 'my%20server').write_text(
            'HostName=putty.example.com\nPortNumber=2222\nUserName=alice\nProtocol=ssh\n')
        (putty / 'sshhostkeys').write_text('ssh-ed25519@2222:putty.example.com 0x1234,0x5678\n')
        ssh = home / '.ssh'
        ssh.mkdir(exist_ok=True)
        for name in ['config', 'known_hosts']:
            self.addCleanup(lambda path=ssh / name: path.unlink(missing_ok=True))
        (ssh / 'config').write_text('Host box\n  HostName box.example.com\n  User bob\n  Port 2200\n')
        key = self.work_dir('key') / 'key'
        subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-f', str(key)], check=True)
        public_key = ' '.join(key.with_suffix('.pub').read_text().split()[:2])
        (ssh / 'known_hosts').write_text('known.example.com %s\n' % public_key)

        gui = self.start_gui()
        gui.wait_window('LoginDialog')
        D = 'ImportSessionsDialog/'

        def import_from(source, expected):
            gui.trigger('LoginDialog/ImportSessionsAction')
            gui.wait_window('ImportSessionsDialog')
            gui.set(D + 'SourceComboBox', source)
            gui.wait_for(lambda: gui.names(D + 'SessionListView2') == expected, what='the sites of %s' % source)
            rows = gui.get(D + 'SessionListView2')['rows']
            self.assertTrue(all(row['checked'] for row in rows))
            self.screenshot('import_' + source)
            gui.click(D + 'OKButton')
            gui.wait_no_window('ImportSessionsDialog')
            gui.wait_idle()

        # The first source with sites is offered
        gui.trigger('LoginDialog/ImportSessionsAction')
        gui.wait_window('ImportSessionsDialog')
        self.assertEqual(gui.get(D + 'SourceComboBox')['text'], 'PuTTY')
        gui.click(D + 'CancelButton')
        gui.wait_no_window('ImportSessionsDialog')

        import_from('PuTTY', ['my server'])
        selected = [row['cells'][0] for row in gui.get('LoginDialog/SessionTree')['rows'] if row['current']]
        self.assertEqual(selected, ['my server'])
        ini = gui.config_file.read_text()
        self.assertIn('[Sessions\\my%20server]', ini)
        self.assertIn('HostName=putty.example.com', ini)
        self.assertIn('PortNumber=2222', ini)
        self.assertIn('ssh-ed25519@2222:putty.example.com=0x1234,0x5678', ini)

        import_from('OpenSSH', ['box'])
        ini = gui.config_file.read_text()
        self.assertIn('HostName=box.example.com', ini)
        self.assertIn('PortNumber=2200', ini)

        import_from('known_hosts', ['known.example.com'])
        self.assertIn('ssh-ed25519@22:known.example.com=', gui.config_file.read_text())

        # Nothing of FileZilla
        gui.trigger('LoginDialog/ImportSessionsAction')
        gui.wait_window('ImportSessionsDialog')
        gui.set(D + 'SourceComboBox', 'FileZilla')
        gui.wait_for(lambda: gui.get(D + 'ErrorLabel')['visible'], what='the error')
        self.assertIn('filezilla/sitemanager.xml', gui.get(D + 'ErrorLabel')['text'])
        self.assertFalse(gui.get(D + 'OKButton')['enabled'])
        gui.click(D + 'CancelButton')
        self.assertEqual(gui.quit(), 0)

    def test_session_info(self):
        remote = self.work_dir('remote')
        (remote / 'sub').mkdir()
        gui = self.start_gui(ini=site_ini(remote=remote))
        self.login(gui, 'local')
        F = 'FileSystemInfoDialog/'

        # The server and the protocol
        gui.trigger(W + 'FileSystemInfoAction')
        gui.wait_window('FileSystemInfoDialog')
        server = dict(gui.rows(F + 'ServerView'))
        self.assertEqual(server['File transfer protocol'], 'SFTP-3')
        self.assertEqual(server['Cryptographic protocol'], 'SSH-2')
        self.assertIn('OpenSSH', server['SSH implementation'])
        self.assertEqual(gui.get(F + 'HostKeyAlgorithmEdit')['text'], 'ssh-ed25519 255')
        result = subprocess.run(['ssh-keygen', '-l', '-f', str(SSHD.host_key)], check=True, capture_output=True,
                                text=True)
        fingerprint = result.stdout.split()[1].removeprefix('SHA256:')
        self.assertIn(fingerprint, gui.get(F + 'HostKeyFingerprintSHA256Edit')['text'])
        self.assertFalse(gui.get(F + 'CertificateGroup')['visible'])
        capabilities = dict(gui.rows(F + 'ProtocolView'))
        self.assertEqual(capabilities['Can check available space'], 'Yes')
        # The space available of the current directory (the statvfs
        # extension of OpenSSH)
        gui.set(F + 'PageControl', 2)
        gui.wait_for(lambda: dict(gui.rows(F + 'SpaceAvailableView'))['Total bytes on device'] != 'Unknown',
                     what='the space available')
        self.assertEqual(gui.get(F + 'SpaceAvailablePathEdit')['text'], str(remote))
        self.screenshot('fsinfo')
        gui.click(F + 'CloseButton')
        gui.wait_no_window('FileSystemInfoDialog')

        # The URL of the session and of a file
        G = 'GenerateUrlDialog/'
        gui.trigger(W + 'SessionGenerateUrlAction2')
        gui.wait_window('GenerateUrlDialog')
        gui.set(G + 'OptionsPageControl', 0)
        url = 'sftp://%s@127.0.0.1:%d%s/' % (SSHD.user, SSHD.port, remote)
        self.assertEqual(gui.get(G + 'ResultMemo')['text'], url)
        gui.set(G + 'OptionsPageControl', 1)
        gui.set(G + 'ScriptFormatCombo', 'Shell script')
        script = gui.get(G + 'ResultMemo')['text']
        self.assertTrue(script.startswith('#!/bin/sh'), script)
        self.assertIn('-hostkey=\\"ssh-ed25519 255 ', script)
        self.screenshot('generate_url')
        gui.click(G + 'CancelBtn')
        gui.wait_no_window('GenerateUrlDialog')
        self.select(gui, 'Remote', 'sub')
        gui.trigger(W + 'FileGenerateUrlAction2')
        gui.wait_window('GenerateUrlDialog')
        # A directory with the trailing slash
        self.assertEqual(gui.get(G + 'ResultMemo')['text'], url + 'sub/\n')
        gui.click(G + 'CancelBtn')
        gui.wait_no_window('GenerateUrlDialog')

        # The caches are cleared
        gui.activate(W + 'RemoteDirView', 'sub')
        gui.wait_for(lambda: gui.get(W + 'RemotePathComboBox')['text'] == str(remote / 'sub'), what='the subdirectory')
        gui.wait_idle()
        self.assertTrue(gui.get(W + 'ClearCachesAction')['enabled'])
        gui.trigger(W + 'ClearCachesAction')
        gui.wait_for(lambda: not gui.get(W + 'ClearCachesAction')['enabled'], what='the empty caches')

        # The color of the tab is saved with the session (popups need real
        # input on Wayland)
        self.open_menu(gui, 'TabsMenu')
        gui.trigger(W + 'ColorMenuAction2')
        gui.wait_window('ColorMenu')
        gui.click('ColorMenu/ColorCC99FF')
        gui.wait_no_window('ColorMenu')
        self.screenshot('tab_color')
        gui.trigger(W + 'SaveCurrentSessionAction2')
        gui.wait_window('SaveSessionDialog')
        gui.set('SaveSessionDialog/SessionNameEdit', 'colored')
        gui.click('SaveSessionDialog/OKButton')
        gui.wait_no_window('SaveSessionDialog')
        gui.wait_idle()
        ini = gui.config_file.read_text()
        section = ini[ini.index('[Sessions\\colored]'):]
        self.assertIn('Color=13408767', section)
        self.assertEqual(gui.quit(), 0)

    def test_links_and_sizes(self):
        remote = self.work_dir('remote')
        (remote / 'a.txt').write_text('a')
        (remote / 'b.txt').write_text('bb')
        (remote / 'dir').mkdir()
        (remote / 'dir' / 'c.bin').write_bytes(b'x' * 3000)
        local = self.work_dir('local')
        (local / 'l.txt').write_text('l')
        (local / 'ldir').mkdir()
        (local / 'ldir' / 'd.bin').write_bytes(b'y' * 5000)
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')
        S = 'SymlinkDialog/'

        def create_link(side, focused, name, hard_link_enabled):
            self.select(gui, side, focused)
            gui.trigger(W + 'NewLinkAction')
            gui.wait_window('SymlinkDialog')
            self.assertEqual(gui.get(S + 'PointToEdit')['text'], focused)
            self.assertEqual(gui.get(S + 'HardLinkCheck')['enabled'], hard_link_enabled)
            gui.set(S + 'FileNameEdit', name)
            gui.click(S + 'OkButton')
            gui.wait_no_window('SymlinkDialog')
            gui.wait_for(lambda: name in gui.names(W + side + 'DirView'), what='the link %s' % name)

        def edit_link(side, name, target):
            self.select(gui, side, name)
            gui.trigger(W + 'CurrentAddEditLinkAction')
            gui.wait_window('SymlinkDialog')
            self.assertEqual(gui.get(S + 'FileNameEdit')['text'], name)
            gui.set(S + 'PointToEdit', target)
            gui.click(S + 'OkButton')
            gui.wait_no_window('SymlinkDialog')
            gui.wait_idle()

        # A remote link to the focused file, edited
        create_link('Remote', 'a.txt', 'link', True)
        self.assertEqual(os.readlink(remote / 'link'), 'a.txt')
        edit_link('Remote', 'link', 'b.txt')
        gui.wait_for(lambda: os.readlink(remote / 'link') == 'b.txt', what='the edited remote link')
        # A local one
        create_link('Local', 'l.txt', 'llink', True)
        self.assertEqual(os.readlink(local / 'llink'), 'l.txt')
        edit_link('Local', 'llink', 'ldir')
        self.assertEqual(os.readlink(local / 'llink'), 'ldir')

        # The sizes of the directories
        def size_of(side, name):
            return next(cells[1] for cells in gui.rows(W + side + 'DirView') if cells[0] == name)

        for side, name in [('Remote', 'dir'), ('Local', 'ldir')]:
            self.assertEqual(size_of(side, name), '')
            self.select(gui, side, name)
            gui.trigger(W + 'CalculateDirectorySizesAction')
            gui.wait_for(lambda: size_of(side, name) != '', what='the size of %s' % name)

        # The popup menus of the files and of the directory (popups need
        # real input on Wayland); a file clicked is selected
        self.select(gui, 'Remote', 'a.txt')
        x, y = gui.row_point(W + 'RemoteDirView', 'b.txt')
        self.session.input('move', x, y, 'click', 'right')
        gui.wait_window('RemoteFilePopup')
        self.screenshot('file_popup')
        items = gui.get('RemoteFilePopup')['items']
        self.assertEqual(items[:3], ['Open', 'Edit', 'Copy...'])
        self.assertIn('Properties', items)
        self.assertEqual([row['cells'][0] for row in gui.get(W + 'RemoteDirView')['rows'] if row['selected']],
                         ['b.txt'])
        self.session.input('key', 'escape')
        gui.wait_no_window('RemoteFilePopup')
        geometry = gui.get(W + 'LocalDirView')['geometry']
        x0, y0 = gui.client_origin('ScpCommanderForm')
        self.session.input('move', x0 + geometry['x'] + 40, y0 + geometry['y'] + geometry['height'] - 20, 'click', 'right')
        gui.wait_window('LocalDirViewPopup')
        items = gui.get('LocalDirViewPopup')['items']
        self.assertEqual(items[:2], ['Go To', 'Refresh'])
        self.assertIn('Paste from Clipboard', items)
        self.session.input('key', 'escape')
        gui.wait_no_window('LocalDirViewPopup')
        self.assertEqual(gui.quit(), 0)

    def test_synchronize_browsing(self):
        remote = self.work_dir('remote')
        (remote / 'sub' / 'deep').mkdir(parents=True)
        (remote / 'other').mkdir()
        local = self.work_dir('local')
        (local / 'sub').mkdir()
        gui = self.start_gui(ini=site_ini(remote=remote, local=local))
        self.login(gui, 'local')
        remote_path = W + 'RemotePathComboBox'
        local_path = W + 'LocalPathComboBox'
        gui.trigger(W + 'SynchronizeBrowsingAction2')
        self.assertTrue(gui.get(W + 'SynchronizeBrowsingAction2')['checked'])

        # The remote panel follows the local one
        gui.activate(W + 'LocalDirView', 'sub')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(remote / 'sub'), what='the remote directory')
        self.assertEqual(gui.get(local_path)['text'], str(local / 'sub'))
        # and the local one the remote one, a missing directory is created
        # when confirmed
        gui.activate(W + 'RemoteDirView', 'deep')
        gui.wait_window('MessageForm')
        self.assertIn(str(local / 'sub' / 'deep'), self.message_text(gui))
        gui.click('MessageForm/Yes')
        gui.wait_for(lambda: gui.get(local_path)['text'] == str(local / 'sub' / 'deep'), what='the created directory')
        self.assertTrue((local / 'sub' / 'deep').is_dir())
        gui.activate(W + 'LocalDirView', '..')
        gui.wait_for(lambda: gui.get(remote_path)['text'] == str(remote / 'sub'), what='the parent directory')
        gui.activate(W + 'RemoteDirView', '..')
        gui.wait_for(lambda: gui.get(local_path)['text'] == str(local), what='the local parent directory')
        gui.wait_idle()

        # Not created, the browsing is not synchronized anymore
        gui.activate(W + 'RemoteDirView', 'other')
        gui.wait_window('MessageForm')
        gui.click('MessageForm/No')
        gui.wait_no_window('MessageForm')
        gui.wait_for(lambda: not gui.get(W + 'SynchronizeBrowsingAction2')['checked'], what='the browsing not synchronized')
        self.assertEqual(gui.get(remote_path)['text'], str(remote / 'other'))
        self.assertEqual(gui.get(local_path)['text'], str(local))
        self.assertFalse((local / 'other').exists())
        self.assertEqual(gui.quit(), 0)

    def test_configuration_backup(self):
        gui = self.start_gui(ini=site_ini('first'))
        gui.wait_window('LoginDialog')
        backup = self.work_dir('backup') / 'backup.ini'

        # Exported (backed up)
        gui.trigger('LoginDialog/ExportAction')
        gui.wait_window('ExportDialog')
        gui.set('ExportDialog/fileNameEdit', str(backup))
        gui.key('Return', 'ExportDialog/fileNameEdit')
        gui.wait_no_window('ExportDialog')
        gui.wait_for(lambda: backup.exists(), what='the backup')
        self.assertIn('[Sessions\\first]', backup.read_text())

        # Restoring it is confirmed (it restarts the application)
        gui.trigger('LoginDialog/ImportAction')
        gui.wait_window('MessageForm')
        self.assertIn('Restore configuration from a backup?', self.message_text(gui))
        gui.click('MessageForm/OK')
        gui.wait_window('ImportDialog')
        gui.key('Escape', 'ImportDialog/fileNameEdit')
        gui.wait_no_window('ImportDialog')

        # The configuration file cleaned up is not saved again
        gui.trigger('LoginDialog/CleanUpAction')
        gui.wait_window('CleanupDialog')
        rows = gui.rows('CleanupDialog/DataListView')
        self.assertEqual(rows[0], ['Configuration INI file', str(gui.config_file)])
        self.assertFalse(gui.get('CleanupDialog/OKButton')['enabled'])
        gui.check('CleanupDialog/DataListView', 'Configuration INI file')
        gui.click('CleanupDialog/OKButton')
        gui.wait_no_window('CleanupDialog')
        self.assertFalse(gui.config_file.exists())
        gui.click('LoginDialog/CloseButton')
        self.assertEqual(gui.quit(), 0)
        self.assertFalse(gui.config_file.exists())

    def test_translation(self):
        remote = self.work_dir('remote')
        (remote / 'a.txt').write_text('a')
        # The Polish translation (the language of the system by default)
        gui = self.start_gui(ini=site_ini(remote=remote) + '[Configuration\\Interface]\nLanguage=PL\n')
        gui.wait_window('LoginDialog')
        self.assertEqual(gui.window('LoginDialog')['title'], 'Logowanie')
        self.assertEqual(gui.get('LoginDialog/LoginButton')['text'], 'Logowanie')
        self.screenshot('login')
        gui.select('LoginDialog/SessionTree', 'local')
        gui.click('LoginDialog/LoginButton')
        # The resource strings
        gui.wait_window('MessageForm')
        self.assertIn('nieznanym serwerem', self.message_text(gui))
        self.assertEqual(gui.get('MessageForm/Cancel')['text'], 'Anuluj')
        gui.click('MessageForm/Yes')
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        gui.wait_idle()
        menus = [item['text'] for item in gui.get(W + 'MenuToolbar')['menus']]
        self.assertIn('Pliki', menus)
        self.screenshot('commander')
        self.assertEqual(gui.quit(), 0)

    def test_about(self):
        gui = self.start_gui(ini=site_ini())
        gui.wait_window('LoginDialog')
        gui.trigger('LoginDialog/AboutAction')
        gui.wait_window('AboutDialog')
        self.assertEqual(gui.get('AboutDialog/ApplicationLabel')['text'], 'OpenSCP')
        self.screenshot('about')
        gui.click('AboutDialog/OKButton')
        gui.wait_no_window('AboutDialog')
        self.login(gui, 'local')
        # The Help menu of both interfaces
        menus = [item['name'] for item in gui.get(W + 'MenuToolbar')['menus']]
        self.assertEqual(menus[-1], 'HelpMenu')
        gui.trigger(W + 'AboutAction')
        gui.wait_window('AboutDialog')
        gui.click('AboutDialog/OKButton')
        gui.wait_no_window('AboutDialog')
        self.assertEqual(gui.quit(), 0)

    def test_connection_refused(self):
        with __import__('socket').socket() as probe:
            probe.bind(('127.0.0.1', 0))
            port = probe.getsockname()[1]
        gui = self.start_gui(ini=site_ini(port=port))
        gui.wait_window('LoginDialog')
        gui.select('LoginDialog/SessionTree', 'local')
        gui.click('LoginDialog/LoginButton')
        gui.wait_window('MessageForm')
        message = gui.get('MessageForm', children=True)
        texts = ' '.join(child.get('text', '') for child in message['children'])
        self.assertIn('Connection refused', texts)
        # Reconnecting automatically after a while (SessionReopenAutoIdle)
        self.assertRegex(gui.get('MessageForm/Retry')['text'], r'^&Reconnect \(\d s\)$')
        self.screenshot('refused')
        gui.click('MessageForm/OK')
        # The site manager again
        gui.wait_window('LoginDialog')
        gui.click('LoginDialog/CloseButton')
        gui.wait_no_window('LoginDialog')
        self.assertTrue(gui.get(W + 'RemoteNoSessionPage')['visible'])
        self.assertEqual(gui.quit(), 0)


if __name__ == '__main__':
    unittest.main()
