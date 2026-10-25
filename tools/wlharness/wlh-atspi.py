#!/usr/bin/env python3
"""Finds the accessible objects of an application of a harness session over
AT-SPI and prints them as JSON lines: role, name, extents (in the client
area of the window, as Qt reports them on Wayland, where windows do not know
their position on the screen) and the names of their actions.

Runs inside the session (Session.accessibles), whose AT-SPI registry
Session.start_accessibility started; the applications run with
QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1.

    wlh-atspi.py APPLICATION [--name NAME] [--role ROLE] [--action ACTION]
    wlh-atspi.py APPLICATION --popup --name NAME --action ACTION [--ready-file FILE]

--action performs the action (e.g. "Press", "Toggle") of the first object
found instead of printing. Popup menus are not in the tree of their
application: --popup waits for one that shows with the object (writing
FILE once it listens) and performs its action.
"""

import argparse
import json
import sys

import gi

gi.require_version('Atspi', '2.0')
from gi.repository import Atspi, GLib  # noqa: E402

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


def perform(found, action):
    for node, info in found:
        if action in info['actions']:
            node.get_action_iface().do_action(info['actions'].index(action))
            return True
    return False


def watch_popup(args):
    result = {'code': 1}

    def on_event(event):
        source = event.source
        try:
            if (event.detail1 != 1) or (source.get_role_name() not in ('popup menu', 'menu')) or \
               (source.get_application().get_name() != args.application):
                return
        except gi.repository.GLib.GError:
            return
        found = []
        walk(source, 0, found, args)
        if perform(found, args.action):
            result['code'] = 0
            Atspi.event_quit()

    listener = Atspi.EventListener.new(on_event)
    listener.register('object:state-changed:showing')
    if args.ready_file is not None:
        with open(args.ready_file, 'w') as ready:
            ready.write('listening\n')
    GLib.timeout_add(args.timeout * 1000, lambda: Atspi.event_quit() and False)
    Atspi.event_main()
    if result['code'] != 0:
        print('no popup menu with %s showed' % args.name, file=sys.stderr)
    return result['code']


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('application')
    parser.add_argument('--name')
    parser.add_argument('--role')
    parser.add_argument('--action')
    parser.add_argument('--popup', action='store_true')
    parser.add_argument('--ready-file')
    parser.add_argument('--timeout', type=int, default=20)
    args = parser.parse_args()
    if args.popup:
        return watch_popup(args)

    desktop = Atspi.get_desktop(0)
    found = []
    for index in range(desktop.get_child_count()):
        application = desktop.get_child_at_index(index)
        if application is not None and application.get_name() == args.application:
            walk(application, 0, found, args)
    if args.action is not None:
        if perform(found, args.action):
            return 0
        print('no object with the action %s' % args.action, file=sys.stderr)
        return 1
    for _, info in found:
        print(json.dumps(info))
    return 0


if __name__ == '__main__':
    sys.exit(main())
