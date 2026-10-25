#!/usr/bin/env python3
"""Generates a C++ table of the English texts of upstream's .dfm forms.

The translations (translations/*.ini) key the texts of the forms by the
.dfm file and the path of the component in it ([Login.dfm]
MainPanel.SitePanel.BasicGroup.Label1.Caption=...). The GUI is written in
Qt with the same English texts, so the table lets a translator map an English
text to the translated one (source/qt/app/Translations.cpp).

Usage: dfm2cpp.py OUTPUT.cpp INPUT.dfm...
"""

import argparse
import re
import sys
from pathlib import Path

# The properties with texts the translations translate
# HelpKeyword: the pages of upstream's documentation of the forms and their
# pages (not translated)
TEXT_PROPERTIES = {'Caption', 'Hint', 'Text', 'Filter', 'DialogTitle', 'DialogText', 'EditCaption', 'ButtonHint',
                   'HelpKeyword'}
LIST_PROPERTIES = {'Items.Strings'}

_TOKEN_RE = re.compile(r'''
    (?P<ws>\s+)
  | (?P<string>(?:'(?:[^'\n]|'')*'|\#[0-9]+)+)
  | (?P<ident>[A-Za-z_][A-Za-z0-9_.]*)
  | (?P<number>[-+]?[0-9]+(?:\.[0-9]+)?(?:[eE][-+]?[0-9]+)?)
  | (?P<binary>\{[0-9A-Fa-f\s]*\})
  | (?P<punct>[=:()<>\[\],+])
''', re.X)


class DfmError(Exception):
    pass


def _decode_string(token):
    """The text of a string token: quoted parts and #code characters (UTF-16
    code units, as Delphi writes the characters outside of ASCII)."""
    units = []
    for match in re.finditer(r"'((?:[^'\n]|'')*)'|#([0-9]+)", token):
        if match.group(1) is not None:
            for char in match.group(1).replace("''", "'"):
                units += _utf16(char)
        else:
            units.append(int(match.group(2)))
    data = b''.join(unit.to_bytes(2, 'little') for unit in units)
    return data.decode('utf-16-le', errors='surrogatepass')


def _utf16(char):
    data = char.encode('utf-16-le', errors='surrogatepass')
    return [int.from_bytes(data[i:i + 2], 'little') for i in range(0, len(data), 2)]


def _tokens(text):
    pos = 0
    line = 1
    while pos < len(text):
        match = _TOKEN_RE.match(text, pos)
        if not match:
            raise DfmError('line %d: unexpected %r' % (line, text[pos:pos + 20]))
        if match.lastgroup != 'ws':
            yield match.lastgroup, match.group(0), line
        line += match.group(0).count('\n')
        pos = match.end()


