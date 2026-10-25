"""End-to-end tests of the desktop integration of the GUI (milestone M7):
drag and drop between the panels and with Dolphin, the file clipboard.

SFTP against the local test sshd, as test_gui_ssh (whose setup this module
shares): the remote side is the local file system. Dolphin runs in the same
headless KWin session; its files are found over AT-SPI.
"""

import json
import time
import unittest
from pathlib import Path
from urllib.parse import unquote, urlparse

import test_gui_ssh as ssh
import wlharness
from dolphin import Dolphin

W = 'ScpCommanderForm/'
# Downloads for other applications go to a temporary directory
TEMPORARY_MODE_INI = '[Configuration\\Interface]\nDDFakeFile=0\n'


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

    def start_probe(self, title):
        """wlh-probe, a Qt application that logs what is dropped on it."""
        log = self.session.logs / ('%s.events' % title)
        self.session.spawn([wlharness._default_tool('wlh-probe'), '--title', title, '--log', str(log)], log_name=title)
        self.session.wait_for_window(title)
        return log

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

    def start_dolphin(self, directory):
        dolphin = Dolphin(self.session, directory, log_name='%s-dolphin' % self.test_name).start()
        self.addCleanup(dolphin.stop)
        dolphin.place(820, 0, 780, 860)
        return dolphin

    def test_drag_between_panels(self):
        remote = self.work_dir('remote')
        local = self.work_dir('local')
        (local / 'up.txt').write_text('uploaded')
        (remote / 'down.txt').write_text('downloaded')
        (remote / 'sub').mkdir()
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


if __name__ == '__main__':
    unittest.main()
