"""End-to-end tests of the GUI that need no server (milestone M5)."""

import stat
import unittest

from guitests import GUI, GuiTestCase
from linscp_gui import LinScpGui


def mode(path):
    return stat.S_IMODE(path.stat().st_mode)


class StartupTest(GuiTestCase):
    def test_main_window_and_clean_exit(self):
        gui = self.start_gui()
        window = gui.wait_window('ScpCommanderForm')
        self.assertEqual(window['title'], 'LinSCP')
        self.session.wait_for_window('LinSCP')
        self.assertEqual(gui.quit(), 0)

    def test_private_configuration(self):
        # A new configuration: its directory and file are the user's only
        # (the file holds the passwords)
        gui = self.start_gui()
        gui.wait_window('ScpCommanderForm')
        self.assertEqual(gui.quit(), 0)
        self.assertTrue(gui.config_file.exists())
        self.assertEqual(mode(gui.config_file.parent), 0o700)
        self.assertEqual(mode(gui.config_file), 0o600)

        # An existing one is made private
        gui.config_file.parent.chmod(0o755)
        gui.config_file.chmod(0o644)
        gui = LinScpGui(self.session, GUI)
        self.addCleanup(gui.stop)
        gui.start(log_name='%s-existing' % self.test_name)
        gui.wait_window('ScpCommanderForm')
        self.assertEqual(mode(gui.config_file.parent), 0o700)
        self.assertEqual(mode(gui.config_file), 0o600)
        self.assertEqual(gui.quit(), 0)
        self.assertEqual(mode(gui.config_file), 0o600)


if __name__ == '__main__':
    unittest.main()
