"""Checks that the local unprivileged test sshd works (milestone M0)."""

import os
import shutil
import tempfile
import unittest
from pathlib import Path

from local_sshd import LocalSshd


class LocalSshdTest(unittest.TestCase):
    def setUp(self):
        self.dir = Path(tempfile.mkdtemp(prefix="openscp-sshd-test-"))
        self.server = LocalSshd(self.dir).start()

    def run(self, result=None):
        result = result or self.defaultTestResult()
        problems = len(result.failures) + len(result.errors)
        super().run(result)
        if len(result.failures) + len(result.errors) > problems:
            print(f"keeping {self.dir} for inspection")
        else:
            shutil.rmtree(self.dir, ignore_errors=True)
        return result

    def tearDown(self):
        self.server.stop()

    def test_shell_command(self):
        result = self.server.ssh("echo", "openscp")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "openscp")

    def test_sftp_round_trip(self):
        source = self.dir / "source.bin"
        source.write_bytes(os.urandom(256 * 1024))
        remote = self.server.root / "uploaded.bin"
        downloaded = self.dir / "downloaded.bin"
        result = self.server.sftp_batch(f"put {source} {remote}\nget {remote} {downloaded}\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(downloaded.read_bytes(), source.read_bytes())

    def test_unknown_host_key_is_rejected(self):
        self.server.known_hosts.write_text("")
        result = self.server.ssh("true")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Host key verification failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
