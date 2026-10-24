#!/usr/bin/env python3
"""Unprivileged OpenSSH server on 127.0.0.1 for local SFTP/SCP tests.

Runs the system sshd as the current user with its own host key, client key,
authorized_keys and known_hosts, so nothing from the user's ~/.ssh is used or
modified. Only public key authentication works (password authentication would
need root). For anything else, use the test VM (docs/port/PLAN.md, section 6.3).

Usable as a module (see LocalSshd) and as a command line tool:

    local_sshd.py start --dir DIR [--port N]   # prints KEY=VALUE lines
    local_sshd.py stop --dir DIR
"""

import argparse
import getpass
import os
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path


class SshdError(RuntimeError):
    pass


def _find_sshd():
    for candidate in (shutil.which("sshd"), "/usr/bin/sshd", "/usr/sbin/sshd"):
        if candidate and os.access(candidate, os.X_OK):
            # sshd re-executes itself and requires an absolute path
            return str(Path(candidate).resolve())
    raise SshdError("sshd not found")


def _free_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return probe.getsockname()[1]


class LocalSshd:
    def __init__(self, directory, port=0):
        self.dir = Path(directory).resolve()
        self.port_file = self.dir / "port"
        if port == 0 and self.port_file.exists():
            port = int(self.port_file.read_text())
        self.port = port or _free_port()
        self.host = "127.0.0.1"
        self.user = getpass.getuser()
        self.host_key = self.dir / "host_ed25519"
        self.client_key = self.dir / "client_ed25519"
        self.known_hosts = self.dir / "known_hosts"
        self.config = self.dir / "sshd_config"
        self.pid_file = self.dir / "sshd.pid"
        self.log_file = self.dir / "sshd.log"
        self.root = self.dir / "root"

    def start(self):
        self.dir.mkdir(parents=True, exist_ok=True)
        self.root.mkdir(exist_ok=True)
        for key in (self.host_key, self.client_key):
            if not key.exists():
                subprocess.run(["ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-C", "openscp-test", "-f", str(key)], check=True)
        (self.dir / "authorized_keys").write_text(Path(f"{self.client_key}.pub").read_text())
        host_public = Path(f"{self.host_key}.pub").read_text().split()
        self.known_hosts.write_text(f"[{self.host}]:{self.port} {host_public[0]} {host_public[1]}\n")
        self.port_file.write_text(str(self.port))
        self.config.write_text("\n".join([
            f"Port {self.port}",
            f"ListenAddress {self.host}",
            f"HostKey {self.host_key}",
            f"PidFile {self.pid_file}",
            f"AuthorizedKeysFile {self.dir / 'authorized_keys'}",
            "PubkeyAuthentication yes",
            "PasswordAuthentication no",
            "KbdInteractiveAuthentication no",
            "UsePAM no",
            "StrictModes no",
            "PermitUserEnvironment no",
            "Subsystem sftp internal-sftp",
            "LogLevel VERBOSE",
            "",
        ]))
        self.pid_file.unlink(missing_ok=True)
        subprocess.run([_find_sshd(), "-f", str(self.config), "-E", str(self.log_file)], check=True)
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            try:
                with socket.create_connection((self.host, self.port), timeout=0.5):
                    return self
            except OSError:
                time.sleep(0.05)
        raise SshdError(f"sshd did not start; see {self.log_file}")

    def stop(self):
        if not self.pid_file.exists():
            return
        try:
            pid = int(self.pid_file.read_text())
            os.kill(pid, signal.SIGTERM)
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                os.kill(pid, 0)
                time.sleep(0.05)
        except (ProcessLookupError, ValueError):
            pass
        self.pid_file.unlink(missing_ok=True)

    def client_options(self):
        """OpenSSH client options that ignore the user's own configuration and agent."""
        return [
            "-F", "/dev/null",
            "-i", str(self.client_key),
            "-o", "IdentitiesOnly=yes",
            "-o", "IdentityAgent=none",
            "-o", f"UserKnownHostsFile={self.known_hosts}",
            "-o", "StrictHostKeyChecking=yes",
            "-o", "BatchMode=yes",
        ]

    def ssh(self, *remote_command, **kwargs):
        command = ["ssh", *self.client_options(), "-p", str(self.port), f"{self.user}@{self.host}", *remote_command]
        return subprocess.run(command, capture_output=True, text=True, timeout=30, **kwargs)

    def sftp_batch(self, batch):
        command = ["sftp", *self.client_options(), "-P", str(self.port), "-b", "-", f"{self.user}@{self.host}"]
        return subprocess.run(command, input=batch, capture_output=True, text=True, timeout=30)

    def describe(self):
        return {
            "OPENSCP_TEST_SSH_HOST": self.host,
            "OPENSCP_TEST_SSH_PORT": str(self.port),
            "OPENSCP_TEST_SSH_USER": self.user,
            "OPENSCP_TEST_SSH_KEY": str(self.client_key),
            "OPENSCP_TEST_SSH_KNOWN_HOSTS": str(self.known_hosts),
            "OPENSCP_TEST_SSH_ROOT": str(self.root),
        }

    def __enter__(self):
        return self.start()

    def __exit__(self, *exc):
        self.stop()


def _main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command", choices=("start", "stop"))
    parser.add_argument("--dir", help="state directory (default: a new temporary directory)")
    parser.add_argument("--port", type=int, default=0)
    args = parser.parse_args()
    if args.command == "stop":
        if not args.dir:
            parser.error("stop requires --dir")
        LocalSshd(args.dir).stop()
        return 0
    directory = args.dir or tempfile.mkdtemp(prefix="openscp-sshd-")
    server = LocalSshd(directory, args.port).start()
    print(f"OPENSCP_TEST_SSH_DIR={server.dir}")
    for name, value in server.describe().items():
        print(f"{name}={value}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(_main())
    except SshdError as error:
        print(f"local_sshd: {error}", file=sys.stderr)
        sys.exit(1)
