"""Startup smoke tests of a build of the GUI (OPENSCP_SMOKE_GUI), the release
build above all: it has no automation hook, so these check only that the
application starts, connects to the local sshd and keeps running.

The application runs offscreen, never on the user's desktop, with a profile,
a runtime directory and a D-Bus of its own: on the user's session bus a
second instance would hand its command line to a running one. Skipped
without the binary (the release build is not built); the keys are converted
by OPENSCP_CLI.
"""

import os
import shutil
import signal
import subprocess
import tempfile
import time
import unittest
from pathlib import Path

from local_sshd import LocalSshd
from openscp_cli import OpenScpCli

GUI = os.environ.get('OPENSCP_SMOKE_GUI')
CLI = os.environ.get('OPENSCP_CLI')
# How long the application has to keep running once ready (seconds)
SETTLE = 3
# The variables that lead to the user's session
SESSION_VARIABLES = ('WAYLAND_DISPLAY', 'DISPLAY', 'SSH_AUTH_SOCK', 'DBUS_SESSION_BUS_ADDRESS',
                     'DBUS_SYSTEM_BUS_ADDRESS', 'SESSION_MANAGER', 'XDG_SESSION_ID')
BUS_CONFIG = """<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:path={socket}</listen>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
"""


def wait_for(predicate, timeout, what):
    deadline = time.monotonic() + timeout
    while not predicate():
        if time.monotonic() > deadline:
            raise AssertionError('timed out waiting for %s' % what)
        time.sleep(0.1)


class SmokeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not GUI or not Path(GUI).is_file():
            raise unittest.SkipTest('no GUI binary %s' % GUI)
        if not CLI:
            raise unittest.SkipTest('OPENSCP_CLI is not set')
        # Short: the path of the bus socket is limited
        cls.state = Path(tempfile.mkdtemp(prefix='smoke-', dir=os.environ.get('WLH_TMPDIR')))
        cls.bus = cls.start_bus(cls.state / 'bus')
        cls.sshd = LocalSshd(cls.state / 'sshd').start()
        cls.key = cls.state / 'client.ppk'
        result = OpenScpCli(CLI, cls.state / 'keygen').run('/keygen', cls.sshd.client_key, '/output=%s' % cls.key)
        if result.returncode != 0:
            raise RuntimeError('cannot convert the client key: %r' % result)

    @classmethod
    def tearDownClass(cls):
        cls.sshd.stop()
        cls.bus.terminate()
        cls.bus.wait(10)
        shutil.rmtree(cls.state, ignore_errors=True)

    @staticmethod
    def start_bus(directory):
        directory.mkdir()
        config = directory / 'session.conf'
        socket = directory / 'socket'
        config.write_text(BUS_CONFIG.format(socket=socket))
        bus = subprocess.Popen(['dbus-daemon', '--config-file=%s' % config, '--nofork', '--nopidfile'],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        wait_for(lambda: socket.exists() or (bus.poll() is not None), 10, 'the D-Bus')
        if bus.poll() is not None:
            raise RuntimeError('dbus-daemon exited with %d' % bus.returncode)
        bus.address = 'unix:path=%s' % socket
        return bus

    def setUp(self):
        self.profile = self.state / self.id().rsplit('.', 1)[-1]
        self.config_file = self.profile / 'config' / 'openscp' / 'openscp.ini'
        self.process = None

    def tearDown(self):
        if (self.process is not None) and (self.process.poll() is None):
            self.process.send_signal(signal.SIGTERM)
            try:
                self.process.wait(10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()

    def start(self, *args, ini=None):
        if ini is not None:
            self.config_file.parent.mkdir(parents=True)
            self.config_file.write_text(ini)
        env = {name: value for name, value in os.environ.items() if name not in SESSION_VARIABLES}
        for name, sub in (('HOME', 'home'), ('XDG_CONFIG_HOME', 'config'), ('XDG_DATA_HOME', 'data'),
                          ('XDG_CACHE_HOME', 'cache'), ('XDG_STATE_HOME', 'state'), ('XDG_RUNTIME_DIR', 'runtime'),
                          ('TMPDIR', 'tmp')):
            path = self.profile / sub
            path.mkdir(parents=True, exist_ok=True)
            env[name] = str(path)
        (self.profile / 'runtime').chmod(0o700)
        env['QT_QPA_PLATFORM'] = 'offscreen'
        env['DBUS_SESSION_BUS_ADDRESS'] = self.bus.address
        env['DBUS_SYSTEM_BUS_ADDRESS'] = self.bus.address
        self.output = self.profile / 'output.log'
        with open(self.output, 'wb') as output:
            self.process = subprocess.Popen([GUI, *map(str, args)], env=env, stdin=subprocess.DEVNULL,
                                            stdout=output, stderr=subprocess.STDOUT)

    def assert_running(self, what):
        """The application did not end (it ends only when asked)."""
        if self.process.poll() is not None:
            tail = self.output.read_text(errors='replace')[-3000:]
            self.fail('the application ended with %d %s:\n%s' % (self.process.returncode, what, tail))

    def test_starts(self):
        # The site manager, over the main window
        self.start()
        time.sleep(SETTLE)
        self.assert_running('at the start')

    def test_connects(self):
        remote = self.profile / 'remote'
        local = self.profile / 'local'
        for directory in (remote, local):
            (directory / 'docs').mkdir(parents=True)
            (directory / 'notes.md').write_text('# Notes\n')
        fingerprint = subprocess.run(['ssh-keygen', '-l', '-E', 'sha256', '-f', '%s.pub' % self.sshd.host_key],
                                     capture_output=True, text=True, check=True).stdout.split()[1].split(':', 1)[1]
        ini = '\n'.join([
            '[Sessions\\Smoke]', 'HostName=127.0.0.1', 'PortNumber=%d' % self.sshd.port,
            'UserName=%s' % self.sshd.user, 'PublicKeyFile=%s' % self.key,
            'SshHostKey=ssh-ed25519 255 %s' % fingerprint,
            'RemoteDirectory=%s' % remote, 'LocalDirectory=%s' % local, ''])
        log = self.profile / 'session.log'
        self.start('Smoke', '/log=%s' % log, ini=ini)
        # The session is open, its directory read
        wait_for(lambda: log.exists() and ('Startup conversation with host finished.' in log.read_text(errors='replace')),
                 60, 'the session')
        self.assert_running('when connecting')
        wait_for(lambda: str(remote) in log.read_text(errors='replace'), 30, 'the remote directory')
        time.sleep(SETTLE)
        self.assert_running('with the session')


if __name__ == '__main__':
    unittest.main()
