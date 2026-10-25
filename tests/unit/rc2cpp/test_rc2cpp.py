"""Tests of the .rc string table generator (tools/rc2cpp)."""

import unittest
from pathlib import Path

import rc2cpp

ROOT = Path(__file__).resolve().parents[3]


class ParseTest(unittest.TestCase):
    def test_entries(self):
        text = (
            '#include "Texts.h"\r\n'
            '#define X 1\r\n'
            'STRINGTABLE\r\nBEGIN\r\n'
            '  // comment\r\n'
            '  FIRST, "One"\r\n'
            '  SECOND,\r\n    "Two\\n"\r\n    "lines"\r\n'
            '  THIRD "Say ""hi"" \\"x\\" \\\\ \\t"\r\n'
            'END\r\n\r\n'
            'STRINGTABLE\r\nBEGIN\r\n  FOURTH, "\u00a9 1997\u20132026"\r\nEND\r\n')
        self.assertEqual(rc2cpp.parse(text), [
            ('FIRST', 'One'),
            ('SECOND', 'Two\nlines'),
            ('THIRD', 'Say "hi" "x" \\ \t'),
            ('FOURTH', '\u00a9 1997\u20132026')])

    def test_errors(self):
        for text in [
                'STRINGTABLE\nBEGIN\n  A, "x"\n',
                'STRINGTABLE\nBEGIN\n  A,\nEND\n',
                'STRINGTABLE\nBEGIN\n  "x"\nEND\n',
                'STRINGTABLE\nBEGIN\n  A, "\\q"\nEND\n',
                'STRINGTABLE\nBEGIN\n  A, "x" + \nEND\n']:
            with self.subTest(text=text):
                with self.assertRaises(rc2cpp.RcError):
                    rc2cpp.parse(text)


class GenerateTest(unittest.TestCase):
    def test_literal(self):
        self.assertEqual(rc2cpp.cpp_literal('a"b\\c\n'), 'L"a\\"b\\\\c\\n"')
        self.assertEqual(rc2cpp.cpp_literal('\u00a9\U0001f600'), 'L"\\u00a9\\U0001f600"')
        self.assertEqual(rc2cpp.cpp_literal('\x01x'), 'L"\\x01""x"')

    def test_source(self):
        source = rc2cpp.generate('Texts.rc', '#include "Texts.h"\nSTRINGTABLE\nBEGIN\n  A, "\u00e9"\nEND\n')
        self.assertIn('#include "Texts.h"', source)
        self.assertIn('  { A, L"\\u00e9" },', source)
        self.assertIn('::rtl::RegisterResourceStrings(Strings,', source)
        # The names of the translations
        self.assertIn('  { A, "A" },', source)
        self.assertIn('::rtl::RegisterResourceStringNames(Names,', source)
        source.encode('ascii')

    def test_brand(self):
        source = rc2cpp.generate('Texts.rc', 'STRINGTABLE\nBEGIN\n  A, "WinSCP is winscp.com"\nEND\n',
                                 ('WinSCP', 'LinSCP'))
        self.assertIn('  { A, L"LinSCP is winscp.com" },', source)

    def test_project_files(self):
        for name in ['TextsCore1.rc', 'TextsCore2.rc', 'TextsWin1.rc', 'TextsFileZilla.rc']:
            with self.subTest(name=name):
                text = (ROOT / 'source' / 'resource' / name).read_bytes().decode('utf-8-sig')
                entries = rc2cpp.parse(text)
                self.assertGreater(len(entries), 20)
                identifiers = [identifier for identifier, _ in entries]
                self.assertEqual(len(identifiers), len(set(identifiers)))


if __name__ == '__main__':
    unittest.main()