class _Parser:
    def __init__(self, text):
        self._tokens = list(_tokens(text))
        self._pos = 0
        self.entries = []

    def _peek(self, offset=0):
        index = self._pos + offset
        return self._tokens[index] if index < len(self._tokens) else (None, None, None)

    def _next(self):
        token = self._peek()
        if token[0] is None:
            raise DfmError('unexpected end')
        self._pos += 1
        return token

    def _expect(self, value):
        kind, token, line = self._next()
        if token != value:
            raise DfmError('line %d: expected %r, got %r' % (line, value, token))

    def parse(self):
        self._object([], True)
        if self._peek()[0] is not None:
            raise DfmError('line %d: data after the form' % self._peek()[2])
        return self.entries

    def _object(self, path, root):
        kind, keyword, line = self._next()
        if keyword not in ('object', 'inherited', 'inline'):
            raise DfmError('line %d: expected an object, got %r' % (line, keyword))
        kind, name, line = self._next()
        if self._peek()[1] == ':':
            self._next()
            self._next()
        if self._peek()[1] == '[':
            # The order of an inherited component
            while self._next()[1] != ']':
                pass
        # The root (the form) is not a part of the paths
        own_path = path if root else path + [name]
        while True:
            kind, token, line = self._peek()
            if token in ('object', 'inherited', 'inline'):
                self._object(own_path, False)
            elif token == 'end':
                self._next()
                return
            elif kind == 'ident':
                self._property(own_path)
            else:
                raise DfmError('line %d: unexpected %r' % (line, token))

    def _property(self, path):
        kind, name, line = self._next()
        self._expect('=')
        value = self._value()
        self._add(path, name, value)

    def _add(self, path, name, value):
        last = name.rsplit('.', 1)[-1]
        if isinstance(value, str) and (last in TEXT_PROPERTIES):
            self.entries.append(('.'.join(path + [name]), value))
        elif isinstance(value, list) and (name in LIST_PROPERTIES):
            for index, item in enumerate(value):
                if isinstance(item, str):
                    self.entries.append(('.'.join(path + [name, str(index)]), item))
        elif isinstance(value, tuple):
            # A collection: its items are keyed itemN, without the property
            for index, item in enumerate(value):
                for item_name, item_value in item:
                    self._add(path + ['item%d' % index], item_name, item_value)

    def _string_expression(self):
        kind, token, line = self._next()
        result = _decode_string(token)
        while self._peek()[1] == '+' and self._peek(1)[0] == 'string':
            self._next()
            result += _decode_string(self._next()[1])
        return result

    def _value(self):
        kind, token, line = self._peek()
        if kind is None:
            raise DfmError('unexpected end')
        if kind == 'string':
            return self._string_expression()
        if token == '(':
            self._next()
            items = []
            while self._peek()[1] != ')':
                items.append(self._value())
            self._next()
            return items
        if token == '<':
            self._next()
            items = []
            while self._peek()[1] != '>':
                self._expect('item')
                properties = []
                while self._peek()[1] != 'end':
                    kind, name, line = self._next()
                    self._expect('=')
                    properties.append((name, self._value()))
                self._next()
                items.append(properties)
            self._next()
            return tuple(items)
        if token == '[':
            self._next()
            while self._next()[1] != ']':
                pass
            return None
        if kind in ('ident', 'number', 'binary'):
            self._next()
            return None
        raise DfmError('line %d: unexpected value %r' % (line, token))


def parse(text):
    """[(key, text)] of the translatable texts of the form."""
    return _Parser(text).parse()


def cpp_literal(text):
    """A wide string literal with ASCII-only source (\\u escapes)."""
    parts = []
    for char in text:
        code = ord(char)
        if char == '\\':
            parts.append('\\\\')
        elif char == '"':
            parts.append('\\"')
        elif char == '\n':
            parts.append('\\n')
        elif char == '\r':
            parts.append('\\r')
        elif char == '\t':
            parts.append('\\t')
        elif code < 0x20 or code == 0x7f:
            parts.append('\\x%02x""' % code)
        elif code < 0x80:
            parts.append(char)
        elif code < 0x10000:
            parts.append('\\u%04x' % code)
        else:
            parts.append('\\U%08x' % code)
    return 'L"%s"' % ''.join(parts)


def generate(forms):
    """The C++ source of the table of [(file name, text of the form)]."""
    out = ['// Generated by tools/dfm2cpp/dfm2cpp.py from upstream\'s forms; do not edit.',
           '#include <FormTexts.h>',
           '',
           'const TFormText FormTexts[] =',
           '{']
    for name, text in forms:
        for key, value in parse(text):
            out.append('  { %s, %s, %s },' % (cpp_literal(name), cpp_literal(key), cpp_literal(value)))
    out.append('};')
    out.append('const size_t FormTextCount = sizeof(FormTexts) / sizeof(FormTexts[0]);')
    return '\n'.join(out) + '\n'


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('output', type=Path)
    parser.add_argument('inputs', type=Path, nargs='+')
    args = parser.parse_args(argv)
    forms = []
    for path in sorted(args.inputs):
        try:
            forms.append((path.name, path.read_bytes().decode('utf-8-sig')))
            parse(forms[-1][1])
        except DfmError as error:
            print('%s: %s' % (path, error), file=sys.stderr)
            return 1
    result = generate(forms)
    if not args.output.exists() or args.output.read_text() != result:
        args.output.write_text(result)
    return 0


if __name__ == '__main__':
    sys.exit(main())
