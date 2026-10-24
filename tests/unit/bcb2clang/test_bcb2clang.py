"""Unit tests of the C++Builder dialect converter (tools/bcb2clang)."""

import unittest
from pathlib import Path

import bcb2clang

ROOT = Path(__file__).resolve().parents[3]


def convert(text):
    return bcb2clang.convert_source(text)


def squeeze(text):
    return ' '.join(text.split())


class TokenizerTest(unittest.TestCase):
    def test_round_trip_of_upstream_sources(self):
        files = sorted((ROOT / 'source' / 'core').glob('*.[hc]*'))
        self.assertTrue(files)
        for path in files:
            text = path.read_bytes().decode('utf-8', errors='surrogateescape')
            with self.subTest(path=path.name):
                self.assertEqual(bcb2clang.untokenize(bcb2clang.tokenize(text)), text)

    def test_token_kinds(self):
        tokens = [t for t in bcb2clang.tokenize('x = L"a\\"b" + \'c\'; // __property\n') if t.significant]
        self.assertEqual([t.kind for t in tokens], ['ident', 'punct', 'string', 'punct', 'char', 'punct'])

    def test_preprocessor_lines(self):
        tokens = bcb2clang.tokenize('#define A(x) \\\n  __closure x\nint y;\n')
        self.assertEqual(tokens[0].kind, 'pp')
        self.assertEqual(tokens[0].text, '#define A(x) \\\n  __closure x')

    def test_keywords_in_strings_and_comments_are_kept(self):
        text = 'const char * S = "__property __closure"; /* __property X = { read = Y }; */\n'
        self.assertEqual(convert(text), text)


class ClosureTest(unittest.TestCase):
    def test_typedef(self):
        self.assertEqual(
            convert('typedef void __fastcall (__closure *TNotifyEvent)(TObject * Sender);'),
            'typedef TMethodPtr<void(TObject * Sender)> TNotifyEvent;')

    def test_without_fastcall_and_spacing(self):
        self.assertEqual(
            convert('typedef void (__closure* TNeonTlsInit)(struct ssl_st * Ssl, ne_session * Session);'),
            'typedef TMethodPtr<void(struct ssl_st * Ssl, ne_session * Session)> TNeonTlsInit;')

    def test_complex_return_type(self):
        self.assertEqual(
            convert('typedef const std::vector<int> * __fastcall (__closure * TGet)();'),
            'typedef TMethodPtr<const std::vector<int> *()> TGet;')

    def test_multi_line_keeps_line_count(self):
        source = 'typedef int __fastcall (__closure *TFileOperationEvent)\n  (void * Param1, void * Param2);\nint x;\n'
        result = convert(source)
        self.assertEqual(result.count('\n'), source.count('\n'))
        self.assertEqual(squeeze(result), 'typedef TMethodPtr<int(void * Param1, void * Param2)> TFileOperationEvent; int x;')

    def test_parameters_spanning_lines(self):
        source = 'typedef void __fastcall (__closure *TEvent)(\n  int A,\n  int B);\n'
        result = convert(source)
        self.assertEqual(result, 'typedef TMethodPtr<void(\n  int A,\n  int B)> TEvent;\n')

    def test_parameter_and_member(self):
        self.assertEqual(
            convert('void Add(const UnicodeString & S, void __fastcall (__closure *f)(int Type, const UnicodeString & Line));'),
            'void Add(const UnicodeString & S, TMethodPtr<void(int Type, const UnicodeString & Line)> f);')
        self.assertEqual(
            convert('class A {\npublic:\n  bool __fastcall (__closure * FOnQuery)(int);\n};'),
            'class A {\npublic:\n  TMethodPtr<bool(int)> FOnQuery;\n};')

    def test_cast(self):
        self.assertEqual(
            convert('X = (void (__closure *)(int))Y;'),
            'X = (TMethodPtr<void(int)>)Y;')


