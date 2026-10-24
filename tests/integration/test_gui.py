"""End-to-end tests of the GUI that need no server (milestone M5)."""

import unittest

from guitests import GuiTestCase


class StartupTest(GuiTestCase):
    def test_main_window_and_clean_exit(self):
        gui = self.start_gui()
        window = gui.wait_window('ScpCommanderForm')
        self.assertEqual(window['title'], 'OpenSCP')
        self.session.wait_for_window('OpenSCP')
        self.assertEqual(gui.quit(), 0)


if __name__ == '__main__':
    unittest.main()
