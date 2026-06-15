# SQGI Autostart Demo

This demo uses SQGI to call the `GAutostart-1.0` introspection API from
Squirrel.

## Run From The Source Tree

Build the library first:

```sh
meson setup build -Dportal=disabled
meson compile -C build
```

Run the GTK4 demo against the in-tree library and typelib:

```sh
GI_TYPELIB_PATH="$PWD/build" \
LD_LIBRARY_PATH="$PWD/build" \
sqgi demo/sqgi-autostart/main.nut
```

To test without touching your real autostart settings, point `XDG_CONFIG_HOME`
at a temporary directory. You can use the GUI, or the CLI smoke-test flags:

```sh
tmp="$(mktemp -d)"
XDG_CONFIG_HOME="$tmp/config" \
GI_TYPELIB_PATH="$PWD/build" \
LD_LIBRARY_PATH="$PWD/build" \
sqgi demo/sqgi-autostart/main.nut --register

find "$tmp" -type f -maxdepth 4 -print -exec sed -n '1,80p' {} \;
rm -rf "$tmp"
```

Useful commands:

```sh
sqgi demo/sqgi-autostart/main.nut
sqgi demo/sqgi-autostart/main.nut --register
sqgi demo/sqgi-autostart/main.nut --register-cmd /bin/true --from-sqgi-demo
sqgi demo/sqgi-autostart/main.nut --unregister
sqgi demo/sqgi-autostart/main.nut --autostart
sqgi demo/sqgi-autostart/main.nut --started
sqgi demo/sqgi-autostart/main.nut --status-path
```

## Package

The manifest builds/stages the native `gautostart` library and its typelib.
It also bundles the SQGI icon for the GTK window and staged icon-theme data.
This demo uses a normal GTK window only; it does not create a persistent
tray/status icon.

```sh
cd demo/sqgi-autostart
sqgipkg --doctor
sqgipkg --target appimage
sqgipkg --target win-nsis
```

`target=all` is set in the manifest, so plain `sqgipkg` will try to build both
the Linux AppImage and the Windows NSIS output when the required toolchains are
available.
