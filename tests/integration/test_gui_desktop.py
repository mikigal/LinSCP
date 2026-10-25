"""End-to-end tests of the desktop integration of the GUI (milestone M7):
drag and drop between the panels and with Dolphin, the file clipboard.

SFTP against the local test sshd, as test_gui_ssh (whose setup this module
shares): the remote side is the local file system. Dolphin runs in the same
headless KWin session; its files are found over AT-SPI.
"""

import json
import os
import sys
import time
import unittest
from pathlib import Path
from urllib.parse import unquote, urlparse

import desktop_services
import guitests
import test_gui_ssh as ssh
import wlharness
from dolphin import Dolphin
from openscp_gui import LSAN_SUPPRESSIONS, OpenScpGui

W = 'ScpCommanderForm/'
SOURCE = Path(__file__).resolve().parents[2]
# Downloads for other applications go to a temporary directory
TEMPORARY_MODE_INI = '[Configuration\\Interface]\nDDFakeFile=0\n'
# The application closes once the operation of its command line is done
EXIT_ON_COMPLETION_INI = '[Configuration\\Interface]\nConfirmExitOnCompletion=0\n'


def session_url(path=''):
    return 'sftp://%s@127.0.0.1:%d%s' % (ssh.SSHD.user, ssh.SSHD.port, path)


def key_switch():
    return '/privatekey=%s' % ssh.key_file()


def setUpModule():
    ssh.setUpModule()


def tearDownModule():
    ssh.tearDownModule()


