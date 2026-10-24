#!/usr/bin/env python3
"""Converts C++Builder dialect constructs to standard C++ (Clang with -fdeclspec).

See docs/port/PLAN.md, section 3.2. The conversion is deterministic and keeps
every line where it was, so that diffs between converted upstream revisions
stay small and compiler diagnostics point at the original lines.

Handled so far (declarations, enough for the core's headers):

- ``__property T X = { read = R, write = W, index = I, ... };`` becomes
  ``__declspec(property(get = ..., put = ...)) T X;``. Field-backed reads and
  writes and ``index`` specifiers get generated inline accessors.
- ``R __fastcall (__closure * Name)(Args)`` becomes
  ``TMethodPtr<R(Args)> Name`` (in typedefs, members and parameters).
- Backslashes in ``#include`` paths become slashes.

Keywords with a direct equivalent (``__fastcall``, ``__published``,
``__int64``, ...) are macros or typedefs of the RTL shim (source/rtl) and are
left alone.
"""

import argparse
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Tokenizer
# ---------------------------------------------------------------------------

_TOKEN_RE = re.compile(r'''
    (?P<ws>(?:[ \t\r\n\f\v]|\\\n)+)
  | (?P<comment>//[^\n]*|/\*.*?\*/)
  | (?P<string>(?:u8|u|U|L)?R"(?P<delim>[^()\\\s]{0,16})\(.*?\)(?P=delim)"
             |(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*")
  | (?P<char>(?:u8|u|U|L)?'(?:\\.|[^'\\\n])+')
  | (?P<ident>[A-Za-z_$][A-Za-z0-9_$]*)
  | (?P<number>\.?[0-9](?:[eEpP][+-]|[0-9A-Za-z_.'])*)
  | (?P<punct>::|->\*?|\.\.\.|<<=|>>=|[<>=!+\-*/%&|^]=|&&|\|\||\+\+|--|<<|>>|\#\#
             |[{}\[\]()<>;:,.?~!+\-*/%&|^=#\\@`])
''', re.S | re.X)


class Token:
    __slots__ = ('kind', 'text')

    def __init__(self, kind, text):
        self.kind = kind
        self.text = text

    def __repr__(self):
        return 'Token(%r, %r)' % (self.kind, self.text)

    @property
    def significant(self):
        return self.kind not in ('ws', 'comment')


def _preprocessor_end(text, pos):
    """End of the logical preprocessor line starting at pos (excluding the newline)."""
    end = pos
    while True:
        newline = text.find('\n', end)
        if newline < 0:
            return len(text)
        # A comment may hide a line break; keep it simple: only backslash continuations
        if newline > 0 and text[newline - 1] == '\\':
            end = newline + 1
            continue
        if text[end:newline].count('/*') > text[end:newline].count('*/'):
            close = text.find('*/', newline)
            if close >= 0:
                end = close + 2
                continue
        return newline


def tokenize(text):
    """Splits text into tokens; ''.join(t.text for t in tokens) == text."""
    tokens = []
    pos = 0
    at_line_start = True
    length = len(text)
    while pos < length:
        if at_line_start:
            match = re.compile(r'[ \t]*#').match(text, pos)
            if match:
                end = _preprocessor_end(text, pos)
                tokens.append(Token('pp', text[pos:end]))
                pos = end
                at_line_start = False
                continue
        match = _TOKEN_RE.match(text, pos)
        if not match:
            # Unknown character, keep it as punctuation
            tokens.append(Token('punct', text[pos]))
            pos += 1
            at_line_start = False
            continue
        kind = match.lastgroup
        if kind == 'delim':
            kind = 'string'
        token_text = match.group(0)
        tokens.append(Token(kind, token_text))
        pos = match.end()
        if kind == 'ws':
            if '\n' in token_text:
                at_line_start = True
        elif kind == 'comment' and token_text.startswith('/*') and '\n' in token_text:
            # A block comment does not start a line for the preprocessor
            at_line_start = False
        else:
            at_line_start = False
    return tokens


def untokenize(tokens):
    return ''.join(t.text for t in tokens)


class ConversionError(Exception):
    pass


# ---------------------------------------------------------------------------
# Helpers on token lists
# ---------------------------------------------------------------------------

_OPENING = {'(': ')', '[': ']', '{': '}'}
_CLOSING = {')': '(', ']': '[', '}': '{'}


def _next_significant(tokens, index):
    index += 1
    while index < len(tokens) and not tokens[index].significant:
        index += 1
    return index


def _prev_significant(tokens, index):
    index -= 1
    while index >= 0 and not tokens[index].significant:
        index -= 1
    return index


