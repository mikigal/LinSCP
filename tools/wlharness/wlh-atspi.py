#!/usr/bin/env python3
"""Finds the accessible objects of an application of a harness session over
AT-SPI and prints them as JSON lines: role, name, extents (in the client
area of the window, as Qt reports them on Wayland, where windows do not know
their position on the screen) and the names of their actions.

Runs inside the session (Session.accessibles), whose AT-SPI registry
Session.start_accessibility started; the applications run with
QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1.

    wlh-atspi.py APPLICATION [--name NAME] [--role ROLE] [--action ACTION]

--action performs the action (e.g. "Press", "Toggle") of the first object
found instead of printing.
"""

import argparse
import json
import sys

import gi

gi.require_version('Atspi', '2.0')
from gi.repository import Atspi  # noqa: E402

# Deep enough for the views of a file manager, bounded for large menus
MAX_DEPTH = 30
MAX_CHILDREN = 500


def describe(node):
    extents = node.get_extents(Atspi.CoordType.WINDOW)
    actions = []
    action = node.get_action_iface()
    if action is not None:
        actions = [action.get_action_name(i) for i in range(action.get_n_actions())]
    return {'role': node.get_role_name(), 'name': node.get_name(),
            'x': extents.x, 'y': extents.y, 'width': extents.width, 'height': extents.height,
            'showing': node.get_state_set().contains(Atspi.StateType.SHOWING),
            'actions': actions}


def walk(node, depth, found, args):
    try:
        info = describe(node)
    except gi.repository.GLib.GError:
        return
    if (args.name is None or info['name'] == args.name) and (args.role is None or info['role'] == args.role):
        found.append((node, info))
    if depth >= MAX_DEPTH:
        return
    for index in range(min(node.get_child_count(), MAX_CHILDREN)):
        child = node.get_child_at_index(index)
        if child is not None:
            walk(child, depth + 1, found, args)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('application')
    parser.add_argument('--name')
    parser.add_argument('--role')
    parser.add_argument('--action')
    args = parser.parse_args()

    desktop = Atspi.get_desktop(0)
    found = []
    for index in range(desktop.get_child_count()):
        application = desktop.get_child_at_index(index)
        if application is not None and application.get_name() == args.application:
            walk(application, 0, found, args)
    if args.action is not None:
        for node, info in found:
            if args.action in info['actions']:
                node.get_action_iface().do_action(info['actions'].index(args.action))
                return 0
        print('no object with the action %s' % args.action, file=sys.stderr)
        return 1
    for _, info in found:
        print(json.dumps(info))
    return 0


if __name__ == '__main__':
    sys.exit(main())
