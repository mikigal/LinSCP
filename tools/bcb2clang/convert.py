#!/usr/bin/env python3
"""Converts an upstream WinSCP tree in place for building with Clang on Linux.

Used to produce the trees of the upstream-converted branch, see
docs/port/UPSTREAM.md:

- source/core: the C++Builder dialect (bcb2clang.convert_source), with method
  references resolved against the classes of all core files;
- the other upstream directories that are built (PuTTY, FileZilla): only
  backslashes in #include paths.

Exits with 1 if any file could not be converted.
"""

import argparse
import sys
from pathlib import Path

import bcb2clang

# Directories in the C++Builder dialect
DIALECT_DIRS = ['source/core']
# Other built upstream directories, with Windows-style #include paths
INCLUDE_DIRS = ['source/putty', 'source/filezilla']

SOURCE_SUFFIXES = {'.h', '.hpp', '.c', '.cpp'}


def _sources(root, directory, recursive):
    base = root / directory
    paths = base.rglob('*') if recursive else base.glob('*')
    return sorted(p for p in paths if p.is_file() and p.suffix in SOURCE_SUFFIXES)


def convert_tree(root):
    """Converts the tree at root in place; returns the list of errors."""
    errors = []
    jobs = []
    for directory in DIALECT_DIRS:
        paths = _sources(root, directory, recursive=False)
        table = bcb2clang.make_class_table(paths)
        jobs += [(path, lambda text, table=table: bcb2clang.convert_source(text, table)) for path in paths]
    for directory in INCLUDE_DIRS:
        jobs += [(path, bcb2clang.convert_includes) for path in _sources(root, directory, recursive=True)]
    for path, convert in jobs:
        text = bcb2clang.read_source(path)
        try:
            converted = convert(text)
        except bcb2clang.ConversionError as error:
            errors.append('%s: %s' % (path.relative_to(root), error))
            continue
        if converted != text:
            bcb2clang.write_source(path, converted)
    return errors


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('root', type=Path, help='root of the WinSCP tree')
    args = parser.parse_args(argv)
    errors = convert_tree(args.root.resolve())
    for error in errors:
        print(error, file=sys.stderr)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