def _matching(tokens, index):
    """Index of the bracket closing the one at index."""
    opening = tokens[index].text
    closing = _OPENING[opening]
    depth = 0
    for i in range(index, len(tokens)):
        token = tokens[i]
        if token.kind != 'punct':
            continue
        if token.text == opening:
            depth += 1
        elif token.text == closing:
            depth -= 1
            if depth == 0:
                return i
    raise ConversionError('unbalanced %r' % opening)


def _text(tokens, start, end):
    return untokenize(tokens[start:end])


def _flat(tokens, start, end):
    """Text of tokens with comments dropped and whitespace collapsed."""
    parts = []
    for token in tokens[start:end]:
        if token.kind == 'comment':
            parts.append(' ')
        elif token.kind == 'ws':
            parts.append(' ')
        else:
            parts.append(token.text)
    return re.sub(r'\s+', ' ', ''.join(parts)).strip()


# ---------------------------------------------------------------------------
# __closure
# ---------------------------------------------------------------------------

_CLOSURE_DELIMITERS = {';', '{', '}', ',', ':'}


def _closure_return_type_start(tokens, paren):
    """First token of the return type of a closure declarator whose '(' is at paren."""
    depth = 0
    index = paren - 1
    start = paren
    while index >= 0:
        token = tokens[index]
        if token.kind == 'pp':
            break
        if token.kind == 'punct':
            if token.text in (')', ']', '>'):
                depth += 1
            elif token.text in ('(', '[', '<'):
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and token.text in _CLOSURE_DELIMITERS:
                break
        if token.kind == 'ident' and depth == 0 and token.text in ('typedef', 'public', 'protected', 'private'):
            break
        if token.significant:
            start = index
        index -= 1
    return start


def convert_closures(tokens):
    result = list(tokens)
    while True:
        position = next((i for i, t in enumerate(result) if t.kind == 'ident' and t.text == '__closure'), None)
        if position is None:
            return result
        paren = _prev_significant(result, position)
        if paren < 0 or result[paren].text != '(':
            raise ConversionError('__closure not in a declarator')
        star = _next_significant(result, position)
        if result[star].text != '*':
            raise ConversionError('__closure not followed by *')
        name_index = _next_significant(result, star)
        qualifiers = []
        while result[name_index].kind == 'ident' and result[name_index].text in ('const', 'volatile'):
            qualifiers.append(result[name_index].text)
            name_index = _next_significant(result, name_index)
        if result[name_index].kind == 'ident':
            name = result[name_index].text
            close = _next_significant(result, name_index)
        else:
            # Abstract declarator, e.g. in a cast
            name = None
            close = name_index
        if result[close].text != ')':
            raise ConversionError('unexpected %r in closure declarator' % result[close].text)
        params_open = _next_significant(result, close)
        if result[params_open].text != '(':
            raise ConversionError('closure declarator without parameters')
        params_close = _matching(result, params_open)

        start = _closure_return_type_start(result, paren)
        return_type = [t for t in result[start:paren] if not (t.kind == 'ident' and t.text == '__fastcall')]
        return_text = _flat(return_type, 0, len(return_type))
        params_text = _text(result, params_open + 1, params_close)
        replacement = 'TMethodPtr<%s(%s)>' % (return_text, params_text)
        # Keep the line count: line breaks outside of the parameters go before the name
        newlines = _text(result, start, params_close + 1).count('\n') - params_text.count('\n')
        replacement += '\n' * newlines
        if qualifiers:
            replacement += ' ' + ' '.join(qualifiers)
        if name is not None:
            replacement += ' ' + name
        result[start:params_close + 1] = [Token('converted', replacement)]


# ---------------------------------------------------------------------------
# __property
# ---------------------------------------------------------------------------

def _split_top_level(tokens, start, end, separator):
    """Splits tokens[start:end] at separator tokens on bracket depth 0."""
    parts = []
    depth = 0
    part_start = start
    for i in range(start, end):
        token = tokens[i]
        if token.kind == 'punct':
            if token.text in _OPENING:
                depth += 1
            elif token.text in _CLOSING:
                depth -= 1
            elif token.text == separator and depth == 0:
                parts.append((part_start, i))
                part_start = i + 1
    parts.append((part_start, end))
    return parts


def _class_body(tokens, index):
    """(open, close) indexes of the braces of the class enclosing tokens[index]."""
    depth = 0
    i = index
    while i >= 0:
        token = tokens[i]
        if token.kind == 'punct':
            if token.text == '}':
                depth += 1
            elif token.text == '{':
                if depth == 0:
                    return i, _matching(tokens, i)
                depth -= 1
        i -= 1
    return None


