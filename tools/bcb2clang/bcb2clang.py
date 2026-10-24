#!/usr/bin/env python3
"""Converts C++Builder dialect constructs to standard C++ (Clang with -fdeclspec).

See docs/port/PLAN.md, section 3.2. The conversion is deterministic and keeps
every line where it was, so that diffs between converted upstream revisions
stay small and compiler diagnostics point at the original lines.

Conversions:

- ``__property T X = { read = R, write = W, index = I, ... };`` becomes
  ``__declspec(property(get = ..., put = ...)) T X;``. Field-backed reads and
  writes and ``index`` specifiers get generated inline accessors.
- ``R __fastcall (__closure * Name)(Args)`` becomes
  ``TMethodPtr<R(Args)> Name`` (in typedefs, members and parameters).
- References to methods (``OnX = Method;``, ``&Object->Method``, ...) become
  ``::rtl::MethodPtr(Object, &TClass::Method)``; which names are methods is
  known from the classes of all converted files (ClassTable).
- ``try { A } __finally { B }`` becomes a scope guard (``::rtl::TFinally``),
  also in macro definitions.
- Backslashes in ``#include`` paths become slashes.

Line endings (the upstream sources use CRLF) are kept.

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
    (?P<ws>(?:[ \t\r\n\f\v]|\\\r?\n)+)
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
        if text[end:newline].rstrip('\r').endswith('\\'):
            end = newline + 1
            continue
        if text[end:newline].count('/*') > text[end:newline].count('*/'):
            close = text.find('*/', newline)
            if close >= 0:
                end = close + 2
                continue
        return newline


def tokenize(text, preprocessor=True):
    """Splits text into tokens; ''.join(t.text for t in tokens) == text.
    Preprocessor directives are single tokens, unless preprocessor is False
    (for the text of a directive)."""
    tokens = []
    pos = 0
    at_line_start = preprocessor
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
            if '\n' in token_text and preprocessor:
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
# try/__finally
# ---------------------------------------------------------------------------

_JUMPS = {'return', 'break', 'continue', 'goto'}


def convert_finally(tokens):
    """``try { A } __finally { B }`` becomes a scope guard running B, see
    source/rtl/include/rtl/Finally.h. B moves in front of A; the line count
    of the whole statement stays the same."""
    result = list(tokens)
    while True:
        position = next((i for i, t in enumerate(result) if t.kind == 'ident' and t.text == '__finally'), None)
        if position is None:
            return result
        try_close = _prev_significant(result, position)
        if try_close < 0 or result[try_close].text != '}':
            raise ConversionError('__finally not preceded by a block')
        depth = 0
        try_open = try_close
        while try_open >= 0:
            text = result[try_open].text if result[try_open].kind == 'punct' else None
            if text == '}':
                depth += 1
            elif text == '{':
                depth -= 1
                if depth == 0:
                    break
            try_open -= 1
        try_index = _prev_significant(result, try_open)
        if try_index < 0 or result[try_index].kind != 'ident' or result[try_index].text != 'try':
            raise ConversionError('__finally does not follow a try block')
        finally_open = _next_significant(result, position)
        if finally_open >= len(result) or result[finally_open].text != '{':
            raise ConversionError('__finally without a block')
        finally_close = _matching(result, finally_open)
        # B runs in a lambda, where jumps would not leave the enclosing function or loop
        jumps = {t.text for t in result[finally_open:finally_close] if t.kind == 'ident'} & _JUMPS
        if jumps:
            raise ConversionError('%s in a __finally block' % ', '.join(sorted(jumps)))

        replacement = (
            [Token('converted', '{ ::rtl::TFinally Finally([&]()')] +
            result[position + 1:finally_open] +
            result[finally_open:finally_close + 1] +
            [Token('converted', '); try')] +
            result[try_index + 1:try_open] +
            result[try_open:try_close + 1] +
            result[try_close + 1:position] +
            [Token('converted', 'catch (...) { Finally.Unwind(); } }')])
        result[try_index:finally_close + 1] = replacement


# ---------------------------------------------------------------------------
# Method references
# ---------------------------------------------------------------------------
#
# C++Builder makes a closure of a method named without a call: ``OnX = Method;``,
# ``Run(Method)``, ``&Method``, ``Object->Method`` and ``&Object->Method``. In
# standard C++ these must name the class, so they become
# ``::rtl::MethodPtr(Object, &TClass::Method)`` (source/rtl/include/rtl/Closure.h).
# Which names are methods is known from the class declarations of all files
# being converted (a ClassTable).

class ClassInfo:
    def __init__(self, name, bases):
        self.name = name
        self.bases = bases
        # Non-static member functions
        self.methods = set()
        self.static_methods = set()
        # Data members and properties
        self.fields = set()
        # Identifiers in the declarations of data members
        self.field_types = {}


_CLASS_KEYS = {'class', 'struct'}
_CLASS_NAME_NOISE = {'PACKAGE', 'DELPHICLASS', 'final', 'alignas'}
_BASE_NOISE = {'public', 'protected', 'private', 'virtual'}


def _add_field(info, tokens, statement_start, name_index):
    info.fields.add(tokens[name_index].text)
    info.field_types[tokens[name_index].text] = [
        t.text for t in tokens[statement_start:name_index] if t.kind == 'ident']


def _class_members(tokens, open_index, close_index, info):
    depth = 0
    i = open_index + 1
    statement_start = i
    while i < close_index:
        token = tokens[i]
        if token.kind == 'ident' and token.text == '__property' and depth == 0:
            end = _statement_end(tokens, i)
            stop = next((j for j in range(i + 1, end) if tokens[j].text in ('[', '=')), None)
            if stop is not None:
                info.fields.add(tokens[_prev_significant(tokens, stop)].text)
            i = end + 1
            statement_start = i
            continue
        if token.kind == 'punct':
            previous = _prev_significant(tokens, i)
            previous_name = tokens[previous].text if (previous > open_index and tokens[previous].kind == 'ident') else None
            if depth == 0 and token.text == ':' and previous > open_index and tokens[previous].text == ')':
                # Constructor initializer list: skip to the body
                while (i < close_index) and (tokens[i].text != '{'):
                    i += 1
                continue
            if token.text in ('{', '('):
                if token.text == '(' and depth == 0 and previous_name is not None:
                    declaration = {t.text for t in tokens[statement_start:previous] if t.kind == 'ident'}
                    if declaration & {'typedef', 'friend'}:
                        pass
                    elif 'static' in declaration:
                        info.static_methods.add(previous_name)
                    elif previous_name != info.name:
                        info.methods.add(previous_name)
                depth += 1
            elif token.text in ('}', ')'):
                depth -= 1
                if depth == 0 and token.text == '}':
                    statement_start = i + 1
            elif depth == 0 and token.text in (';', ':'):
                if token.text == ';' and previous_name is not None:
                    _add_field(info, tokens, statement_start, previous)
                statement_start = i + 1
            elif depth == 0 and token.text in ('=', '[', ',') and previous_name is not None:
                _add_field(info, tokens, statement_start, previous)
        i += 1


class ClassTable:
    """Classes declared in a set of sources: their bases and members."""

    def __init__(self):
        self.classes = {}
        # Names of properties and fields of classes not in the table (RTL)
        self.other_fields = set()
        # Function-like macros: their arguments are not necessarily expressions
        self.macros = set()

    def add_source(self, text):
        self.add_tokens(tokenize(text))

    def add_tokens(self, tokens):
        for token in tokens:
            if token.kind == 'pp':
                match = re.match(r'\s*#\s*define\s+(\w+)\(', token.text)
                if match:
                    self.macros.add(match.group(1))
        for i, token in enumerate(tokens):
            if token.kind != 'ident' or token.text not in _CLASS_KEYS:
                continue
            previous = _prev_significant(tokens, i)
            if previous >= 0 and tokens[previous].text in ('friend', 'enum'):
                continue
            # The name is the last identifier before ':' or '{'
            name = None
            j = _next_significant(tokens, i)
            while j < len(tokens) and tokens[j].kind == 'ident':
                if tokens[j].text not in _CLASS_NAME_NOISE:
                    name = tokens[j].text
                j = _next_significant(tokens, j)
            if name is None or j >= len(tokens) or tokens[j].text not in (':', '{'):
                continue
            bases = []
            if tokens[j].text == ':':
                while j < len(tokens) and tokens[j].text not in ('{', ';'):
                    if tokens[j].kind == 'ident' and tokens[j].text not in _BASE_NOISE:
                        following = _next_significant(tokens, j)
                        if tokens[following].text in (',', '{'):
                            bases.append(tokens[j].text)
                    j += 1
                if j >= len(tokens) or tokens[j].text != '{':
                    continue
            info = self.classes.setdefault(name, ClassInfo(name, bases))
            info.bases = bases or info.bases
            _class_members(tokens, j, _matching(tokens, j), info)

    def add_rtl_headers(self, directory):
        """Property names declared by the RTL shim's headers."""
        for path in sorted(Path(directory).glob('*.h')):
            text = path.read_text(errors='replace')
            self.other_fields.update(re.findall(r'__declspec\(property\([^)]*\)\)[^;]*?(\w+)\s*(?:\[\])*;', text))

    def lineage(self, name):
        """The class and its known bases."""
        result = []
        pending = [name]
        while pending:
            info = self.classes.get(pending.pop(0))
            if info is not None and info not in result:
                result.append(info)
                pending.extend(info.bases)
        return result

    def field_class(self, class_name, name):
        """The class of the data member name of the class, if it is one of the table."""
        for info in self.lineage(class_name):
            if name in info.field_types:
                return next((t for t in info.field_types[name] if t in self.classes), None)
        return None

    def is_method_of(self, class_name, name):
        lineage = self.lineage(class_name)
        for info in lineage:
            if name in info.fields or name in info.static_methods:
                return False
            if name in info.methods:
                return True
        return False

    def declaring_class(self, name):
        """The base-most class declaring the method name, if the name is
        unambiguously a method."""
        if name in self.other_fields:
            return None
        declaring = []
        for info in self.classes.values():
            if name in info.fields or name in info.static_methods:
                return None
            if name in info.methods:
                declaring.append(info.name)
        for candidate in declaring:
            if all(candidate in [i.name for i in self.lineage(other)] for other in declaring):
                return candidate
        return None


