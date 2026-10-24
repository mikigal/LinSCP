"""The base of the end-to-end tests of the GUI (L4, docs/port/PLAN.md
section 6.2): OpenSCP in a headless KWin session (tools/wlharness), driven
through its automation hook (tools/testenv/openscp_gui.py).

The binary is taken from OPENSCP_GUI. Each test class has a session of its
own; each test starts the application. Sessions of failed tests are kept
for inspection (logs and screenshots in the session directory).
"""

import os
import shutil
import unittest

import wlharness
from openscp_gui import OpenScpGui

GUI = os.environ.get('OPENSCP_GUI')


class GuiTestCase(unittest.TestCase):
    width = 1280
    height = 800

    @classmethod
    def setUpClass(cls):
        if not GUI:
            raise unittest.SkipTest('OPENSCP_GUI is not set')
        cls.session = wlharness.Session.create(width=cls.width, height=cls.height)
        cls.failed = False

    @classmethod
    def tearDownClass(cls):
        cls.session.stop()
        if cls.failed:
            print('keeping session %s for inspection' % cls.session.dir)
        else:
            shutil.rmtree(cls.session.dir, ignore_errors=True)

    def run(self, result=None):
        result = result or self.defaultTestResult()
        problems = len(result.failures) + len(result.errors)
        super().run(result)
        if len(result.failures) + len(result.errors) > problems:
            type(self).failed = True
        return result

    def start_gui(self, *args, env=None):
        """Starts the application; it is ended after the test."""
        gui = OpenScpGui(self.session, GUI)
        self.addCleanup(gui.stop)
        gui.start(*args, env=env, log_name=self.id().rsplit('.', 1)[-1])
        return gui

    def screenshot(self, name):
        """A screenshot of the whole screen in the session directory."""
        return self.session.screenshot(self.session.dir / ('%s.png' % name))