_DECLARATION_SPECIFIERS = {'virtual', 'static', 'inline', '__fastcall', 'DYNAMIC', 'explicit', 'friend', 'HIDESBASE'}


def _method_return_type(tokens, open_index, name_index):
    """Return type text of the member function whose name is at name_index."""
    i = _prev_significant(tokens, name_index)
    start = name_index
    while i > open_index:
        token = tokens[i]
        if token.kind == 'pp' or (token.kind == 'punct' and token.text in (';', '{', '}', ':')):
            break
        if token.significant:
            start = i
        i -= 1
    parts = [t for t in tokens[start:name_index]
             if not (t.kind == 'ident' and t.text in _DECLARATION_SPECIFIERS)]
    return _flat(parts, 0, len(parts))


def _member_kinds(tokens, body):
    """Direct members of a class body: (fields, methods), methods map names
    to the set of their return types."""
    fields = set()
    methods = {}
    if body is None:
        return fields, methods
    open_index, close_index = body
    depth = 0
    i = open_index + 1
    while i < close_index:
        token = tokens[i]
        if token.kind == 'ident' and token.text == '__property' and depth == 0:
            # The names in property specifiers are not declarations
            i = _statement_end(tokens, i) + 1
            continue
        if token.kind == 'punct':
            previous = _prev_significant(tokens, i)
            previous_name = tokens[previous].text if (previous > open_index and tokens[previous].kind == 'ident') else None
            if token.text == ':' and depth == 0 and previous > open_index and tokens[previous].text == ')':
                # Constructor initializer list: skip to the body
                while (i < close_index) and (tokens[i].text != '{'):
                    i += 1
                continue
            if token.text in ('{', '('):
                if token.text == '(' and depth == 0 and previous_name is not None:
                    methods.setdefault(previous_name, set()).add(_method_return_type(tokens, open_index, previous))
                depth += 1
            elif token.text in ('}', ')'):
                depth -= 1
            elif depth == 0 and token.text in (';', '=', '[', ',', ':') and previous_name is not None:
                fields.add(previous_name)
        i += 1
    return fields, methods


def _statement_end(tokens, index):
    depth = 0
    for i in range(index, len(tokens)):
        token = tokens[i]
        if token.kind != 'punct':
            continue
        if token.text in _OPENING:
            depth += 1
        elif token.text in _CLOSING:
            depth -= 1
        elif token.text == ';' and depth == 0:
            return i
    raise ConversionError('unterminated declaration')


def _is_field(name, kinds):
    fields, methods = kinds
    if name in fields:
        return True
    if name in methods:
        return False
    # Not declared in this class (inherited): WinSCP/VCL naming convention
    return re.match(r'F[A-Z0-9_]', name) is not None


def _index_parameters(tokens, open_index):
    """Parameter declaration and name of an array property index [Type Name]."""
    close = _matching(tokens, open_index)
    declaration = _flat(tokens, open_index + 1, close)
    name_index = _prev_significant(tokens, close)
    name = tokens[name_index].text if tokens[name_index].kind == 'ident' else None
    return declaration, name, close