_NOT_TYPES = {
    'return', 'throw', 'case', 'else', 'new', 'delete', 'sizeof', 'typeid', 'goto', 'do',
    'co_return', 'co_yield', 'co_await', 'and', 'or', 'not'}
_VALUE_END = {')', ']'}


def _ends_value(token):
    """Whether a token ends an operand, so that a following '&' is binary."""
    if token.kind in ('number', 'string', 'char'):
        return True
    if token.kind == 'ident':
        return token.text not in _NOT_TYPES
    return token.kind == 'punct' and token.text in _VALUE_END


def _local_names(tokens, start, end):
    """Names declared by parameters and local declarations in tokens[start:end],
    mapped to the identifier of their type."""
    names = {}
    for i in range(start, end):
        token = tokens[i]
        if token.kind != 'ident':
            continue
        previous = _prev_significant(tokens, i)
        following = _next_significant(tokens, i)
        if following >= len(tokens) or tokens[following].text not in (';', '=', '(', '[', ',', ')', ':'):
            continue
        before = tokens[previous] if previous >= 0 else None
        if before is None:
            continue
        if before.kind == 'punct' and before.text in ('*', '&'):
            # A declarator (TType * Name), not an operator (X * Name, &Name)
            before = tokens[_prev_significant(tokens, previous)]
        if (before.kind == 'ident' and before.text not in _NOT_TYPES) or before.text == '>':
            names[token.text] = before.text if before.kind == 'ident' else None
    return names