class PropertyTest(unittest.TestCase):
    def test_methods(self):
        source = 'class A {\npublic:\n  __property int Count = { read = GetCount, write = SetCount };\nprivate:\n  int GetCount();\n  void SetCount(int V);\n};'
        result = convert(source)
        self.assertIn('int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); }', result)
        self.assertIn('void __property_set_Count(int Value) { SetCount(Value); }', result)
        self.assertIn('__declspec(property(get = __property_get_Count, put = __property_set_Count)) int Count;', result)

    def test_fields(self):
        source = 'class A {\npublic:\n  __property TStrings * List = { read=FList, write=FList };\nprivate:\n  TStrings * FList;\n};'
        result = convert(source)
        self.assertIn('TStrings * __property_get_List() const { return FList; }', result)
        self.assertIn('void __property_set_List(TStrings * Value) { FList = Value; }', result)

    def test_field_detection(self):
        # FName is initialized in the constructor before it is declared
        source = (
            'class A {\npublic:\n  A() : FName(), FData(0) {}\n'
            '  __property UnicodeString Name = { read = FName };\n'
            '  __property int Data = { read = FData };\n'
            '  __property int Other = { read = FOther };\n'
            'private:\n  UnicodeString FName;\n  int FData;\n  int FOther();\n};')
        result = convert(source)
        self.assertIn('return FName;', result)
        self.assertIn('return FData;', result)
        self.assertIn('return ::rtl::Mutable(this)->FOther();', result)

    def test_inherited_names_follow_the_naming_convention(self):
        result = convert('class B : public A {\n  __property int X = { read = FX, write = SetX };\n};')
        self.assertIn('return FX;', result)
        self.assertIn('SetX(Value);', result)

    def test_getter_return_type(self):
        source = 'class A {\n  const TList * GetList();\npublic:\n  __property TList * List = { read = GetList };\n};'
        self.assertIn('const TList * __property_get_List() const', convert(source))
        # Overloaded getters: the declared type
        source = 'class A {\n  int Get();\n  bool Get(int);\npublic:\n  __property long X = { read = Get };\n};'
        self.assertIn('long __property_get_X() const', convert(source))

    def test_index_specifier(self):
        source = 'class A {\n  __property TStrings * IncludeStr = { read = GetStr, index = MASK_INDEX(false, true) };\n};'
        result = convert(source)
        self.assertIn('return ::rtl::Mutable(this)->GetStr(MASK_INDEX(false, true));', result)
        self.assertIn('__declspec(property(get = __property_get_IncludeStr)) TStrings * IncludeStr;', result)

    def test_array_property(self):
        source = 'class A {\n  __property bool Opened[UnicodeString Index] = { read = GetOpened, write = SetOpened };\n};'
        result = convert(source)
        self.assertIn('bool __property_get_Opened(UnicodeString Index) const { return ::rtl::Mutable(this)->GetOpened(Index); }', result)
        self.assertIn('void __property_set_Opened(UnicodeString Index, bool Value) { SetOpened(Index, Value); }', result)
        self.assertIn('__declspec(property(get = __property_get_Opened, put = __property_set_Opened)) bool Opened[];', result)

    def test_two_dimensional(self):
        result = convert('class A {\n  __property int Cell[int X][int Y] = { read = GetCell };\n};')
        self.assertIn('int __property_get_Cell(int X, int Y) const { return ::rtl::Mutable(this)->GetCell(X, Y); }', result)
        self.assertIn('int Cell[][];', result)

    def test_ignored_specifiers_and_redeclaration(self):
        result = convert('class A {\n  __property int X = { read = GetX, default = 0, stored = false, nodefault };\n  __property Y;\n};')
        self.assertIn('__declspec(property(get = __property_get_X)) int X;', result)
        self.assertNotIn('__property Y', result)

    def test_multi_line_keeps_line_count(self):
        source = 'class A {\n  __property int X = {\n    read = GetX,\n    write = SetX };\n  int Y;\n};\n'
        result = convert(source)
        self.assertEqual(result.count('\n'), source.count('\n'))
        self.assertTrue(result.endswith('  int Y;\n};\n'))

    def test_errors(self):
        with self.assertRaises(bcb2clang.ConversionError):
            convert('class A { __property int X = { default = 0 }; };')


class PreprocessorTest(unittest.TestCase):
    def test_include_paths(self):
        self.assertEqual(convert('#include <proxy\\proxy.h>\n'), '#include <proxy/proxy.h>\n')
        self.assertEqual(convert('#include "windows\\platform.h" // x\n'), '#include "windows/platform.h" // x\n')
        self.assertEqual(convert('#define P "a\\b"\n'), '#define P "a\\b"\n')


class CoreHeadersTest(unittest.TestCase):
    def test_line_counts_are_kept(self):
        for path in sorted((ROOT / 'source' / 'core').glob('*.h')):
            text = path.read_bytes().decode('utf-8', errors='surrogateescape')
            with self.subTest(path=path.name):
                converted = convert(text)
                self.assertEqual(converted.count('\n'), text.count('\n'))
                self.assertNotIn('__property ', converted)
                self.assertNotIn('__closure', converted)
                # Converting again changes nothing
                self.assertEqual(convert(converted), converted)


if __name__ == '__main__':
    unittest.main()
