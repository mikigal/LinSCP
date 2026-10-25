#!/usr/bin/env python3
"""Headless KWin (Wayland) session harness for OpenSCP GUI tests.

Each session is fully isolated from the user's desktop: its own D-Bus session
bus, its own XDG_RUNTIME_DIR, HOME and XDG config/data/cache directories, and a
`kwin_wayland --virtual` compositor rendering into a virtual framebuffer.

Usable as a module (see Session) and as a command line tool:

    wlharness.py start [--width W] [--height H] [--scale S]   # prints session dir
    wlharness.py -s DIR spawn [--log NAME] -- COMMAND...        # background process
    wlharness.py -s DIR run -- COMMAND...                       # foreground process
    wlharness.py -s DIR shot FILE.png
    wlharness.py -s DIR windows
    wlharness.py -s DIR place CAPTION X Y W H
    wlharness.py -s DIR input COMMAND...                        # see wlh-input
    wlharness.py -s DIR env                                      # shell exports
    wlharness.py -s DIR stop

The session directory can also be given via the WLH_SESSION environment variable.
"""

import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import uuid
from pathlib import Path

WAYLAND_SOCKET = "wayland-wlh"
# sockaddr_un.sun_path is 108 bytes including the terminating NUL
MAX_SOCKET_PATH = 107
STATE_FILE = "session.json"
SCRIPT_MARKER = "WLH-RESULT"

# Environment variables copied from the caller into the isolated session
INHERITED_ENV = ("PATH", "LANG", "LANGUAGE", "LC_ALL", "LC_CTYPE", "LC_MESSAGES", "TZ", "TERM", "USER", "LOGNAME", "SHELL")


class HarnessError(RuntimeError):
    pass


def _wait_for(predicate, timeout, what):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.05)
    raise HarnessError(f"timed out waiting for {what}")


def _default_tool(name):
    """Locates a harness binary: explicit environment variable, then build tree, then PATH."""
    env_name = "WLH_" + name.upper().replace("WLH-", "").replace("-", "_")
    if os.environ.get(env_name):
        return os.environ[env_name]
    repo = Path(__file__).resolve().parents[2]
    for build in sorted((repo / "build").glob("*/tools/wlharness")):
        candidate = build / name
        if candidate.exists():
            return str(candidate)
    found = shutil.which(name)
    if found:
        return found
    raise HarnessError(f"cannot find {name}; build it (cmake --build --preset dev) or set {env_name}")