def _member_functions(tokens, table):
    """(class name, parameters open, body close) of member function definitions."""
    result = []
    for info_name, open_index, close_index in _class_bodies(tokens, table):
        # Inline member functions
        depth = 0
        for i in range(open_index + 1, close_index):
            token = tokens[i]
            if token.kind != 'punct':
                continue
            if token.text == '(' and depth == 0:
                close = _matching(tokens, i)
                body = _function_body(tokens, close)
                if body is not None:
                    result.append((info_name, i, _matching(tokens, body)))
            if token.text in _OPENING:
                depth += 1
            elif token.text in _CLOSING:
                depth -= 1
    # Out-of-class definitions: TClass::Name(...) { ... }
    for i, token in enumerate(tokens):
        if token.kind != 'ident' or token.text not in table.classes:
            continue
        scope = _next_significant(tokens, i)
        if scope >= len(tokens) or tokens[scope].text != '::':
            continue
        previous = _prev_significant(tokens, i)
        if previous >= 0 and tokens[previous].text in ('::', '->', '.'):
            continue
        name = _next_significant(tokens, scope)
        if name < len(tokens) and tokens[name].text == '~':
            name = _next_significant(tokens, name)
        if name >= len(tokens) or tokens[name].kind != 'ident':
            continue
        paren = _next_significant(tokens, name)
        if paren >= len(tokens) or tokens[paren].text != '(':
            continue
        body = _function_body(tokens, _matching(tokens, paren))
        if body is not None:
            result.append((token.text, paren, _matching(tokens, body)))
    return result


