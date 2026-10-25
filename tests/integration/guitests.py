"""The base of the end-to-end tests of the GUI (L4, docs/port/PLAN.md
section 6.2): LinSCP in a headless KWin session (tools/wlharness), driven
through its automation hook (tools/testenv/linscp_gui.py).

The binary is taken from LINSCP_GUI. Each test class has a session of its
own; each test starts the application. Sessions of failed tests are kept
for inspection (logs and screenshots in the session directory), all of them
with LINSCP_GUI_KEEP set.
"""

import os
import shutil
import unittest

import wlharness
from linscp_gui import LinScpGui

GUI = os.environ.get('LINSCP_GUI')


class GuiTestCase(unittest.TestCase):
    width = 1280
    height = 800

    @classmethod
    def setUpClass(cls):
        if not GUI:
            raise unittest.SkipTest('LINSCP_GUI is not set')
        cls.session = wlharness.Session.create(width=cls.width, height=cls.height)
        cls.failed = False

    @classmethod
    def tearDownClass(cls):
        cls.session.stop()
        if cls.failed or os.environ.get('LINSCP_GUI_KEEP'):
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

    def start_gui(self, *args, env=None, ini=None):
        """Starts the application with a configuration of its own (the INI
        file's text, or none); it is ended after the test."""
        gui = LinScpGui(self.session, GUI)
        config = gui.config_file.parent
        shutil.rmtree(config, ignore_errors=True)
        if ini is not None:
            config.mkdir(parents=True)
            gui.config_file.write_text(ini)
        self.addCleanup(gui.stop)
        gui.start(*args, env=env, log_name=self.test_name)
        return gui

    @property
    def test_name(self):
        return self.id().rsplit('.', 1)[-1]

    def work_dir(self, name):
        """A directory of the test in the session directory."""
        path = self.session.dir / self.test_name / name
        path.mkdir(parents=True)
        return path

    def screenshot(self, name):
        """A screenshot of the whole screen in the session directory."""
        return self.session.screenshot(self.session.dir / ('%s-%s.png' % (self.test_name, name)))