class Session:
    """An isolated headless KWin session."""

    def __init__(self, directory):
        self.dir = Path(directory)
        self.runtime_dir = self.dir / "rt"
        self.home = self.dir / "home"
        self.logs = self.dir / "logs"
        self.state = {}
        state_file = self.dir / STATE_FILE
        if state_file.exists():
            self.state = json.loads(state_file.read_text())

    # --- lifecycle -------------------------------------------------------

    @classmethod
    def create(cls, width=1280, height=800, scale=1.0, base_dir=None, document_portal=True):
        """Starts a new session.

        document_portal: Qt transfers dragged and copied files between applications through
        the XDG document portal; when the portal is offered but broken, the receiver gets no
        URLs at all. The portal needs a FUSE mount, which requires either a setuid
        fusermount3 (unusable under no_new_privs sandboxes) or capabilities. So by default
        the session runs in its own user and mount namespace, whose D-Bus daemon keeps the
        namespace capabilities for the services it activates. Pass False to run without
        namespaces and without the document portal.
        """
        base = base_dir or os.environ.get("WLH_TMPDIR") or tempfile.gettempdir()
        directory = Path(tempfile.mkdtemp(prefix="wlh-", dir=base))
        session = cls(directory)
        session._start(width, height, scale, document_portal)
        return session

    def _start(self, width, height, scale, document_portal):
        socket_path = self.runtime_dir / WAYLAND_SOCKET
        if len(str(socket_path)) > MAX_SOCKET_PATH:
            raise HarnessError(f"session path too long for a Unix socket: {socket_path}; set WLH_TMPDIR to a shorter directory")
        for path in (self.runtime_dir, self.home, self.logs, self.home / ".config", self.home / ".local/share", self.home / ".cache", self.home / ".local/state"):
            path.mkdir(parents=True, exist_ok=True)
        self.runtime_dir.chmod(0o700)

        env = self._base_env()
        self.state = {"env": env, "pids": {}, "children": [], "namespace_pid": None, "width": width, "height": height, "scale": scale}
        self._write_desktop_config()
        bus_config = self._write_bus_config(document_portal)

        bus_command = ["dbus-daemon", f"--config-file={bus_config}", "--nofork", "--nopidfile"]
        if document_portal:
            # unshare execs the daemon in place, so it is the namespace's first process;
            # everything else joins its namespaces (see _command), without capabilities
            bus_command = ["unshare", "--user", "--map-current-user", "--mount", "--keep-caps", "--", *bus_command]
        with open(self.logs / "dbus.log", "wb") as log:
            bus = subprocess.Popen(bus_command, env=env, stdout=subprocess.DEVNULL, stderr=log, start_new_session=True)
        self.state["pids"]["dbus"] = bus.pid
        if document_portal:
            self.state["namespace_pid"] = bus.pid
        self._save()
        _wait_for(lambda: (self.runtime_dir / "bus").exists() or bus.poll() is not None, 10, "the D-Bus session bus")
        if bus.poll() is not None:
            raise HarnessError(f"dbus-daemon exited with {bus.returncode}; see {self.logs / 'dbus.log'}")

        kwin_env = dict(env)
        # print() from KWin scripts is logged under this category
        kwin_env["QT_LOGGING_RULES"] = "kwin_scripting.debug=true"
        kwin_env.pop("WAYLAND_DISPLAY")
        kwin_env.pop("QT_QPA_PLATFORM")
        with open(self.logs / "kwin.log", "wb") as log:
            kwin = subprocess.Popen(
                self._command(["kwin_wayland", "--virtual", "--socket", WAYLAND_SOCKET,
                               "--width", str(width), "--height", str(height), "--scale", str(scale),
                               "--no-lockscreen", "--no-global-shortcuts", "--no-kactivities"]),
                env=kwin_env, stdout=subprocess.DEVNULL, stderr=log, start_new_session=True)
        self.state["pids"]["kwin"] = kwin.pid
        self._save()
        try:
            _wait_for(lambda: socket_path.exists() or kwin.poll() is not None, 20, "the KWin Wayland socket")
            if kwin.poll() is not None:
                raise HarnessError(f"kwin_wayland exited with {kwin.returncode}; see {self.logs / 'kwin.log'}")
            _wait_for(self._kwin_on_bus, 20, "KWin on the session bus")
        except Exception:
            self.stop()
            raise

    def _write_desktop_config(self):
        """Pins look and behaviour so screenshots and geometry are deterministic."""
        config = self.home / ".config"
        (config / "kdeglobals").write_text(
            "[General]\nColorScheme=BreezeLight\n\n"
            "[Icons]\nTheme=breeze\n\n"
            "[KDE]\nLookAndFeelPackage=org.kde.breeze.desktop\nwidgetStyle=Breeze\nAnimationDurationFactor=0\n")
        applications = self.home / ".local/share/applications"
        applications.mkdir(parents=True, exist_ok=True)
        # Lets the portal resolve the app IDs of the probe and of OpenSCP; it reads them when
        # it starts (activated by the first application that uses it)
        for app_id, name in (("wlh-probe", "wlh-probe"), ("openscp", "OpenSCP")):
            (applications / f"{app_id}.desktop").write_text(
                f"[Desktop Entry]\nType=Application\nName={name}\nExec={app_id}\nNoDisplay=true\n")

    def _write_bus_config(self, document_portal):
        """Session bus whose activatable services are the system ones, optionally minus the document portal."""
        services = self.dir / "dbus-services"
        services.mkdir(exist_ok=True)
        for directory in (Path("/usr/local/share/dbus-1/services"), Path("/usr/share/dbus-1/services")):
            if not directory.is_dir():
                continue
            for service in directory.glob("*.service"):
                if not document_portal and service.name == "org.freedesktop.portal.Documents.service":
                    continue
                link = services / service.name
                if not link.exists():
                    link.symlink_to(service)
        config = self.dir / "dbus-session.conf"
        config.write_text(f"""<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:path={self.runtime_dir / 'bus'}</listen>
  <servicedir>{services}</servicedir>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
""")
        return config

    def _base_env(self):
        env = {name: os.environ[name] for name in INHERITED_ENV if name in os.environ}
        env.update({
            "HOME": str(self.home),
            "XDG_RUNTIME_DIR": str(self.runtime_dir),
            "XDG_CONFIG_HOME": str(self.home / ".config"),
            "XDG_DATA_HOME": str(self.home / ".local/share"),
            "XDG_CACHE_HOME": str(self.home / ".cache"),
            "XDG_STATE_HOME": str(self.home / ".local/state"),
            # Not inherited: a Plasma session puts ~/.config/kdedefaults and other per-user
            # directories in these, which would leak the user's theme into the session
            "XDG_DATA_DIRS": "/usr/local/share:/usr/share",
            "XDG_CONFIG_DIRS": "/etc/xdg",
            "XDG_SESSION_TYPE": "wayland",
            "XDG_CURRENT_DESKTOP": "KDE",
            "KDE_SESSION_VERSION": "6",
            "WAYLAND_DISPLAY": WAYLAND_SOCKET,
            "QT_QPA_PLATFORM": "wayland",
            "DBUS_SESSION_BUS_ADDRESS": f"unix:path={self.runtime_dir / 'bus'}",
            # The system bus of the session (start_system_bus), never the
            # real one: nothing in a test may suspend or power off the
            # computer, mount its drives, ...
            "DBUS_SYSTEM_BUS_ADDRESS": f"unix:path={self.runtime_dir / 'system_bus'}",
            # Qt logs to the systemd journal when stderr is not a terminal; keep logs in the session
            "QT_FORCE_STDERR_LOGGING": "1",
        })
        return env

    def stop(self):
        for pid in reversed(self.state.get("children", [])):
            self._kill_group(pid)
        for name in ("kwin", "dbus"):
            pid = self.state.get("pids", {}).get(name)
            if pid:
                self._kill_group(pid)
        self.state["children"] = []
        self.state["pids"] = {}
        self._save()

    @staticmethod
    def _kill_group(pid):
        for sig in (signal.SIGTERM, signal.SIGKILL):
            try:
                os.killpg(pid, sig)
            except (ProcessLookupError, PermissionError):
                return
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                try:
                    os.kill(pid, 0)
                    # Reap if it is our own child
                    try:
                        os.waitpid(pid, os.WNOHANG)
                    except ChildProcessError:
                        pass
                except ProcessLookupError:
                    return
                time.sleep(0.05)

    def _save(self):
        (self.dir / STATE_FILE).write_text(json.dumps(self.state, indent=2))

    @property
    def env(self):
        return dict(self.state["env"])

    # --- processes -------------------------------------------------------

    def _command(self, command):
        """Wraps a command so that it runs inside the session's namespaces, if any."""
        namespace_pid = self.state.get("namespace_pid")
        if not namespace_pid:
            return list(command)
        # Entering a mount namespace resets the working directory to its root
        return ["nsenter", "--target", str(namespace_pid), "--user", "--mount", "--preserve-credentials",
                f"--wd={os.getcwd()}", "--", *command]

    def spawn(self, command, log_name=None, extra_env=None):
        """Starts a background process inside the session; returns its Popen."""
        env = self.env
        env.update(extra_env or {})
        log_name = log_name or Path(command[0]).name
        with open(self.logs / f"{log_name}.log", "ab") as log:
            process = subprocess.Popen(self._command(command), env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        self.state["children"].append(process.pid)
        self._save()
        return process

    def run(self, command, extra_env=None, timeout=60, check=True):
        env = self.env
        env.update(extra_env or {})
        return subprocess.run(self._command(command), env=env, timeout=timeout, check=check, capture_output=True, text=True)

    # --- compositor ------------------------------------------------------

    def _dbus_call(self, destination, path, method, *args):
        return self.run(["gdbus", "call", "--session", "--dest", destination, "--object-path", path, "--method", method, *args], timeout=20)

    def _kwin_on_bus(self):
        result = self.run(["gdbus", "call", "--session", "--dest", "org.freedesktop.DBus", "--object-path", "/org/freedesktop/DBus",
                           "--method", "org.freedesktop.DBus.NameHasOwner", "org.kde.KWin"], check=False, timeout=5)
        return result.returncode == 0 and "true" in result.stdout

    def kwin_script(self, source, timeout=10):
        """Runs a KWin script; returns the values it reported via report(value)."""
        token = uuid.uuid4().hex
        marker = f"{SCRIPT_MARKER}-{token}"
        wrapped = (
            f"function report(value) {{ print('{marker} ' + JSON.stringify(value)); }}\n"
            f"{source}\n"
            f"print('{marker}-END');\n")
        script_file = self.dir / f"script-{token}.js"
        script_file.write_text(wrapped)
        plugin = f"wlh-{token}"
        log_file = self.logs / "kwin.log"
        try:
            result = self._dbus_call("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.loadScript", str(script_file), plugin)
            script_id = result.stdout.strip().strip("(),")
            self._dbus_call("org.kde.KWin", f"/Scripting/Script{script_id}", "org.kde.kwin.Script.run")
            values = []

            def finished():
                values.clear()
                done = False
                for line in log_file.read_text(errors="replace").splitlines():
                    position = line.find(marker)
                    if position < 0:
                        continue
                    payload = line[position + len(marker):]
                    if payload.startswith("-END"):
                        done = True
                    elif payload.startswith(" "):
                        values.append(json.loads(payload[1:]))
                return done

            _wait_for(finished, timeout, "the KWin script to finish")
            return values
        finally:
            self._dbus_call("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting.unloadScript", plugin)
            script_file.unlink(missing_ok=True)

    def windows(self):
        """Returns normal windows and dialogs with their frame and client geometry (logical pixels)."""
        values = self.kwin_script("""
            const result = [];
            for (const w of workspace.windowList()) {
                if (!w.normalWindow && !w.dialog) continue;
                const f = w.frameGeometry, c = w.clientGeometry;
                result.push({caption: w.caption, resourceClass: w.resourceClass, pid: w.pid, active: w.active,
                             frame: {x: f.x, y: f.y, width: f.width, height: f.height},
                             client: {x: c.x, y: c.y, width: c.width, height: c.height}});
            }
            report(result);
        """)
        return values[0] if values else []

    def wait_for_window(self, caption, timeout=20):
        found = []

        def present():
            found[:] = [w for w in self.windows() if w["caption"] == caption]
            return bool(found)

        _wait_for(present, timeout, f"window '{caption}'")
        return found[0]

    def place_window(self, caption, x, y, width, height):
        """Moves and resizes a window (frame geometry); returns its new geometry."""
        self.kwin_script(f"""
            for (const w of workspace.windowList()) {{
                if (w.caption === {json.dumps(caption)}) {{
                    w.frameGeometry = {{x: {x}, y: {y}, width: {width}, height: {height}}};
                }}
            }}
        """)
        expected = (x, y, width, height)

        def placed():
            window = next((w for w in self.windows() if w["caption"] == caption), None)
            frame = window and window["frame"]
            return bool(frame) and (frame["x"], frame["y"], frame["width"], frame["height"]) == expected

        _wait_for(placed, 10, f"window '{caption}' to be placed")
        return next(w for w in self.windows() if w["caption"] == caption)

    def screenshot(self, path):
        path = Path(path).resolve()
        path.unlink(missing_ok=True)
        self.run(["spectacle", "--background", "--nonotify", "--fullscreen", "--output", str(path)], timeout=30)
        _wait_for(lambda: path.exists() and path.stat().st_size > 0, 10, "the screenshot")
        return path

    def input(self, *commands):
        self.run([_default_tool("wlh-input"), *[str(c) for c in commands]], timeout=60)

    # --- system bus ------------------------------------------------------

    def start_system_bus(self):
        """Starts the system bus of the session (DBUS_SYSTEM_BUS_ADDRESS of its
        environment), for fake system services: the applications of a session
        never reach the real system bus."""
        if self.state.get("system_bus"):
            return
        socket_path = self.runtime_dir / "system_bus"
        config = self.dir / "dbus-system.conf"
        config.write_text(f"""<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>system</type>
  <listen>unix:path={socket_path}</listen>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
""")
        process = self.spawn(["dbus-daemon", f"--config-file={config}", "--nofork", "--nopidfile"], log_name="dbus-system")
        self._system_bus = process
        self.state["system_bus"] = process.pid
        self._save()
        _wait_for(lambda: socket_path.exists() or process.poll() is not None, 10, "the system bus")
        if process.poll() is not None:
            raise HarnessError(f"the system bus exited; see {self.logs / 'dbus-system.log'}")

    # --- accessibility ---------------------------------------------------

    def start_accessibility(self):
        """Starts the AT-SPI registry, so that applications started later
        with ACCESSIBILITY_ENV can be inspected (accessibles). The session
        bus activates the accessibility bus; the registry is started here,
        as the accessibility bus cannot activate it without systemd."""
        if self.state.get("atspi"):
            return
        process = self.spawn(["/usr/lib/at-spi2-registryd"], log_name="at-spi2-registryd")
        # Ended with the other children of the session
        self._registry = process
        self.state["atspi"] = process.pid
        self._save()
        _wait_for(lambda: "org.a11y.atspi.Registry" in (self.logs / "at-spi2-registryd.log").read_text(errors="replace")
                  or process.poll() is not None, 20, "the AT-SPI registry")
        if process.poll() is not None:
            raise HarnessError(f"at-spi2-registryd exited; see {self.logs / 'at-spi2-registryd.log'}")

    def accessibles(self, application, name=None, role=None):
        """The accessible objects of an application (wlh-atspi.py), with
        their extents in the client area of its window."""
        command = [sys.executable, str(Path(__file__).with_name("wlh-atspi.py")), application]
        if name is not None:
            command += ["--name", name]
        if role is not None:
            command += ["--role", role]
        result = self.run(command, timeout=60)
        return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]

    def wait_for_accessible(self, application, name, role=None, timeout=20):
        """The first showing accessible object of the name (and role)."""
        found = []

        def present():
            found[:] = [a for a in self.accessibles(application, name, role) if a["showing"] and a["width"] > 0]
            return bool(found)

        _wait_for(present, timeout, f"the accessible '{name}' of {application}")
        return found[0]

    def do_accessible_action(self, application, name, action, role=None):
        command = [sys.executable, str(Path(__file__).with_name("wlh-atspi.py")), application, "--name", name, "--action", action]
        if role is not None:
            command += ["--role", role]
        self.run(command, timeout=60)