def _class_bodies(tokens, table):
    for i, token in enumerate(tokens):
        if token.kind != 'ident' or token.text not in _CLASS_KEYS:
            continue
        j = _next_significant(tokens, i)
        name = None
        while j < len(tokens) and tokens[j].kind == 'ident':
            if tokens[j].text not in _CLASS_NAME_NOISE:
                name = tokens[j].text
            j = _next_significant(tokens, j)
        if name not in table.classes:
            continue
        while j < len(tokens) and tokens[j].text not in ('{', ';'):
            j += 1
        if j < len(tokens) and tokens[j].text == '{':
            yield name, j, _matching(tokens, j)


_FUNCTION_QUALIFIERS = {'const', 'noexcept', 'override', 'final', 'volatile', '__fastcall'}


def _function_body(tokens, params_close):
    """Index of the '{' of the body of a function whose parameter list closes at params_close."""
    i = _next_significant(tokens, params_close)
    while i < len(tokens) and tokens[i].kind == 'ident' and tokens[i].text in _FUNCTION_QUALIFIERS:
        i = _next_significant(tokens, i)
    if i < len(tokens) and tokens[i].text == ':':
        # Constructor initializer list
        depth = 0
        while i < len(tokens):
            text = tokens[i].text if tokens[i].kind == 'punct' else None
            if text == '{' and depth == 0:
                break
            if text in ('(', '['):
                depth += 1
            elif text in (')', ']'):
                depth -= 1
            elif text == ';' and depth == 0:
                return None
            i += 1
    if i < len(tokens) and tokens[i].text == '{':
        return i
    return None


def _object_start(tokens, arrow):
    """First token of the postfix expression before the '->' at arrow."""
    i = _prev_significant(tokens, arrow)
    start = None
    while i >= 0:
        token = tokens[i]
        if token.kind == 'punct' and token.text in (')', ']'):
            depth = 0
            while i >= 0:
                text = tokens[i].text if tokens[i].kind == 'punct' else None
                if text in (')', ']'):
                    depth += 1
                elif text in ('(', '['):
                    depth -= 1
                    if depth == 0:
                        break
                i -= 1
            start = i
            i = _prev_significant(tokens, i)
            if i >= 0 and tokens[i].kind == 'ident':
                # Function call or array element
                start = i
                i = _prev_significant(tokens, i)
        elif token.kind == 'ident' and token.text not in _NOT_TYPES:
            start = i
            i = _prev_significant(tokens, i)
        else:
            break
        if i >= 0 and tokens[i].kind == 'punct' and tokens[i].text in ('->', '.', '::'):
            i = _prev_significant(tokens, i)
            continue
        break
    return start