class GuiDesktopTest(ssh.GuiSshTestCase):
    width = 1600
    height = 900

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.session.start_accessibility()

    def start_commander(self, remote, local, ini=''):
        gui = self.start_gui(ini=ssh.site_ini(remote=remote, local=local) + ini)
        self.login(gui, 'local')
        return gui

    def start_probe(self, title, *args):
        """wlh-probe, a Qt application that logs what is dropped on it."""
        log = self.session.logs / ('%s.events' % title)
        self.session.spawn([wlharness._default_tool('wlh-probe'), '--title', title, '--log', str(log), *args], log_name=title)
        self.session.wait_for_window(title)
        return log

    def start_desktop_services(self, logind=False):
        """The notification server, the tray and the task manager of a desktop
        shell (tools/testenv/desktop_services.py), with logind on the system
        bus of the session."""
        log = self.session.logs / ('%s-desktop.events' % self.test_name)
        args = []
        if logind:
            self.session.start_system_bus()
            args.append('--logind')
        process = self.session.spawn([sys.executable, desktop_services.__file__, '--log', str(log), *args],
                                     log_name='%s-desktop' % self.test_name)

        def stop():
            process.terminate()
            process.wait(10)

        self.addCleanup(stop)
        wlharness._wait_for(lambda: self.probe_events(log, 'ready'), 20, 'the desktop services')
        return log

    def desktop_call(self, method, *args):
        self.session.run(['gdbus', 'call', '--session', '--dest', 'org.openscp.TestDesktop',
                          '--object-path', '/org/openscp/TestDesktop', '--method', 'org.openscp.TestDesktop.' + method,
                          *map(str, args)])

    @staticmethod
    def probe_events(log, name):
        if not log.exists():
            return []
        events = [json.loads(line) for line in log.read_text().splitlines() if line.strip()]
        return [event for event in events if event['event'] == name]

    def place_commander(self, gui, x, y, width, height):
        title = gui.window('ScpCommanderForm')['title']
        client = self.session.place_window(title, x, y, width, height)['client']
        gui.wait_for(lambda: gui.get('ScpCommanderForm')['geometry']['width'] == client['width'], what='the placed window')

    def start_dolphin(self, directory, env=None):
        dolphin = Dolphin(self.session, directory, log_name='%s-dolphin' % self.test_name, env=env).start()
        self.addCleanup(dolphin.stop)
        dolphin.place(820, 0, 780, 860)
        return dolphin

    def test_drag_between_panels(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'up.txt').write_text('uploaded')
        (remote / 'down.txt').write_text('downloaded')
        (remote / 'sub').mkdir()
        log = self.start_desktop_services()
        gui = self.start_commander(remote, local)

        # Upload: the copy dialog of drag and drop has "do not show again"
        # checked
        gui.drag_real(gui.row_point(W + 'LocalDirView', 'up.txt'), gui.screen_point(W + 'RemoteDirView'))
        gui.wait_window('CopyDialog')
        self.assertTrue(gui.get('CopyDialog/NeverShowAgainCheck')['checked'])
        self.assertEqual(gui.get('CopyDialog/RemoteDirectoryEdit')['text'], str(remote) + '/*.*')
        self.screenshot('upload')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'up.txt').read_text(), 'uploaded')
        self.assertIn('up.txt', gui.names(W + 'RemoteDirView'))
        # Which a notification tells (a click turns the dialog on again)
        gui.wait_for(lambda: self.probe_events(log, 'notify'), what='the notification')
        notification = self.probe_events(log, 'notify')[0]
        self.assertEqual(notification['summary'], 'Transfer confirmation turned off')
        self.assertIn('Click here to undo.', notification['body'])

        # Download without the dialog now
        gui.drag_real(gui.row_point(W + 'RemoteDirView', 'down.txt'), gui.screen_point(W + 'LocalDirView'))
        gui.wait_for(lambda: (local / 'down.txt').exists(), what='the download')
        gui.wait_idle()
        self.assertIsNone(gui.window('CopyDialog'))
        self.assertEqual((local / 'down.txt').read_text(), 'downloaded')

        # Within the remote panel onto a directory: moved
        gui.drag_real(gui.row_point(W + 'RemoteDirView', 'down.txt'), gui.row_point(W + 'RemoteDirView', 'sub'))
        gui.wait_for(lambda: (remote / 'sub' / 'down.txt').exists(), what='the move')
        gui.wait_idle()
        self.assertFalse((remote / 'down.txt').exists())

        self.assertEqual(gui.quit(), 0)
        self.assertIn('DDTransferConfirmation2=1', gui.config_file.read_text())

    def test_drop_from_file_manager(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        source = self.work_dir('dolphin')
        (source / 'from-dolphin.txt').write_text('from the file manager')
        gui = self.start_commander(remote, local)
        self.place_commander(gui, 0, 0, 800, 860)
        dolphin = self.start_dolphin(source)

        gui.drag_real(dolphin.item_point('from-dolphin.txt'), gui.screen_point(W + 'RemoteDirView'))
        gui.wait_window('CopyDialog')
        self.screenshot('drop')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((remote / 'from-dolphin.txt').read_text(), 'from the file manager')
        # Copied, not moved
        self.assertTrue((source / 'from-dolphin.txt').exists())
        self.assertEqual(gui.quit(), 0)

    def test_drag_to_file_manager(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        target = self.work_dir('dolphin')
        (remote / 'remote.txt').write_text('to the file manager')
        gui = self.start_commander(remote, local)
        self.place_commander(gui, 0, 0, 800, 860)
        dolphin = self.start_dolphin(target)

        # Dolphin tells the directory of the drop (Ark's protocol); the
        # file is downloaded there
        gui.drag_real(gui.row_point(W + 'RemoteDirView', 'remote.txt'), dolphin.view_point())
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/LocalDirectoryEdit')['text'], str(target) + '/*.*')
        self.screenshot('drop')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((target / 'remote.txt').read_text(), 'to the file manager')
        self.assertTrue((remote / 'remote.txt').exists())
        dolphin.item_point('remote.txt')
        self.assertEqual(gui.quit(), 0)

    def test_drag_to_application_temporary(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (remote / 'remote.txt').write_text('through a temporary directory')
        gui = self.start_commander(remote, local, TEMPORARY_MODE_INI)
        self.place_commander(gui, 0, 0, 800, 860)
        log = self.start_probe('probe-target')
        probe = self.session.place_window('probe-target', 900, 100, 500, 400)['client']

        # The file is downloaded once the target asks for it (on the drop)
        gui.drag_real(gui.row_point(W + 'RemoteDirView', 'remote.txt'),
                      (probe['x'] + probe['width'] // 2, probe['y'] + probe['height'] // 2))
        gui.wait_for(lambda: self.probe_events(log, 'drop'), what='the drop')
        drop = self.probe_events(log, 'drop')[0]
        self.assertNotIn('application/vnd.portal.filetransfer', drop['formats'])
        self.assertEqual(len(drop['urls']), 1)
        path = Path(unquote(urlparse(drop['urls'][0]).path))
        self.assertEqual(path.name, 'remote.txt')
        self.assertEqual(path.read_text(), 'through a temporary directory')
        self.assertIsNone(gui.window('CopyDialog'))
        # The temporary directory is deleted (with the application at the
        # latest)
        self.assertEqual(gui.quit(), 0)
        self.assertFalse(path.parent.exists())

    def test_clipboard_with_file_manager(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        target = self.work_dir('dolphin')
        (remote / 'remote.txt').write_text('pasted to the file manager')
        (target / 'local.txt').write_text('pasted from the file manager')
        gui = self.start_commander(remote, local)
        self.place_commander(gui, 0, 0, 800, 860)
        dolphin = self.start_dolphin(target)

        # Copied in Dolphin, pasted to the remote panel: uploaded
        x, y = dolphin.item_point('local.txt')
        self.session.input('move', x, y, 'click', 'sleep', 200, 'key', 'ctrl+c')
        x, y = gui.screen_point(W + 'RemoteDirView')
        self.session.input('move', x, y, 'click', 'sleep', 300, 'key', 'ctrl+v')
        self.confirm_copy(gui, 'Upload file \'local.txt\' to remote directory:')
        self.assertEqual((remote / 'local.txt').read_text(), 'pasted from the file manager')

        # Copied in the remote panel, pasted in Dolphin: downloaded to its
        # directory, where the fake directory of the clipboard was pasted
        x, y = gui.row_point(W + 'RemoteDirView', 'remote.txt')
        self.session.input('move', x, y, 'click', 'sleep', 200, 'key', 'ctrl+c')
        x, y = dolphin.view_point()
        self.session.input('move', x, y, 'click', 'sleep', 300)
        self.assertEqual(dolphin.paste(), 'Paste One Folder')
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/LocalDirectoryEdit')['text'], str(target) + '/*.*')
        self.assertTrue(gui.get('CopyDialog/NeverShowAgainCheck')['checked'])
        self.screenshot('paste')
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_idle()
        self.assertEqual((target / 'remote.txt').read_text(), 'pasted to the file manager')
        self.assertEqual(sorted(p.name for p in target.iterdir()), ['local.txt', 'remote.txt'])

        # Pasted in the local panel: downloaded
        x, y = gui.screen_point(W + 'LocalDirView')
        self.session.input('move', x, y, 'click', 'sleep', 300, 'key', 'ctrl+v')
        self.confirm_copy(gui, 'Download file \'remote.txt\' to local directory:')
        self.assertEqual((local / 'remote.txt').read_text(), 'pasted to the file manager')
        self.assertEqual(gui.quit(), 0)

    def test_clipboard_temporary(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (remote / 'remote.txt').write_text('through a temporary directory')
        (remote / 'with space.txt').write_text('second')
        gui = self.start_commander(remote, local, TEMPORARY_MODE_INI)
        self.place_commander(gui, 0, 0, 800, 860)
        log = self.start_probe('probe-paste')
        probe = self.session.place_window('probe-paste', 900, 100, 500, 400)['client']

        def paste():
            self.session.input('move', probe['x'] + 100, probe['y'] + 100, 'click', 'sleep', 300, 'type', 'v')
            count = len(self.probe_events(log, 'paste'))
            return gui.wait_for(lambda: self.probe_events(log, 'paste')[count - 1:] and
                                self.probe_events(log, 'paste')[-1], what='the paste')

        # The file is downloaded once the target asks for it
        x, y = gui.row_point(W + 'RemoteDirView', 'remote.txt')
        self.session.input('move', x, y, 'click', 'sleep', 200, 'key', 'ctrl+c')
        pasted = paste()
        self.assertEqual(len(pasted['urls']), 1)
        path = Path(unquote(urlparse(pasted['urls'][0]).path))
        self.assertEqual(path.read_text(), 'through a temporary directory')

        # The names of the files as text
        gui.select(W + 'RemoteDirView', 'remote.txt', 'with space.txt')
        x, y = gui.row_point(W + 'RemoteDirView', 'remote.txt')
        self.session.input('move', x, y, 'click', 'sleep', 200, 'key', 'ctrl+a', 'key', 'ctrl+shift+c')
        self.assertEqual(paste()['text'], 'remote.txt\n"with space.txt"')
        x, y = gui.row_point(W + 'RemoteDirView', 'remote.txt')
        self.session.input('move', x, y, 'click', 'sleep', 200, 'key', 'ctrl+bracketright')
        self.assertEqual(paste()['text'], str(remote))
        self.assertEqual(gui.quit(), 0)

    def accept_host_key(self, gui):
        gui.wait_window('MessageForm')
        self.assertIn('unknown server', gui.get('MessageForm/MainMessage')['text'])
        gui.click('MessageForm/Yes')

    def wait_remote_directory(self, gui, path):
        gui.wait_for(lambda: gui.get(W + 'RemoteDirView')['visible'], 60, 'the remote panel')
        gui.wait_for(lambda: gui.get(W + 'RemotePathComboBox')['text'] == str(path), what='the remote directory')
        gui.wait_idle()

    def run_instance(self, *args, timeout=60):
        """Runs another instance of the application; returns its exit code."""
        env = {'OPENSCP_AUTOMATION_SOCKET': str(self.session.dir / 'instance.sock'),
               'LSAN_OPTIONS': 'suppressions=%s:print_suppressions=0' % LSAN_SUPPRESSIONS}
        process = self.session.spawn([guitests.GUI, *map(str, args)], log_name='%s-instance' % self.test_name, extra_env=env)
        return process.wait(timeout)

    def test_session_url(self):
        remote = self.work_dir('remote')
        (remote / 'file.txt').write_text('listed')
        gui = self.start_gui(session_url(str(remote) + '/'), key_switch(), ini='')
        self.accept_host_key(gui)
        self.wait_remote_directory(gui, remote)
        self.assertEqual(gui.names(W + 'RemoteDirView'), ['..', 'file.txt'])
        self.assertIsNone(gui.window('LoginDialog'))
        self.assertEqual(gui.quit(), 0)

    def test_session_to_running_instance(self):
        remote = self.work_dir('remote')
        gui = self.start_gui(ini='')
        gui.wait_window('LoginDialog')
        # Not while the site manager shows: the other instance opens its own
        # window then; the session goes to the running one once it is closed
        gui.click('LoginDialog/CloseButton')
        gui.wait_no_window('LoginDialog')

        self.assertEqual(self.run_instance(session_url(str(remote) + '/'), key_switch()), 0)
        self.accept_host_key(gui)
        self.wait_remote_directory(gui, remote)
        self.assertEqual(gui.quit(), 0)

    def test_upload_command_line(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'one.txt').write_text('one')
        (local / 'two.txt').write_text('two')
        gui = self.start_gui(session_url(str(remote) + '/'), key_switch(), '/upload', '--', local / 'one.txt', local / 'two.txt',
                             ini=EXIT_ON_COMPLETION_INI)
        self.accept_host_key(gui)
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/DirectoryLabel')['text'], 'Upload 2 files to remote directory:')
        gui.click('CopyDialog/OkButton')
        # The application closes once done
        self.assertEqual(gui.process.wait(60), 0)
        self.assertEqual((remote / 'one.txt').read_text(), 'one')
        self.assertEqual((remote / 'two.txt').read_text(), 'two')

    def test_download_url(self):
        remote = self.work_dir('remote')
        (remote / 'file.txt').write_text('downloaded by its URL')
        gui = self.start_gui(session_url(str(remote / 'file.txt')), key_switch(), ini=EXIT_ON_COMPLETION_INI)
        self.accept_host_key(gui)
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/LocalDirectoryEdit')['text'], str(self.session.home) + '/*.*')
        gui.click('CopyDialog/OkButton')
        self.assertEqual(gui.process.wait(60), 0)
        self.assertEqual((self.session.home / 'file.txt').read_text(), 'downloaded by its URL')

    def test_paste_session_url(self):
        remote = self.work_dir('remote')
        # The key for the sessions of URLs
        gui = self.start_gui(key_switch(), ini='')
        gui.wait_window('LoginDialog')
        gui.click('LoginDialog/CloseButton')
        gui.wait_no_window('LoginDialog')
        self.place_commander(gui, 0, 0, 800, 860)
        log = self.start_probe('probe-copy', '--clipboard-text', session_url(str(remote) + '/'))
        probe = self.session.place_window('probe-copy', 900, 100, 500, 400)['client']
        self.session.input('move', probe['x'] + 100, probe['y'] + 100, 'click', 'sleep', 300, 'type', 'c')
        gui.wait_for(lambda: self.probe_events(log, 'copy'), what='the copy')

        x, y = gui.screen_point(W + 'LocalDirView')
        self.session.input('move', x, y, 'click', 'sleep', 300, 'key', 'ctrl+v')
        self.accept_host_key(gui)
        self.wait_remote_directory(gui, remote)
        self.assertEqual(gui.quit(), 0)

    def test_notifications(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'up.txt').write_text('in the background')
        log = self.start_desktop_services()
        # The queue is hidden
        gui = self.start_commander(remote, local, '[Configuration\\Interface\\QueueView]\nShow=2\n')

        # Notified once the background transfer is done
        self.select(gui, 'Local', 'up.txt')
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_window('CopyDialog')
        gui.set('CopyDialog/QueueCheck2', True)
        gui.click('CopyDialog/OkButton')
        gui.wait_for(lambda: self.probe_events(log, 'notify'), 60, 'the notification')
        notification = self.probe_events(log, 'notify')[0]
        self.assertEqual(notification['summary'], 'local \u2013 Information')
        self.assertEqual(notification['body'], 'Transfer queue is empty.')
        self.assertEqual(notification['app_name'], 'OpenSCP')
        self.assertEqual(notification['hints']['desktop-entry'], 'openscp')
        self.assertIn('default', notification['actions'])
        self.assertEqual((remote / 'up.txt').read_text(), 'in the background')
        self.assertFalse(gui.get(W + 'QueuePanel')['visible'])

        # A click shows the queue
        self.desktop_call('ClickNotification', notification['id'])
        gui.wait_for(lambda: gui.get(W + 'QueuePanel')['visible'], what='the queue')
        self.assertEqual(gui.quit(), 0)

    def test_taskbar_progress(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'big.bin').write_bytes(os.urandom(1024 * 1024))
        log = self.start_desktop_services()
        # 256 KB/s
        gui = self.start_commander(remote, local, '[Configuration\\Interface\\CopyParam]\nCPSLimit=262144\n')

        self.select(gui, 'Local', 'big.bin')
        gui.trigger(W + 'CurrentCopyAction')
        self.confirm_copy(gui, "Upload file 'big.bin' to remote directory:")
        self.assertEqual((remote / 'big.bin').stat().st_size, 1024 * 1024)
        updates = [event for event in self.probe_events(log, 'launcher')]
        self.assertTrue(all(update['uri'] == 'application://openscp.desktop' for update in updates))
        shown = [update['properties']['progress'] for update in updates if update['properties']['progress-visible']]
        self.assertTrue(any(0 < progress < 1 for progress in shown), updates)
        self.assertEqual(shown, sorted(shown))
        # Hidden once done
        self.assertFalse(updates[-1]['properties']['progress-visible'])
        self.assertEqual(gui.quit(), 0)

    def test_minimize_to_tray(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        log = self.start_desktop_services()
        gui = self.start_commander(remote, local)

        D = 'SynchronizeDialog/'
        gui.trigger(W + 'SynchronizeAction')
        gui.wait_window('SynchronizeDialog')
        gui.set(D + 'SynchronizeSynchronizeCheck', False)
        gui.click(D + 'StartButton')
        gui.wait_for(lambda: gui.get(D + 'StopButton')['visible'], 60, 'watching')
        self.assertTrue(gui.get(D + 'MinimizeDropDownButton')['visible'])

        gui.trigger(D + 'MinimizetoTray1')
        gui.wait_for(lambda: self.probe_events(log, 'tray_register'), 30, 'the tray icon')
        gui.wait_for(lambda: not gui.get('ScpCommanderForm')['visible'], what='the hidden window')
        # Still watching
        (local / 'new.txt').write_text('new')
        gui.wait_for(lambda: (remote / 'new.txt').exists(), 60, 'the upload of the new file')

        # A click on the icon restores the application
        self.desktop_call('ActivateTrayItem')
        gui.wait_for(lambda: gui.get('ScpCommanderForm')['visible'], what='the restored window')
        gui.wait_for(lambda: self.probe_events(log, 'tray_unregister'), 30, 'the hidden tray icon')
        gui.click(D + 'StopButton')
        gui.wait_for(lambda: gui.get(D + 'StartButton')['visible'], what='stopped watching')
        gui.click(D + 'CancelButton')
        gui.wait_no_window('SynchronizeDialog')
        self.assertEqual(gui.quit(), 0)

    def test_shut_down_once_empty(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'last.txt').write_text('last')
        log = self.start_desktop_services(logind=True)
        gui = self.start_commander(remote, local)

        # Queued while the queue is disabled
        gui.trigger(W + 'QueueEnableAction')
        self.select(gui, 'Local', 'last.txt')
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_window('CopyDialog')
        gui.set('CopyDialog/QueueCheck2', True)
        gui.click('CopyDialog/OkButton')
        gui.wait_no_window('CopyDialog')
        gui.wait_for(lambda: gui.rows(W + 'QueueView3'), 30, 'the pending item')
        self.assertTrue(gui.get(W + 'QueueSuspendOnceEmptyAction2')['visible'])
        self.assertTrue(gui.get(W + 'QueueShutDownOnceEmptyAction2')['visible'])
        gui.trigger(W + 'QueueShutDownOnceEmptyAction2')
        gui.trigger(W + 'QueueEnableAction')

        # The application closes and the computer is shut down (by the fake
        # logind)
        self.assertEqual(gui.process.wait(60), 0)
        self.assertEqual((remote / 'last.txt').read_text(), 'last')
        self.assertEqual(self.probe_events(log, 'logind'), [{'event': 'logind', 'method': 'PowerOff', 'interactive': True}])

    def test_suspend_once_done(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'big.bin').write_bytes(os.urandom(1024 * 1024))
        log = self.start_desktop_services(logind=True)
        # 256 KB/s
        gui = self.start_commander(remote, local, '[Configuration\\Interface\\CopyParam]\nCPSLimit=262144\n')

        self.select(gui, 'Local', 'big.bin')
        gui.trigger(W + 'CurrentCopyAction')
        gui.wait_window('CopyDialog')
        gui.click('CopyDialog/OkButton')
        gui.wait_window('ProgressForm')
        self.assertEqual(gui.get('ProgressForm/CycleOnceDoneItem')['text'], 'Once finished: Stay Idle')
        gui.trigger('ProgressForm/SuspendOnceDoneItem')
        gui.wait_for(lambda: gui.get('ProgressForm/CycleOnceDoneItem')['text'] == 'Once finished: Sleep Computer',
                     what='the choice')
        self.screenshot('progress')

        # Suspended once done, then asked whether to exit
        gui.wait_window('MessageForm', 60)
        self.assertEqual(self.probe_events(log, 'logind'), [{'event': 'logind', 'method': 'Suspend', 'interactive': True}])
        gui.click('MessageForm/No')
        gui.wait_no_window('MessageForm')
        self.assertEqual((remote / 'big.bin').stat().st_size, 1024 * 1024)
        self.assertEqual(gui.quit(), 0)

    def test_service_menu_upload(self):
        remote = self.work_dir('remote')
        source = self.work_dir('dolphin')
        (source / 'sent.txt').write_text('sent from Dolphin')
        gui = OpenScpGui(self.session, guitests.GUI)
        self.addCleanup(gui.stop)
        gui.socket_path.unlink(missing_ok=True)
        gui.config_file.parent.mkdir(parents=True, exist_ok=True)
        gui.config_file.write_text(ssh.site_ini(remote=remote) + EXIT_ON_COMPLETION_INI)
        # The action of the context menu, starting the binary under test
        menus = self.session.home / '.local/share/kio/servicemenus'
        menus.mkdir(parents=True, exist_ok=True)
        menu = menus / 'openscp-upload.desktop'
        self.addCleanup(menu.unlink)
        text = (SOURCE / 'source/qt/desktop/openscp-upload.desktop').read_text()
        menu.write_text(text.replace('=openscp', '=%s' % guitests.GUI))
        # Dolphin runs the actions of the user's directory when executable
        menu.chmod(0o755)
        dolphin = self.start_dolphin(source, env=gui.automation_env())

        x, y = dolphin.item_point('sent.txt')
        self.session.do_popup_action('dolphin', 'Upload with OpenSCP', 'Press',
                                     lambda: self.session.input('move', x, y, 'click', 'sleep', 200, 'click', 'right'))

        # The site manager, the upload, then the application closes
        gui.attach()
        self.login(gui, 'local')
        gui.wait_window('CopyDialog')
        self.assertEqual(gui.get('CopyDialog/DirectoryLabel')['text'], "Upload file 'sent.txt' to remote directory:")
        gui.click('CopyDialog/OkButton')
        gui.wait_for(lambda: (remote / 'sent.txt').exists() and not gui.socket_path.exists(), 60, 'the upload')
        self.assertEqual((remote / 'sent.txt').read_text(), 'sent from Dolphin')

    def client_script(self, name):
        """A fake SSH client that records its arguments."""
        directory = self.work_dir('client')
        record = directory / 'arguments.json'
        script = directory / name
        script.write_text('#!%s\nimport json, sys\njson.dump(sys.argv[1:], open(%r, "w"))\n' % (sys.executable, str(record)))
        script.chmod(0o755)
        return script, record

    def open_in_client(self, client):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        ini = '[Configuration\\Interface]\nPuttyPath=%s\n' % client
        gui = self.start_commander(remote, local, ini)
        self.assertTrue(gui.get(W + 'PuttyAction')['enabled'])
        gui.trigger(W + 'PuttyAction')
        return gui

    def test_open_in_ssh_client(self):
        script, record = self.client_script('terminal')
        gui = self.open_in_client('%s -e ssh' % script)
        gui.wait_for(record.exists, 30, 'the client')
        # The session in the syntax of OpenSSH (it does not read PuTTY's keys)
        self.assertEqual(json.loads(record.read_text()),
                         ['-e', 'ssh', '-p', str(ssh.SSHD.port), '-l', ssh.SSHD.user, '--', '127.0.0.1'])
        self.assertEqual(gui.quit(), 0)

    def test_open_in_putty(self):
        script, record = self.client_script('putty')
        gui = self.open_in_client(script)
        gui.wait_for(record.exists, 30, 'the client')
        self.assertEqual(json.loads(record.read_text()),
                         ['-ssh', '127.0.0.1', '-l', ssh.SSHD.user, '-P', str(ssh.SSHD.port),
                          '-i', str(ssh.key_file()), '-agent', '-a'])
        self.assertEqual(gui.quit(), 0)

    def test_open_in_client_command(self):
        # The patterns of the site: the command is the user's
        script, record = self.client_script('client')
        gui = self.open_in_client('%s !U@!@ !#' % script)
        gui.wait_for(record.exists, 30, 'the client')
        self.assertEqual(json.loads(record.read_text()), ['%s@127.0.0.1' % ssh.SSHD.user, str(ssh.SSHD.port)])
        self.assertEqual(gui.quit(), 0)


if __name__ == '__main__':
    unittest.main()
