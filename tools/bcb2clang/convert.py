#!/usr/bin/env python3
"""Converts an upstream WinSCP tree in place for building with Clang on Linux.

Used to produce the trees of the upstream-converted branch, see
docs/port/UPSTREAM.md:

- source/core: the C++Builder dialect (bcb2clang.convert_source), with method
  references resolved against the classes of all core files;
- ported application files of source/windows (PORTED_FILES): converted
  copies in the directories of the Linux application layer, where they are
  edited; source/windows itself stays pristine;
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

# Application files of source/windows (which is not built) that the Linux
# application layer is ported from: upstream path -> path of the converted copy
PORTED_FILES = {
    'source/windows/ConsoleRunner.cpp': 'source/cli/ConsoleRunner.cpp',
    'source/windows/GUIConfiguration.cpp': 'source/app/GUIConfiguration.cpp',
    'source/windows/GUIConfiguration.h': 'source/app/GUIConfiguration.h',
    'source/windows/ProgParams.cpp': 'source/app/ProgParams.cpp',
    'source/windows/ProgParams.h': 'source/app/ProgParams.h',
    'source/windows/SynchronizeController.cpp': 'source/app/SynchronizeController.cpp',
    'source/windows/SynchronizeController.h': 'source/app/SynchronizeController.h',
}

SOURCE_SUFFIXES = {'.h', '.hpp', '.c', '.cpp'}


def _sources(root, directory, recursive):
    base = root / directory
    paths = base.rglob('*') if recursive else base.glob('*')
    return sorted(p for p in paths if p.is_file() and p.suffix in SOURCE_SUFFIXES)


def convert_tree(root):
    """Converts the tree at root in place; returns the list of errors."""
    errors = []
    # (source, target, conversion)
    jobs = []
    dialect_paths = []
    for directory in DIALECT_DIRS:
        paths = _sources(root, directory, recursive=False)
        dialect_paths += paths
        table = bcb2clang.make_class_table(paths)
        jobs += [(path, path, lambda text, table=table: bcb2clang.convert_source(text, table)) for path in paths]
    # The ported files refer to the core's classes and to each other's
    ported = [(root / source, root / target) for source, target in sorted(PORTED_FILES.items())
              if (root / source).is_file()]
    table = bcb2clang.make_class_table(dialect_paths + [source for source, _ in ported])
    jobs += [(source, target, lambda text: bcb2clang.convert_source(text, table)) for source, target in ported]
    for directory in INCLUDE_DIRS:
        jobs += [(path, path, bcb2clang.convert_includes) for path in _sources(root, directory, recursive=True)]
    for source, target, convert in jobs:
        text = bcb2clang.read_source(source)
        try:
            converted = convert(text)
        except bcb2clang.ConversionError as error:
            errors.append('%s: %s' % (source.relative_to(root), error))
            continue
        if (source != target) or (converted != text):
            target.parent.mkdir(parents=True, exist_ok=True)
            bcb2clang.write_source(target, converted)
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