def _in_macro_arguments(tokens, index, macros):
    """Whether tokens[index] is directly in the arguments of a function-like macro."""
    depth = 0
    i = index - 1
    while i >= 0:
        text = tokens[i].text if tokens[i].kind == 'punct' else None
        if text in (')', ']'):
            depth += 1
        elif text in ('(', '['):
            if depth == 0:
                if text == '[':
                    return False
                name = _prev_significant(tokens, i)
                return name >= 0 and tokens[name].kind == 'ident' and tokens[name].text in macros
            depth -= 1
        elif text in ('{', '}', ';') and depth == 0:
            return False
        i -= 1
    return False


def _method_references(result, start, end, class_name, locals_, table, macros):
    """Replacements (start, end, text) of the method references in result[start:end]
    within a member function of class_name."""
    replacements = []
    i = start - 1
    while i < end - 1:
        i += 1
        token = result[i]
        if token.kind != 'ident':
            continue
        following = _next_significant(result, i)
        if following < len(result) and result[following].text in ('(', '::', '<', '##', '->', '.'):
            continue
        previous = _prev_significant(result, i)
        before = result[previous] if previous >= 0 else Token('punct', ';')
        if before.kind == 'punct' and before.text in ('##', '#'):
            continue
        if before.kind == 'punct' and before.text in ('->', '.'):
            object_start = _object_start(result, previous)
            if object_start is None or _in_macro_arguments(result, object_start, macros):
                continue
            object_class = None
            if _next_significant(result, object_start) == previous and result[object_start].kind == 'ident':
                object_name = result[object_start].text
                if object_name == 'this':
                    object_class = class_name
                elif object_name in locals_:
                    object_class = locals_[object_name]
                else:
                    object_class = table.field_class(class_name, object_name)
            if object_class in table.classes:
                if not table.is_method_of(object_class, token.text):
                    continue
                declaring = object_class
            elif before.text == '->':
                declaring = table.declaring_class(token.text)
                if declaring is None:
                    continue
            else:
                continue
            object_text = _flat(result, object_start, previous)
            if before.text == '.':
                object_text = '&' + object_text
            address = _prev_significant(result, object_start)
            if address >= 0 and result[address].text == '&' and not _ends_value(result[_prev_significant(result, address)]):
                object_start = address
            replacements.append((object_start, i, '::rtl::MethodPtr(%s, &%s::%s)' % (object_text, declaring, token.text)))
            continue
        if before.kind == 'punct' and before.text in ('::', '~'):
            continue
        if token.text in locals_ or not table.is_method_of(class_name, token.text):
            continue
        if _in_macro_arguments(result, i, macros):
            continue
        reference_start = i
        if before.kind == 'punct' and before.text == '&':
            if _ends_value(result[_prev_significant(result, previous)]):
                continue
            reference_start = previous
        elif before.kind == 'ident' and before.text not in _NOT_TYPES:
            # A declaration
            continue
        replacements.append((reference_start, i, '::rtl::MethodPtr(this, &%s::%s)' % (class_name, token.text)))
    return replacements


def _apply_replacements(tokens, replacements):
    for start, end, text in sorted(replacements, reverse=True):
        # Keep the line count
        text += '\n' * _text(tokens, start, end + 1).count('\n')
        tokens[start:end + 1] = [Token('converted', text)]


