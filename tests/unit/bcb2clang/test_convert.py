"""Tests of the tree conversion (tools/bcb2clang/convert.py)."""

import tempfile
import unittest
from pathlib import Path

import convert


class ConvertTreeTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='openscp-convert-')
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode())

    def read(self, name):
        return (self.root / name).read_bytes().decode()

    def test_tree(self):
        # Method references across files of the core
        self.write('source/core/A.h', 'class TA {\r\n  void M();\r\n  void N();\r\n};\r\n')
        self.write('source/core/A.cpp',
                   '#include "proxy\\proxy.h"\r\nvoid TA::N()\r\n{\r\n  try { Run(M); } __finally { }\r\n}\r\n')
        self.write('source/putty/ssh/x.c', '#include "ssh\\gss.h"\r\nint __finally;\r\n')
        self.write('source/windows/W.cpp', '#include "a\\b.h"\r\n')
        self.assertEqual(convert.convert_tree(self.root), [])
        self.assertEqual(
            self.read('source/core/A.cpp'),
            '#include "proxy/proxy.h"\r\nvoid TA::N()\r\n{\r\n'
            '  { ::rtl::TFinally Finally([&]() { }); try { Run(::rtl::MethodPtr(this, &TA::M)); } '
            'catch (...) { Finally.Unwind(); } }\r\n}\r\n')
        # Only include paths outside of the core
        self.assertEqual(self.read('source/putty/ssh/x.c'), '#include "ssh/gss.h"\r\nint __finally;\r\n')
        # Directories that are not built are left alone
        self.assertEqual(self.read('source/windows/W.cpp'), '#include "a\\b.h"\r\n')

    def test_ported_files(self):
        self.write('source/core/A.h', 'class TA {\r\n  TNotifyEvent OnX;\r\n};\r\n')
        self.write('source/windows/GUIConfiguration.h', 'class TG {\r\n  void M(TObject *);\r\n};\r\n')
        upstream = 'void TG::M(TObject *)\r\n{\r\n  TA * A = new TA();\r\n  A->OnX = M;\r\n}\r\n'
        self.write('source/windows/GUIConfiguration.cpp', upstream)
        self.assertEqual(convert.convert_tree(self.root), [])
        # Converted copies, with method references resolved against the ported classes
        self.assertEqual(
            self.read('source/app/GUIConfiguration.cpp'),
            'void TG::M(TObject *)\r\n{\r\n  TA * A = new TA();\r\n  A->OnX = ::rtl::MethodPtr(this, &TG::M);\r\n}\r\n')
        self.assertEqual(self.read('source/app/GUIConfiguration.h'), 'class TG {\r\n  void M(TObject *);\r\n};\r\n')
        # The upstream files stay pristine; missing ported files are skipped
        self.assertEqual(self.read('source/windows/GUIConfiguration.cpp'), upstream)
        self.assertFalse((self.root / 'source/cli/ConsoleRunner.cpp').exists())

    def test_errors(self):
        self.write('source/core/B.cpp', 'void F() { try { } __finally { return; } }\n')
        errors = convert.convert_tree(self.root)
        self.assertEqual(len(errors), 1)
        self.assertIn('source/core/B.cpp: return in a __finally block', errors[0])


if __name__ == '__main__':
    unittest.main()
