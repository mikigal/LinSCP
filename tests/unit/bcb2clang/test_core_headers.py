"""The core's headers, converted by bcb2clang, parse against the RTL shim
and the platform layer (milestone M1). The accessors generated for __property declarations are
compiled too, so their getters, setters and fields are checked as well."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

import bcb2clang

ROOT = Path(__file__).resolve().parents[3]
CORE = ROOT / 'source' / 'core'

# Headers that need third-party libraries not built yet, and the header they miss
THIRD_PARTY = {
    'PuttyIntf.h': 'putty.h',        # M3
    'NeonIntf.h': 'ne_uri.h',        # M4
    'WebDAVFileSystem.h': 'ne_uri.h',  # M4
}


class CoreHeadersParseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='openscp-core-headers-')
        cls.converted = Path(cls.temp.name) / 'core'
        cls.converted.mkdir()
        for path in sorted(CORE.glob('*.h')):
            text = path.read_bytes().decode('utf-8', errors='surrogateescape')
            converted = bcb2clang.convert_source(text)
            (cls.converted / path.name).write_bytes(converted.encode('utf-8', errors='surrogateescape'))

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def compile(self, includes):
        unit = Path(self.temp.name) / 'unit.cpp'
        unit.write_text(''.join('#include <%s>\n' % name for name in includes))
        command = [
            os.environ.get('OPENSCP_CXX', 'clang++'),
            '-std=c++17', '-fdeclspec', '-fsyntax-only',
            '-Wall', '-Wextra', '-Werror',
            # Borland pragmas (#pragma warn); MakeMethod's designated initializers (to be reworked in M2)
            '-Wno-unknown-pragmas', '-Wno-reorder-init-list',
            '-I', str(self.converted),
            '-I', str(ROOT / 'source' / 'rtl' / 'include'),
            '-I', str(ROOT / 'source' / 'linux' / 'include'),
            '-I', str(ROOT / 'source' / 'resource'),
            str(unit),
        ]
        return subprocess.run(command, capture_output=True, text=True)

    def test_headers_parse(self):
        headers = [path.name for path in sorted(CORE.glob('*.h'))
                   if path.name not in THIRD_PARTY and path.name != 'CorePCH.h']
        self.assertGreater(len(headers), 25)
        # Like every core source file: the precompiled header first
        result = self.compile(['CorePCH.h'] + headers)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_rtl_headers_are_self_contained(self):
        headers = sorted(path.name for path in (ROOT / 'source' / 'rtl' / 'include').iterdir() if path.is_file())
        self.assertIn('vcl.h', headers)
        for header in headers:
            with self.subTest(header=header):
                result = self.compile([header])
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_third_party_headers_only_miss_their_libraries(self):
        for header, dependency in THIRD_PARTY.items():
            with self.subTest(header=header):
                result = self.compile(['CorePCH.h', header])
                self.assertNotEqual(result.returncode, 0)
                errors = [line for line in result.stderr.splitlines() if 'error:' in line]
                self.assertEqual(len(errors), 1, result.stderr)
                self.assertIn("'%s' file not found" % dependency, errors[0])


if __name__ == '__main__':
    unittest.main()
