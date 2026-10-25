#!/usr/bin/env python3
"""Fake services of a desktop shell for the tests of the desktop integration,
run inside a headless KWin session (tools/wlharness) on its session bus:

- the notification server (org.freedesktop.Notifications),
- the watcher of the system tray (org.kde.StatusNotifierWatcher, which Qt's
  QSystemTrayIcon needs to show its StatusNotifierItem),
- a listener of the progress of the task manager
  (com.canonical.Unity.LauncherEntry signals).

What the services get is logged as JSON lines. The tests act as the user
through org.openscp.TestDesktop at /org/openscp/TestDesktop: ClickNotification
(the default action of a notification) and ActivateTrayItem (a click on the
last registered tray icon).

    desktop_services.py --log FILE
"""

import argparse
import json
import sys

import gi

gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib  # noqa: E402

NOTIFICATIONS_XML = """
<node>
  <interface name="org.freedesktop.Notifications">
    <method name="Notify">
      <arg type="s" direction="in"/><arg type="u" direction="in"/><arg type="s" direction="in"/>
      <arg type="s" direction="in"/><arg type="s" direction="in"/><arg type="as" direction="in"/>
      <arg type="a{sv}" direction="in"/><arg type="i" direction="in"/><arg type="u" direction="out"/>
    </method>
    <method name="CloseNotification"><arg type="u" direction="in"/></method>
    <method name="GetCapabilities"><arg type="as" direction="out"/></method>
    <method name="GetServerInformation">
      <arg type="s" direction="out"/><arg type="s" direction="out"/><arg type="s" direction="out"/><arg type="s" direction="out"/>
    </method>
    <signal name="NotificationClosed"><arg type="u"/><arg type="u"/></signal>
    <signal name="ActionInvoked"><arg type="u"/><arg type="s"/></signal>
    <signal name="ActivationToken"><arg type="u"/><arg type="s"/></signal>
  </interface>
</node>
"""

WATCHER_XML = """
<node>
  <interface name="org.kde.StatusNotifierWatcher">
    <method name="RegisterStatusNotifierItem"><arg type="s" direction="in"/></method>
    <method name="RegisterStatusNotifierHost"><arg type="s" direction="in"/></method>
    <property name="RegisteredStatusNotifierItems" type="as" access="read"/>
    <property name="IsStatusNotifierHostRegistered" type="b" access="read"/>
    <property name="ProtocolVersion" type="i" access="read"/>
    <signal name="StatusNotifierItemRegistered"><arg type="s"/></signal>
    <signal name="StatusNotifierItemUnregistered"><arg type="s"/></signal>
    <signal name="StatusNotifierHostRegistered"/>
  </interface>
</node>
"""

CONTROL_XML = """
<node>
  <interface name="org.openscp.TestDesktop">
    <method name="ClickNotification"><arg type="u" direction="in"/></method>
    <method name="ActivateTrayItem"/>
  </interface>
</node>
"""


