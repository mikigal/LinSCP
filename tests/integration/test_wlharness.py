"""End-to-end check of the Wayland test harness (milestone M0).

Starts an isolated headless KWin, runs probe windows in it, and verifies that
screenshots, window placement, injected pointer/keyboard input and drag & drop
between two applications all work.
"""

import json
import os
import shutil
import time
import unittest
from pathlib import Path

from PIL import Image

import wlharness

PROBE_COLOR = (0x20, 0xA0, 0x60)


def probe_binary():
    return wlharness._default_tool("wlh-probe")


class HarnessTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.session = wlharness.Session.create(width=1280, height=800)
        cls.failed = False

    @classmethod
    def tearDownClass(cls):
        cls.session.stop()
        if cls.failed:
            print(f"keeping session {cls.session.dir} for inspection")
        else:
            shutil.rmtree(cls.session.dir, ignore_errors=True)

    def run(self, result=None):
        result = result or self.defaultTestResult()
        problems = len(result.failures) + len(result.errors)
        super().run(result)
        if len(result.failures) + len(result.errors) > problems:
            type(self).failed = True
        return result

    def start_probe(self, title, *args, env=None):
        log = self.session.logs / f"{title}.events"
        self.session.spawn([probe_binary(), "--title", title, "--log", str(log), *args], log_name=title, extra_env=env)
        self.wait_event(log, lambda e: e["event"] == "shown")
        self.session.wait_for_window(title)
        return log

    @staticmethod
    def events(log):
        if not log.exists():
            return []
        return [json.loads(line) for line in log.read_text().splitlines() if line.strip()]

    def wait_event(self, log, predicate, timeout=15):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for event in self.events(log):
                if predicate(event):
                    return event
            time.sleep(0.05)
        self.fail(f"event not observed in {log}; got {self.events(log)}")

    @staticmethod
    def center(geometry):
        return geometry["x"] + geometry["width"] // 2, geometry["y"] + geometry["height"] // 2

    def test_screenshot_shows_placed_window(self):
        self.start_probe("probe-shot")
        window = self.session.place_window("probe-shot", 40, 40, 400, 300)
        shot = self.session.screenshot(self.session.logs / "probe-shot.png")
        client = window["client"]
        with Image.open(shot) as image:
            # Away from the caption the probe draws in the middle
            pixel = image.convert("RGB").getpixel((client["x"] + 20, client["y"] + 20))
        for actual, expected in zip(pixel, PROBE_COLOR):
            self.assertAlmostEqual(actual, expected, delta=8, msg=f"pixel {pixel} in {shot}")

    def test_pointer_and_keyboard_input(self):
        log = self.start_probe("probe-input")
        window = self.session.place_window("probe-input", 480, 40, 400, 300)
        client = window["client"]
        self.session.input("move", client["x"] + 50, client["y"] + 60, "click")
        press = self.wait_event(log, lambda e: e["event"] == "mouse_press")
        self.assertAlmostEqual(press["x"], 50, delta=1)
        self.assertAlmostEqual(press["y"], 60, delta=1)
        self.assertEqual(press["button"], "left")
        self.session.input("type", "ok")
        self.wait_event(log, lambda e: e["event"] == "key_press" and e["text"] == "o")
        self.wait_event(log, lambda e: e["event"] == "key_press" and e["text"] == "k")

    def test_drag_and_drop_between_applications(self):
        payload = self.session.dir / "dragged.txt"
        payload.write_text("drag me")
        self.start_probe("probe-source", "--drag-source", str(payload))
        target_log = self.start_probe("probe-target")
        source = self.session.place_window("probe-source", 40, 420, 400, 300)
        target = self.session.place_window("probe-target", 600, 420, 400, 300)
        start = self.center(source["client"])
        end = self.center(target["client"])
        self.session.input("drag", start[0], start[1], end[0], end[1], 30)
        drop = self.wait_event(target_log, lambda e: e["event"] == "drop")
        self.assertIn(payload.as_uri(), drop["urls"])
        self.assertIn("text/uri-list", drop["formats"])

    def test_accessibility(self):
        self.session.start_accessibility()
        self.start_probe("probe-a11y", env=wlharness.ACCESSIBILITY_ENV)
        window = self.session.place_window("probe-a11y", 40, 40, 400, 300)
        found = self.session.wait_for_accessible("wlh-probe", "probe-a11y")
        # In the client area of the window
        self.assertEqual((found["width"], found["height"]), (window["client"]["width"], window["client"]["height"]))


if __name__ == "__main__":
    unittest.main()
