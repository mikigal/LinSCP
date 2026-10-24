"""Unit tests of the C++Builder dialect converter (tools/bcb2clang)."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

import bcb2clang

ROOT = Path(__file__).resolve().parents[3]


def convert(text):
    return bcb2clang.convert_source(text)


def upstream_core_sources():
    """{name: text} of the pristine upstream core converted on the
    upstream-converted branch (read from git), or None."""
    def git(*args):
        return subprocess.run(['git', '-C', str(ROOT)] + list(args), capture_output=True, check=True).stdout
    try:
        # One parent: the initial conversion; two: previous conversion and upstream
        revision = git('rev-list', '--parents', '-n1', 'refs/heads/upstream-converted').split()[-1].decode()
        names = git('ls-tree', '--name-only', revision, 'source/core/').decode().split()
    except (OSError, subprocess.CalledProcessError):
        return None
    return {Path(name).name: git('show', '%s:%s' % (revision, name)).decode('utf-8', errors='surrogateescape')
            for name in names if Path(name).suffix in ('.h', '.cpp')}


def squeeze(text):
    return ' '.join(text.split())


class TokenizerTest(unittest.TestCase):
    def test_round_trip_of_core_sources(self):
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


class FinallyTest(unittest.TestCase):
    def test_block(self):
        source = (
            '  try\n  {\n    Use(List);\n  }\n'
            '  __finally\n  {\n    delete List;\n  }\n')
        self.assertEqual(convert(source), (
            '  { ::rtl::TFinally Finally([&]()\n  {\n    delete List;\n  }); try\n'
            '  {\n    Use(List);\n  }\n'
            '  catch (...) { Finally.Unwind(); } }\n'))

    def test_comments_and_single_statement_context(self):
        source = 'if (A) try { X(); } // done\n__finally /* c */ { Y(); }\nZ();\n'
        result = convert(source)
        self.assertEqual(result.count('\n'), source.count('\n'))
        self.assertEqual(
            squeeze(result),
            'if (A) { ::rtl::TFinally Finally([&]() /* c */ { Y(); }); try { X(); } // done '
            'catch (...) { Finally.Unwind(); } } Z();')

    def test_nested(self):
        source = (
            'try {\n'
            '  try { A(); } __finally { B(); }\n'
            '} __finally {\n'
            '  try { C(); } __finally { D(); }\n'
            '}\n')
        result = convert(source)
        self.assertNotIn('__finally', result)
        self.assertEqual(result.count('\n'), source.count('\n'))
        self.assertEqual(result.count('::rtl::TFinally'), 3)
        # The outer finally block, with its nested try, comes first
        self.assertLess(result.index('C()'), result.index('A()'))
        self.assertLess(result.index('D()'), result.index('A()'))

    def test_macro(self):
        source = '#define BLOCK(B) \\\r\n  if (Open()) try { B } __finally { Close(); }\r\nint X;\r\n'
        self.assertEqual(convert(source), (
            '#define BLOCK(B) \\\r\n  if (Open()) { ::rtl::TFinally Finally([&]() { Close(); }); try { B } '
            'catch (...) { Finally.Unwind(); } }\r\nint X;\r\n'))

    def test_errors(self):
        with self.assertRaisesRegex(bcb2clang.ConversionError, 'return in a __finally block'):
            convert('try { A(); } __finally { return; }')
        with self.assertRaises(bcb2clang.ConversionError):
            convert('{ A(); } __finally { B(); }')


class MethodReferenceTest(unittest.TestCase):
    CLASSES = (
        '#define CALL(X) X()\n'
        'class TBase : public TObject {\npublic:\n'
        '  void Handler(int X);\n  static void Static(int X);\n'
        '  __property TNotify OnEvent = { read = FOnEvent, write = FOnEvent };\n'
        '  TNotify FOnEvent;\n};\n'
        'class TOther {\npublic:\n  void Handler(int X);\n  void Other(int X);\n  int Value;\n};\n'
        'class TDerived : public TBase {\npublic:\n  void Run();\n  TOther * FOther;\n};\n')

    def run_body(self, body):
        text = self.CLASSES + 'void TDerived::Run()\n{\n' + body + '\n}\n'
        result = convert(text)
        self.assertEqual(result.count('\n'), text.count('\n'))
        return result.split('void TDerived::Run()\n{\n', 1)[1].rsplit('\n}\n', 1)[0]

    def test_own_methods(self):
        self.assertEqual(self.run_body('OnEvent = Handler;'), 'OnEvent = ::rtl::MethodPtr(this, &TDerived::Handler);')
        self.assertEqual(self.run_body('Register(&Handler, 1);'), 'Register(::rtl::MethodPtr(this, &TDerived::Handler), 1);')

    def test_methods_of_other_objects(self):
        self.assertEqual(self.run_body('Process(FOther->Other);'), 'Process(::rtl::MethodPtr(FOther, &TOther::Other));')
        # Handler is a method of more classes, the type of FOther tells which
        self.assertEqual(self.run_body('Process(&FOther->Handler);'), 'Process(::rtl::MethodPtr(FOther, &TOther::Handler));')
        self.assertEqual(
            self.run_body('TOther * Local = FOther;\nProcess(Local->Handler);'),
            'TOther * Local = FOther;\nProcess(::rtl::MethodPtr(Local, &TOther::Handler));')
        self.assertEqual(
            self.run_body('Process(FOther\n  ->Other);'), 'Process(::rtl::MethodPtr(FOther, &TOther::Other)\n);')
        self.assertEqual(
            self.run_body('TOther Object;\nProcess(&Object.Handler);'),
            'TOther Object;\nProcess(::rtl::MethodPtr(&Object, &TOther::Handler));')

    def test_macros(self):
        source = (
            'class TA {\n  void M();\n  void N();\n  TA * FA;\n};\n'
            '#define RUN(X) Run(M, X); Run(FA->M); Write ## X(M)\n'
            'void TA::N()\n{\n  RUN(1);\n}\n')
        self.assertIn(
            '#define RUN(X) Run(::rtl::MethodPtr(this, &TA::M), X); Run(::rtl::MethodPtr(FA, &TA::M)); Write ## X(M)\n',
            convert(source))

    def test_unchanged(self):
        for body in [
                'Handler(1);',
                'Run2(Static);',
                'X = A & Handler;',
                'int Handler = 1;\nUse(Handler);',
                'Use(FOther->Value);',
                'Use(FOther->Handler(1));',
                'CALL(Handler);',
                'Use(&TDerived::Handler);',
                'Use(Unknown.Handler);',
                'OnEvent = FOnEvent;']:
            with self.subTest(body=body):
                self.assertEqual(self.run_body(body), body)

    def test_inline_member_functions(self):
        result = convert('class TInline {\n  void A() { Set(B); }\n  void B();\n};\n')
        self.assertIn('void A() { Set(::rtl::MethodPtr(this, &TInline::B)); }', result)

    def test_class_table_across_files(self):
        table = bcb2clang.ClassTable()
        table.add_source('class TA {\n  void M();\n  void N();\n};\n')
        self.assertEqual(
            bcb2clang.convert_source('void TA::N()\n{\n  Run(M);\n}\n', table),
            'void TA::N()\n{\n  Run(::rtl::MethodPtr(this, &TA::M));\n}\n')


class LineEndingTest(unittest.TestCase):
    def test_crlf(self):
        source = '#define M(X) \\\r\n  X\r\ntypedef void __fastcall (__closure *TEvent)\r\n  (int A);\r\n'
        result = convert(source)
        self.assertTrue(result.startswith('#define M(X) \\\r\n  X\r\n'))
        self.assertEqual(result.count('\r\n'), source.count('\r\n'))
        self.assertNotRegex(result, '[^\r]\n')


FINALLY_PROGRAM = r'''
#include <rtl/Finally.h>
#include <cstdio>
#include <stdexcept>
#include <string>

static std::string Log;

static int ReturnInTry()
{
  int Value = 0;
  try
  {
    Value = 5;
    Log += "a";
    return 1;
  }
  __finally
  {
    // Sees the state left by the try block
    Log += std::to_string(Value);
  }
}

static void LoopJumps()
{
  for (int I = 0; I < 3; I++)
  {
    try
    {
      if (I == 0) continue;
      if (I == 2) break;
      Log += "c";
    }
    __finally
    {
      Log += "d";
    }
  }
}

static void ExceptionInTry()
{
  try
  {
    try { throw std::runtime_error("e"); }
    __finally { Log += "f"; }
  }
  catch (const std::exception & E) { Log += E.what(); }
}

static void ExceptionInFinallyReplacesTheCurrentOne()
{
  try
  {
    try { throw std::runtime_error("1"); }
    __finally { throw std::logic_error("2"); }
  }
  catch (const std::logic_error & E) { Log += E.what(); }
}

static void ExceptionInFinallyAfterNormalExit()
{
  try
  {
    try { Log += "g"; }
    __finally { throw std::logic_error("3"); }
  }
  catch (const std::logic_error & E) { Log += E.what(); }
}

int main()
{
  int Result = ReturnInTry();
  LoopJumps();
  ExceptionInTry();
  ExceptionInFinallyReplacesTheCurrentOne();
  ExceptionInFinallyAfterNormalExit();
  std::printf("%d %s\n", Result, Log.c_str());
  return 0;
}
'''


class FinallySemanticsTest(unittest.TestCase):
    def test_converted_program(self):
        with tempfile.TemporaryDirectory(prefix='openscp-finally-') as temp:
            source = Path(temp) / 'finally.cpp'
            source.write_text(convert(FINALLY_PROGRAM))
            program = Path(temp) / 'finally'
            subprocess.run([
                os.environ.get('OPENSCP_CXX', 'clang++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=address,undefined', '-I', str(ROOT / 'source' / 'rtl' / 'include'),
                str(source), '-o', str(program)], check=True)
            result = subprocess.run([str(program)], capture_output=True, text=True, check=True)
            self.assertEqual(result.stdout, '1 a5dcddfe2g3\n')


class PreprocessorTest(unittest.TestCase):
    def test_include_paths(self):
        self.assertEqual(convert('#include <proxy\\proxy.h>\n'), '#include <proxy/proxy.h>\n')
        self.assertEqual(convert('#include "windows\\platform.h" // x\n'), '#include "windows/platform.h" // x\n')
        self.assertEqual(convert('#define P "a\\b"\n'), '#define P "a\\b"\n')


class UpstreamCoreTest(unittest.TestCase):
    def test_conversion(self):
        sources = upstream_core_sources()
        if sources is None:
            self.skipTest('no upstream-converted branch')
        table = bcb2clang.ClassTable()
        for text in sources.values():
            table.add_source(text)
        table.add_rtl_headers(bcb2clang.RTL_HEADERS)
        self.assertGreater(len(table.classes), 100)
        for name, text in sorted(sources.items()):
            with self.subTest(path=name):
                self.assertEqual(bcb2clang.untokenize(bcb2clang.tokenize(text)), text)
                converted = bcb2clang.convert_source(text, table)
                self.assertEqual(converted.count('\n'), text.count('\n'))
                self.assertEqual(converted.count('\r\n'), text.count('\r\n'))
                keywords = {t.text for t in bcb2clang.tokenize(converted, preprocessor=False) if t.kind == 'ident'}
                self.assertFalse(keywords & {'__property', '__closure', '__finally'})
                # Converting again changes nothing
                self.assertEqual(bcb2clang.convert_source(converted, table), converted)


if __name__ == '__main__':
    unittest.main()
