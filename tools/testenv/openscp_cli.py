#!/usr/bin/env python3
"""Runs openscp-cli in an isolated environment, for the L2 tests.

The process gets its own HOME, XDG base directories and TMPDIR, the "C.UTF-8"
locale and the UTC time zone, and nothing of the user's session: no SSH
agent (unless a test passes the socket of its own agent), no display.
LeakSanitizer (in sanitizer builds) gets the suppressions of known upstream
leaks (lsan-suppressions.txt).
"""

import os
import subprocess
from pathlib import Path

LSAN_SUPPRESSIONS = Path(__file__).resolve().with_name('lsan-suppressions.txt')


class CliResult:
    def __init__(self, completed):
        self.returncode = completed.returncode
        self.stdout_bytes = completed.stdout
        self.stdout = completed.stdout.decode('utf-8', errors='surrogateescape')
        self.stderr = completed.stderr.decode('utf-8', errors='surrogateescape')

    @property
    def output(self):
        return self.stdout + self.stderr

    def __repr__(self):
        return 'exit code %d\n--- stdout:\n%s--- stderr:\n%s' % (self.returncode, self.stdout, self.stderr)


class OpenScpCli:
    def __init__(self, binary, directory):
        self.binary = str(binary)
        self.dir = Path(directory).resolve()
        self.home = self.dir / 'home'
        self.tmp = self.dir / 'tmp'
        for path in (self.home, self.tmp):
            path.mkdir(parents=True, exist_ok=True)

    @property
    def config_file(self):
        """The default configuration file."""
        return self.home / '.config' / 'openscp' / 'openscp.ini'

    def environment(self, extra=None):
        env = {
            'HOME': str(self.home),
            'XDG_CONFIG_HOME': str(self.home / '.config'),
            'XDG_DATA_HOME': str(self.home / '.local' / 'share'),
            'XDG_CACHE_HOME': str(self.home / '.cache'),
            'TMPDIR': str(self.tmp),
            'PATH': os.environ.get('PATH', '/usr/bin:/bin'),
            'LANG': 'C.UTF-8',
            'TZ': 'UTC',
            'LSAN_OPTIONS': 'suppressions=%s:print_suppressions=0' % LSAN_SUPPRESSIONS,
        }
        for name in ('ASAN_OPTIONS', 'UBSAN_OPTIONS'):
            if name in os.environ:
                env[name] = os.environ[name]
        env.update(extra or {})
        return env

    def run(self, *args, input=None, env=None, timeout=120):
        """Runs the binary with the arguments; input is bytes or str."""
        if isinstance(input, str):
            input = input.encode()
        completed = subprocess.run(
            [self.binary, *map(str, args)], input=input, capture_output=True, timeout=timeout,
            env=self.environment(env), stdin=(subprocess.DEVNULL if input is None else None))
        return CliResult(completed)

    def popen(self, *args, env=None, **kwargs):
        return subprocess.Popen([self.binary, *map(str, args)], env=self.environment(env), **kwargs)

    def commands(self, *commands, args=(), **kwargs):
        """Runs script commands (/command), with the extra switches."""
        return self.run(*args, '/command', *commands, **kwargs)
