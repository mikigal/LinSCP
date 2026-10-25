#!/usr/bin/env python3
"""Runs the OpenSCP GUI in a headless KWin session and drives it through its
automation hook, for the end-to-end tests.

The application runs in a wlharness.Session (tools/wlharness), which isolates
it from the user's desktop (its own compositor, D-Bus, HOME and XDG
directories). The automation hook (source/qt/app/Automation.h, built with
OPENSCP_AUTOMATION) listens on a Unix socket in the session directory;
widgets are addressed by their object names, "Window/Child".
"""

import json
import socket
import time
from pathlib import Path

LSAN_SUPPRESSIONS = Path(__file__).resolve().with_name('lsan-suppressions.txt')


class AutomationError(RuntimeError):
    pass


class OpenScpGui:
    def __init__(self, session, binary):
        self.session = session
        self.binary = str(binary)
        self.socket_path = session.dir / 'openscp.sock'
        self.process = None
        self._socket = None
        self._buffer = b''
        self._next_id = 1

    @property
    def home(self):
        return self.session.home

    @property
    def config_file(self):
        """The default configuration file."""
        return self.home / '.config' / 'openscp' / 'openscp.ini'

    def start(self, *args, env=None, log_name='openscp', timeout=60):
        self.socket_path.unlink(missing_ok=True)
        extra_env = self.automation_env()
        extra_env.update(env or {})
        self.process = self.session.spawn([self.binary, *map(str, args)], log_name=log_name, extra_env=extra_env)
        self.log = self.session.logs / ('%s.log' % log_name)
        deadline = time.monotonic() + timeout
        while True:
            if self.process.poll() is not None:
                raise AutomationError('%s exited with %d; see %s' % (self.binary, self.process.returncode, self.log))
            if self.socket_path.exists():
                try:
                    self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    self._socket.connect(str(self.socket_path))
                    break
                except OSError:
                    self._socket.close()
                    self._socket = None
            if time.monotonic() > deadline:
                raise AutomationError('the automation socket did not appear; see %s' % self.log)
            time.sleep(0.05)
        return self

    def attach(self, timeout=60):
        """Connects to an instance started by another application (with
        OPENSCP_AUTOMATION_SOCKET set to socket_path)."""
        deadline = time.monotonic() + timeout
        while True:
            if self.socket_path.exists():
                try:
                    self._socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    self._socket.connect(str(self.socket_path))
                    return self
                except OSError:
                    self._socket.close()
                    self._socket = None
            if time.monotonic() > deadline:
                raise AutomationError('the automation socket %s did not appear' % self.socket_path)
            time.sleep(0.05)

    def automation_env(self):
        """The environment of an instance to attach to."""
        return {
            'OPENSCP_AUTOMATION_SOCKET': str(self.socket_path),
            'LSAN_OPTIONS': 'suppressions=%s:print_suppressions=0' % LSAN_SUPPRESSIONS,
            'UBSAN_OPTIONS': 'print_stacktrace=1',
        }

    def stop(self):
        """Ends the application if it still runs."""
        if self._socket is not None:
            self._socket.close()
            self._socket = None
        if (self.process is not None) and (self.process.poll() is None):
            self.process.kill()
            self.process.wait()

    # --- the protocol ----------------------------------------------------

    def call(self, cmd, timeout=60, **params):
        request_id = self._next_id
        self._next_id += 1
        request = dict(params, id=request_id, cmd=cmd)
        self._socket.sendall(json.dumps(request).encode() + b'\n')
        deadline = time.monotonic() + timeout
        while True:
            line, separator, rest = self._buffer.partition(b'\n')
            if separator:
                self._buffer = rest
                reply = json.loads(line)
                if reply.get('id') != request_id:
                    continue
                if not reply['ok']:
                    raise AutomationError('%s %s: %s' % (cmd, params, reply['error']))
                return reply.get('result')
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise AutomationError('no reply to %s %s' % (cmd, params))
            self._socket.settimeout(remaining)
            try:
                data = self._socket.recv(65536)
            except socket.timeout:
                data = None
            if data == b'':
                raise AutomationError('the application closed the connection; see %s' % self.log)
            if data:
                self._buffer += data

    def windows(self):
        return self.call('windows')

    def window(self, name):
        return next((w for w in self.windows() if w['name'] == name), None)

    def get(self, target, children=False):
        return self.call('get', target=target, children=children)

    def set(self, target, value):
        return self.call('set', target=target, value=value)

    def click(self, target):
        return self.call('click', target=target)

    def trigger(self, target):
        return self.call('trigger', target=target)

    def select(self, target, *items):
        return self.call('select', target=target, items=list(items))

    def activate(self, target, item):
        return self.call('activate', target=target, item=item)

    def expand(self, target, item, expanded=True):
        return self.call('expand', target=target, item=item, expanded=expanded)

    def key(self, key, target=None):
        """Sends a key (e.g. "Return", "Ctrl+A") to the widget or the focused one;
        shortcuts need real input (Session.input)."""
        params = {'key': key}
        if target is not None:
            params['target'] = target
        return self.call('key', **params)

    def grab(self, target, path):
        path = Path(path).resolve()
        self.call('grab', target=target, file=str(path))
        return path

    def focus(self, target=None):
        """Focuses the widget; without one, describes the focused widget."""
        if target is None:
            return self.call('focus')
        return self.call('focus', target=target)

    def wait_idle(self, timeout=120):
        return self.call('wait-idle', timeout=timeout)

    # --- conveniences ----------------------------------------------------

    def wait_for(self, predicate, timeout=30, what='the condition'):
        """Polls predicate() until it returns a true value, which it returns."""
        deadline = time.monotonic() + timeout
        while True:
            value = predicate()
            if value:
                return value
            if time.monotonic() > deadline:
                raise AutomationError('timed out waiting for %s' % what)
            time.sleep(0.05)

    def wait_window(self, name, timeout=30):
        return self.wait_for(lambda: self.window(name), timeout, 'window %s' % name)

    def wait_no_window(self, name, timeout=30):
        self.wait_for(lambda: self.window(name) is None, timeout, 'window %s to close' % name)

    def rows(self, target):
        """The texts of the cells of the top-level rows of an item view."""
        return [row['cells'] for row in self.get(target)['rows']]

    def names(self, target):
        """The texts of the first column of the top-level rows of an item view."""
        return [cells[0] for cells in self.rows(target)]

    def screen_point(self, target):
        """The center of a widget on the screen, from the geometry of its window
        in the compositor."""
        window_name = target.split('/')[0]
        title = self.wait_window(window_name)['title']
        geometry = self.get(target)['geometry']
        windows = self.wait_for(lambda: [w for w in self.session.windows() if w['caption'] == title],
                                what='the window %s in the compositor' % title)
        client = windows[0]['client']
        return (client['x'] + geometry['x'] + geometry['width'] // 2,
                client['y'] + geometry['y'] + geometry['height'] // 2)

    def client_origin(self, window_name):
        """The position of the client area of a window on the screen."""
        title = self.wait_window(window_name)['title']
        windows = self.wait_for(lambda: [w for w in self.session.windows() if w['caption'] == title],
                                what='the window %s in the compositor' % title)
        client = windows[0]['client']
        return client['x'], client['y']

    def row_point(self, view, name):
        """The center of the first cell of the row of an item view on the
        screen."""
        rows = [row for row in self.get(view)['rows'] if row['cells'][0] == name]
        if not rows or 'rect' not in rows[0]:
            raise AutomationError('no visible row %s in %s' % (name, view))
        rect = rows[0]['rect']
        x, y = self.client_origin(view.split('/')[0])
        return x + rect['x'] + min(rect['width'] // 2, 60), y + rect['y'] + rect['height'] // 2

    def drag_real(self, start, end, steps=20):
        """Drags with the real input of the compositor from start to end
        (screen points): crosses the drag threshold at start, moves gradually
        and drops."""
        (x1, y1), (x2, y2) = start, end
        commands = ['move', x1, y1, 'down', 'sleep', 100, 'rmove', 6, 6, 'sleep', 100, 'rmove', 6, 6, 'sleep', 400]
        for step in range(1, steps + 1):
            t = step / steps
            commands += ['move', round(x1 + (x2 - x1) * t), round(y1 + (y2 - y1) * t), 'sleep', 30]
        commands += ['sleep', 500, 'up']
        self.session.input(*commands)

    def click_real(self, target):
        """Clicks a widget with the real input of the compositor (popups and
        shortcuts need it)."""
        x, y = self.screen_point(target)
        self.session.input('move', x, y, 'click')

    def quit(self, timeout=60):
        """Quits the application; returns its exit code."""
        self.call('quit')
        code = self.process.wait(timeout=timeout)
        self._socket.close()
        self._socket = None
        return code
