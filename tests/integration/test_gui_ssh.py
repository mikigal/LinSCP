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


class GuiSshTest(GuiTestCase):
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
        self.assertTrue(gui.window('ScpCommanderForm')['title'].startswith('agent site'))
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

        # The host key is cached: no prompt the next time
        gui.trigger(W + 'DisconnectSessionAction')
        gui.wait_for(lambda: gui.get(W + 'RemoteNoSessionPage')['visible'], what='the disconnected panel')
        self.assertEqual(gui.window('ScpCommanderForm')['title'], 'OpenSCP')
        gui.click(W + 'RemoteNewSessionButton')
        gui.wait_window('LoginDialog')
        # The last session's site is selected
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

        # The defaults, a new command with a shortcut
        L = 'CustomCommandsDialog/'
        D = 'CustomCommandDialog/'
        gui.trigger(W + 'CustomCommandsCustomizeAction')
        gui.wait_window('CustomCommandsDialog')
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
        gui.wait_no_window('CustomCommandsDialog')

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
        self.assertEqual(gui.get('MessageForm/Retry')['text'], '&Reconnect')
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