def _convert_property(tokens, start, end, kinds):
    """Replacement text for the property declaration tokens[start:end+1].

    The accessors named in the declaration are usually private, while the
    property is public; Clang checks the access of the accessors where the
    property is used, so the property always gets inline accessors of its own,
    declared with the access of the property, that forward to them.
    """
    significant = [i for i in range(start + 1, end) if tokens[i].significant]
    # Name: the last identifier before the first '[' or '=' on depth 0
    stop = None
    for i in significant:
        if tokens[i].text in ('[', '='):
            stop = i
            break
    if stop is None:
        # Redeclaration of an inherited property (visibility change): not
        # needed in C++, the base property is visible as it is
        return ''
    name_index = _prev_significant(tokens, stop)
    if tokens[name_index].kind != 'ident' or name_index <= start:
        raise ConversionError('property without a name')
    name = tokens[name_index].text
    type_text = _flat(tokens, start + 1, name_index)
    if not type_text:
        return ''

    index_declarations = []
    index_names = []
    i = stop
    while tokens[i].text == '[':
        declaration, index_name, close = _index_parameters(tokens, i)
        if index_name is None or index_name == declaration:
            index_name = 'I%d' % len(index_names)
            declaration = '%s %s' % (declaration, index_name)
        index_declarations.append(declaration)
        index_names.append(index_name)
        i = _next_significant(tokens, close)
    if tokens[i].text != '=':
        raise ConversionError('property %s without specifiers' % name)
    brace = _next_significant(tokens, i)
    if tokens[brace].text != '{':
        raise ConversionError('property %s without specifier list' % name)
    brace_close = _matching(tokens, brace)

    specifiers = {}
    for part_start, part_end in _split_top_level(tokens, brace + 1, brace_close, ','):
        part = [j for j in range(part_start, part_end) if tokens[j].significant]
        if not part:
            continue
        key = tokens[part[0]].text
        if len(part) >= 2 and tokens[part[1]].text == '=':
            specifiers[key] = _flat(tokens, part[1] + 1, part_end)
        else:
            specifiers[key] = None

    read = specifiers.get('read')
    write = specifiers.get('write')
    index = specifiers.get('index')
    if read is None and write is None:
        raise ConversionError('property %s has neither read nor write' % name)
    arguments = list(index_names)
    if index is not None:
        arguments.append(index)
    accessors = []
    clauses = []
    if read is not None:
        get = '__property_get_%s' % name
        return_type = type_text
        if not index_names and index is None and _is_field(read, kinds):
            body = 'return %s;' % read
        else:
            # Through a mutable this: works whether the getter is const or not
            body = 'return ::rtl::Mutable(this)->%s(%s);' % (read, ', '.join(arguments))
            # As in C++Builder, the property has the getter's type
            return_types = kinds[1].get(read, set())
            if len(return_types) == 1 and next(iter(return_types)):
                return_type = next(iter(return_types))
        accessors.append('%s %s(%s) const { %s }' % (return_type, get, ', '.join(index_declarations), body))
        clauses.append('get = %s' % get)
    if write is not None:
        put = '__property_set_%s' % name
        parameters = index_declarations + ['%s Value' % type_text]
        if not index_names and index is None and _is_field(write, kinds):
            body = '%s = Value;' % write
        else:
            body = '%s(%s);' % (write, ', '.join(arguments + ['Value']))
        accessors.append('void %s(%s) { %s }' % (put, ', '.join(parameters), body))
        clauses.append('put = %s' % put)
    declaration = '__declspec(property(%s)) %s %s%s;' % (
        ', '.join(clauses), type_text, name, '[]' * len(index_names))
    return ' '.join(accessors + [declaration])


def convert_properties(tokens):
    result = list(tokens)
    while True:
        start = next((i for i, t in enumerate(result) if t.kind == 'ident' and t.text == '__property'), None)
        if start is None:
            return result
        end = _statement_end(result, start)
        kinds = _member_kinds(result, _class_body(result, start))
        replacement = _convert_property(result, start, end, kinds)
        # Keep the line count of multi-line declarations
        newlines = _text(result, start, end + 1).count('\n')
        result[start:end + 1] = [Token('converted', replacement + '\n' * newlines)]


# ---------------------------------------------------------------------------
# Preprocessor
# ---------------------------------------------------------------------------

_INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*)([<"])([^>"]*)([>"])(.*)$', re.S)


def convert_preprocessor(tokens):
    result = []
    for token in tokens:
        if token.kind == 'pp':
            match = _INCLUDE_RE.match(token.text)
            if match and '\\' in match.group(3):
                token = Token('pp', match.group(1) + match.group(2) + match.group(3).replace('\\', '/') +
                              match.group(4) + match.group(5))
        result.append(token)
    return result


# ---------------------------------------------------------------------------

def convert_source(text):
    """Returns the converted source text."""
    tokens = tokenize(text)
    tokens = convert_preprocessor(tokens)
    tokens = convert_closures(tokens)
    tokens = convert_properties(tokens)
    return untokenize(tokens)


def _read(path):
    data = Path(path).read_bytes()
    # Upstream sources are UTF-8 (some with a BOM) or plain ASCII
    return data.decode('utf-8', errors='surrogateescape')


def _write(path, text):
    Path(path).write_bytes(text.encode('utf-8', errors='surrogateescape'))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='+', type=Path, help='source files to convert')
    parser.add_argument('-o', '--output-dir', type=Path,
                        help='write converted files here (same names) instead of in place')
    args = parser.parse_args(argv)
    failed = False
    for path in args.files:
        try:
            converted = convert_source(_read(path))
        except ConversionError as error:
            print('%s: %s' % (path, error), file=sys.stderr)
            failed = True
            continue
        target = (args.output_dir / path.name) if args.output_dir else path
        if args.output_dir:
            args.output_dir.mkdir(parents=True, exist_ok=True)
        if not target.exists() or _read(target) != converted:
            _write(target, converted)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
