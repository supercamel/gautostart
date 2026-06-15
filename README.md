# GAutostart

GAutostart is a small C/GObject library for registering and unregistering an
application to start automatically for the current user session.

The library is GObject Introspection friendly and builds with Meson. Linux host
apps use XDG autostart desktop files in `$XDG_CONFIG_HOME/autostart` (usually
`~/.config/autostart`). Sandboxed Linux apps use the XDG Desktop Portal
Background API through libportal when libportal support is available. Windows
uses the current user's `Software\Microsoft\Windows\CurrentVersion\Run`
registry key.

This intentionally targets user login/session startup, not privileged pre-login
system boot services.

## Build

```sh
meson setup build
meson compile -C build
meson test -C build
```

By default the build generates `GAutostart-1.0.gir` and the matching typelib
when GObject Introspection is available.

libportal is optional on Linux:

```sh
meson setup build -Dportal=auto
meson setup build-no-portal -Dportal=disabled
```

## Documentation

The Read the Docs/Sphinx documentation lives in `docs/`.

```sh
python -m pip install -r docs/requirements.txt
python -m sphinx -W -b html docs docs/_build/html
```

## C API Sketch

```c
GAutostartManager *manager;
const char *argv[] = { "/usr/bin/example-app", "--background", NULL };

manager = g_autostart_manager_new ("com.example.App", "Example App");

g_autostart_manager_register (manager,
                              argv,
                              "Start Example App when you log in",
                              NULL,
                              callback,
                              NULL);
```

On Linux host apps, unregistering writes a `Hidden=true` entry in the user's
autostart directory. That disables both the user entry and any system-wide
autostart entry with the same filename, as defined by the XDG autostart spec.

For AppImages, pass a command path that will remain stable. If the user moves
the AppImage after registration, the generated autostart entry will point at the
old path.

## SQGI Demo

`demo/sqgi-autostart/` contains a GTK4 SQGI app that imports `GAutostart-1.0`
through GI. It uses the SQGI icon, opens a normal GTK window, and lets you
toggle autostart with a checkbox. The demo intentionally does not include a
persistent tray/status icon.