def convert_method_references(tokens, table):
    if table is None:
        return tokens
    result = list(tokens)
    functions = _member_functions(result, table)
    replacements = []
    for class_name, params_open, body_close in functions:
        locals_ = _local_names(result, params_open + 1, body_close)
        replacements += _method_references(
            result, _matching(result, params_open) + 1, body_close, class_name, locals_, table, table.macros)
    # Macros defined for the member functions that follow them
    for index, token in enumerate(result):
        match = _DEFINE_RE.match(token.text) if token.kind == 'pp' else None
        if match is None:
            continue
        following = [f for f in functions if f[1] > index]
        if not following:
            continue
        class_name, params_open, body_close = min(following, key=lambda f: f[1])
        body = tokenize(match.group(2), preprocessor=False)
        parameters = re.search(r'\(([^)]*)\)$', match.group(1))
        parameters = re.findall(r'\w+', parameters.group(1)) if parameters else []
        # Names local to the function, and the macro's parameters
        locals_ = _local_names(result, params_open + 1, body_close)
        locals_.update(dict.fromkeys(parameters))
        macro_replacements = _method_references(
            body, 0, len(body), class_name, locals_, table, table.macros | set(parameters))
        if macro_replacements:
            _apply_replacements(body, macro_replacements)
            result[index] = Token('pp', match.group(1) + untokenize(body))
    _apply_replacements(result, replacements)
    return result


# ---------------------------------------------------------------------------
# Preprocessor
# ---------------------------------------------------------------------------

_INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*)([<"])([^>"]*)([>"])(.*)$', re.S)


_DEFINE_RE = re.compile(r'^(\s*#\s*define\s+\w+(?:\([^)]*\))?)(.*)$', re.S)


def _convert_include(token):
    match = _INCLUDE_RE.match(token.text)
    if match and '\\' in match.group(3):
        token = Token('pp', match.group(1) + match.group(2) + match.group(3).replace('\\', '/') +
                      match.group(4) + match.group(5))
    return token


def convert_preprocessor(tokens):
    result = []
    for token in tokens:
        if token.kind == 'pp':
            token = _convert_include(token)
            match = _DEFINE_RE.match(token.text)
            if match and '__finally' in match.group(2):
                body = untokenize(convert_finally(tokenize(match.group(2), preprocessor=False)))
                token = Token('pp', match.group(1) + body)
        result.append(token)
    return result


# ---------------------------------------------------------------------------

def convert_source(text, table=None):
    """Returns the converted source text. Method references are converted
    with the classes of the table, by default those declared in the text."""
    tokens = tokenize(text)
    if table is None:
        table = ClassTable()
        table.add_tokens(tokens)
    tokens = convert_preprocessor(tokens)
    tokens = convert_closures(tokens)
    tokens = convert_properties(tokens)
    tokens = convert_method_references(tokens, table)
    tokens = convert_finally(tokens)
    result = untokenize(tokens)
    if '\r\n' in text:
        # Line breaks added by the conversion follow the file's convention
        result = re.sub(r'(?<!\r)\n', '\r\n', result)
    return result


def convert_includes(text):
    """Converts only the #include paths, for upstream code that is not in the
    C++Builder dialect (PuTTY, FileZilla)."""
    return untokenize([_convert_include(t) if t.kind == 'pp' else t for t in tokenize(text)])


def read_source(path):
    data = Path(path).read_bytes()
    # Upstream sources are UTF-8 (some with a BOM) or plain ASCII
    return data.decode('utf-8', errors='surrogateescape')


def write_source(path, text):
    Path(path).write_bytes(text.encode('utf-8', errors='surrogateescape'))


RTL_HEADERS = Path(__file__).resolve().parents[2] / 'source' / 'rtl' / 'include' / 'rtl'


def make_class_table(paths):
    """The classes declared in the files, for converting method references
    in any of them, plus the properties of the RTL shim."""
    table = ClassTable()
    for path in paths:
        table.add_source(read_source(path))
    table.add_rtl_headers(RTL_HEADERS)
    return table


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('files', nargs='+', type=Path, help='source files to convert')
    parser.add_argument('-o', '--output-dir', type=Path,
                        help='write converted files here (same names) instead of in place')
    args = parser.parse_args(argv)
    failed = False
    table = make_class_table(args.files)
    for path in args.files:
        try:
            converted = convert_source(read_source(path), table)
        except ConversionError as error:
            print('%s: %s' % (path, error), file=sys.stderr)
            failed = True
            continue
        target = (args.output_dir / path.name) if args.output_dir else path
        if args.output_dir:
            args.output_dir.mkdir(parents=True, exist_ok=True)
        if not target.exists() or read_source(target) != converted:
            write_source(target, converted)
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