# Applications started with it in a session can be inspected over AT-SPI
ACCESSIBILITY_ENV = {"QT_LINUX_ACCESSIBILITY_ALWAYS_ON": "1"}


def _main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-s", "--session", default=os.environ.get("WLH_SESSION"), help="session directory")
    commands = parser.add_subparsers(dest="command", required=True)
    start = commands.add_parser("start")
    start.add_argument("--width", type=int, default=1280)
    start.add_argument("--height", type=int, default=800)
    start.add_argument("--scale", type=float, default=1.0)
    spawn = commands.add_parser("spawn")
    spawn.add_argument("--log")
    spawn.add_argument("argv", nargs=argparse.REMAINDER)
    run = commands.add_parser("run")
    run.add_argument("argv", nargs=argparse.REMAINDER)
    shot = commands.add_parser("shot")
    shot.add_argument("file")
    commands.add_parser("windows")
    place = commands.add_parser("place")
    place.add_argument("caption")
    for name in ("x", "y", "width", "height"):
        place.add_argument(name, type=int)
    input_parser = commands.add_parser("input")
    input_parser.add_argument("argv", nargs=argparse.REMAINDER)
    commands.add_parser("env")
    commands.add_parser("stop")
    args = parser.parse_args()

    def strip_separator(argv):
        return argv[1:] if argv and argv[0] == "--" else argv

    if args.command == "start":
        session = Session.create(args.width, args.height, args.scale)
        print(session.dir)
        return 0
    if not args.session:
        parser.error("a session directory is required (-s DIR or WLH_SESSION)")
    session = Session(args.session)
    if args.command == "spawn":
        process = session.spawn(strip_separator(args.argv), args.log)
        print(process.pid)
    elif args.command == "run":
        result = session.run(strip_separator(args.argv), check=False)
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        return result.returncode
    elif args.command == "shot":
        print(session.screenshot(args.file))
    elif args.command == "windows":
        print(json.dumps(session.windows(), indent=2))
    elif args.command == "place":
        print(json.dumps(session.place_window(args.caption, args.x, args.y, args.width, args.height), indent=2))
    elif args.command == "input":
        session.input(*strip_separator(args.argv))
    elif args.command == "env":
        for name, value in session.env.items():
            print(f"export {name}='{value}'")
    elif args.command == "stop":
        session.stop()
    return 0


if __name__ == "__main__":
    try:
        sys.exit(_main())
    except HarnessError as error:
        print(f"wlharness: {error}", file=sys.stderr)
        sys.exit(1)
