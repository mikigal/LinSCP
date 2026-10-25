#!/usr/bin/env python3
"""Runs Dolphin, KDE's file manager, in a headless KWin session
(tools/wlharness) as the peer of the desktop integration tests: drag and
drop and the clipboard with a real file manager.

Its files are found over AT-SPI (the session's start_accessibility must run
first); the points are on the screen, for the real input of the compositor.
"""

import time
from pathlib import Path

import wlharness


class Dolphin:
    def __init__(self, session, directory, log_name='dolphin'):
        self.session = session
        self.directory = Path(directory)
        self.log_name = log_name
        self.process = None

    @property
    def caption(self):
        return '%s — Dolphin' % self.directory.name

    def start(self, timeout=60):
        self.process = self.session.spawn(['dolphin', '--new-window', str(self.directory)],
                                          log_name=self.log_name, extra_env=wlharness.ACCESSIBILITY_ENV)
        self.session.wait_for_window(self.caption, timeout)
        # The view lists the files once it is accessible
        self.session.wait_for_accessible('dolphin', self.directory.name, 'list', timeout)
        return self

    def stop(self):
        if (self.process is not None) and (self.process.poll() is None):
            self.process.terminate()
            try:
                self.process.wait(10)
            except Exception:
                self.process.kill()
                self.process.wait()

    def place(self, x, y, width, height):
        self.session.place_window(self.caption, x, y, width, height)
        # Lets the view lay its items out again
        time.sleep(0.5)

    def _to_screen(self, accessible, center=True):
        window = self.session.wait_for_window(self.caption)
        client = window['client']
        if center:
            return (client['x'] + accessible['x'] + accessible['width'] // 2,
                    client['y'] + accessible['y'] + accessible['height'] // 2)
        return client['x'] + accessible['x'], client['y'] + accessible['y']

    def item_point(self, name, timeout=20):
        """The center of a file or directory of the view."""
        return self._to_screen(self.session.wait_for_accessible('dolphin', name, 'list item', timeout))

    def view_point(self, timeout=20):
        """A point of the free space of the view (its bottom right part)."""
        view = self.session.wait_for_accessible('dolphin', self.directory.name, 'list', timeout)
        x, y = self._to_screen(view, center=False)
        return x + view['width'] * 3 // 4, y + view['height'] * 3 // 4

    def paste(self, timeout=20):
        """Pastes the clipboard to the directory of the view (the action of
        its menu, whose name tells what the clipboard has: "Paste One File",
        ...)."""
        found = []

        def present():
            found[:] = [a['name'] for a in self.session.accessibles('dolphin', role='menu item')
                        if a['name'].startswith('Paste') and 'Clipboard' not in a['name']]
            return bool(found)

        wlharness._wait_for(present, timeout, 'the paste action of Dolphin')
        self.session.do_accessible_action('dolphin', found[0], 'Press', role='menu item')
        return found[0]