class DesktopServices:
    def __init__(self, log_path):
        self.log_file = open(log_path, 'a', buffering=1)
        self.bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        self.next_id = 1
        # service name -> (owner, path)
        self.items = {}

    def log(self, event, **data):
        self.log_file.write(json.dumps(dict(data, event=event)) + '\n')

    def register(self, path, xml, method_call, get_property=None):
        info = Gio.DBusNodeInfo.new_for_xml(xml)
        self.bus.register_object(path, info.interfaces[0], method_call, get_property, None)

    def own(self, name):
        reply = self.bus.call_sync('org.freedesktop.DBus', '/org/freedesktop/DBus', 'org.freedesktop.DBus',
                                   'RequestName', GLib.Variant('(su)', (name, 4)), None, Gio.DBusCallFlags.NONE, -1, None)
        if reply.unpack()[0] != 1:
            raise RuntimeError('cannot own %s' % name)

    def start(self):
        self.register('/org/freedesktop/Notifications', NOTIFICATIONS_XML, self.notifications_call)
        self.register('/StatusNotifierWatcher', WATCHER_XML, self.watcher_call, self.watcher_property)
        self.register('/org/openscp/TestDesktop', CONTROL_XML, self.control_call)
        for name in ('org.freedesktop.Notifications', 'org.kde.StatusNotifierWatcher', 'org.openscp.TestDesktop'):
            self.own(name)
        self.bus.signal_subscribe(None, 'com.canonical.Unity.LauncherEntry', 'Update', None, None,
                                  Gio.DBusSignalFlags.NONE, self.launcher_update)
        self.bus.signal_subscribe('org.freedesktop.DBus', 'org.freedesktop.DBus', 'NameOwnerChanged', None, None,
                                  Gio.DBusSignalFlags.NONE, self.name_owner_changed)
        self.log('ready')

    # --- org.freedesktop.Notifications -----------------------------------

    def notifications_call(self, connection, sender, path, interface, method, params, invocation):
        if method == 'Notify':
            app_name, replaces_id, icon, summary, body, actions, hints, timeout = params.unpack()
            notification_id = replaces_id or self.next_id
            self.next_id += 1
            self.log('notify', id=notification_id, app_name=app_name, icon=icon, summary=summary, body=body,
                     actions=actions, hints={key: str(value) for key, value in hints.items()}, timeout=timeout)
            invocation.return_value(GLib.Variant('(u)', (notification_id,)))
        elif method == 'CloseNotification':
            self.log('close_notification', id=params.unpack()[0])
            invocation.return_value(None)
        elif method == 'GetCapabilities':
            invocation.return_value(GLib.Variant('(as)', (['actions', 'body'],)))
        elif method == 'GetServerInformation':
            invocation.return_value(GLib.Variant('(ssss)', ('desktop_services', 'OpenSCP', '1.0', '1.2')))

    # --- org.kde.StatusNotifierWatcher -------------------------------------

    def watcher_call(self, connection, sender, path, interface, method, params, invocation):
        if method == 'RegisterStatusNotifierItem':
            service = params.unpack()[0]
            # A path instead of a service name: the item is on the sender's
            # connection
            item_path = '/StatusNotifierItem'
            if service.startswith('/'):
                service, item_path = sender, service
            self.items[service] = (sender, item_path)
            self.log('tray_register', service=service, path=item_path, sender=sender)
            connection.emit_signal(None, '/StatusNotifierWatcher', 'org.kde.StatusNotifierWatcher',
                                   'StatusNotifierItemRegistered', GLib.Variant('(s)', (service + item_path,)))
        else:
            self.log('tray_host', host=params.unpack()[0])
        invocation.return_value(None)

    def watcher_property(self, connection, sender, path, interface, name):
        if name == 'RegisteredStatusNotifierItems':
            return GLib.Variant('as', [service + path for service, (_, path) in self.items.items()])
        if name == 'IsStatusNotifierHostRegistered':
            return GLib.Variant('b', True)
        return GLib.Variant('i', 0)

    def name_owner_changed(self, connection, sender, path, interface, signal, params):
        name, old_owner, new_owner = params.unpack()
        for service, (owner, item_path) in list(self.items.items()):
            if not new_owner and name in (service, owner):
                del self.items[service]
                self.log('tray_unregister', service=service)
                connection.emit_signal(None, '/StatusNotifierWatcher', 'org.kde.StatusNotifierWatcher',
                                       'StatusNotifierItemUnregistered', GLib.Variant('(s)', (service + item_path,)))

    # --- com.canonical.Unity.LauncherEntry ---------------------------------

    def launcher_update(self, connection, sender, path, interface, signal, params):
        uri, properties = params.unpack()
        self.log('launcher', uri=uri, properties=properties)

    # --- org.openscp.TestDesktop -------------------------------------------

    def control_call(self, connection, sender, path, interface, method, params, invocation):
        if method == 'ClickNotification':
            notification_id = params.unpack()[0]
            connection.emit_signal(None, '/org/freedesktop/Notifications', 'org.freedesktop.Notifications',
                                   'ActivationToken', GLib.Variant('(us)', (notification_id, 'test-token')))
            connection.emit_signal(None, '/org/freedesktop/Notifications', 'org.freedesktop.Notifications',
                                   'ActionInvoked', GLib.Variant('(us)', (notification_id, 'default')))
            invocation.return_value(None)
        elif method == 'ActivateTrayItem':
            if not self.items:
                invocation.return_dbus_error('org.openscp.TestDesktop.NoItem', 'no tray item registered')
                return
            service, (owner, item_path) = list(self.items.items())[-1]
            connection.call_sync(service, item_path, 'org.kde.StatusNotifierItem', 'Activate',
                                 GLib.Variant('(ii)', (0, 0)), None, Gio.DBusCallFlags.NONE, -1, None)
            invocation.return_value(None)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--log', required=True)
    args = parser.parse_args()
    services = DesktopServices(args.log)
    services.start()
    GLib.MainLoop().run()
    return 0


if __name__ == '__main__':
    sys.exit(main())
