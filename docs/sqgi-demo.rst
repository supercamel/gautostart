SQGI Demo
=========

``demo/sqgi-autostart/`` contains a GTK4 SQGI application that imports the
``GAutostart-1.0`` introspection API from Squirrel. It opens a normal GTK window
and lets the user toggle autostart with a checkbox.

Run From The Source Tree
------------------------

Build the library first:

.. code-block:: sh

   meson setup build -Dportal=disabled
   meson compile -C build

Run the demo against the in-tree library and typelib:

.. code-block:: sh

   GI_TYPELIB_PATH="$PWD/build" \
   LD_LIBRARY_PATH="$PWD/build" \
   sqgi demo/sqgi-autostart/main.nut

To test without touching real autostart settings, point ``XDG_CONFIG_HOME`` at
a temporary directory:

.. code-block:: sh

   tmp="$(mktemp -d)"
   XDG_CONFIG_HOME="$tmp/config" \
   GI_TYPELIB_PATH="$PWD/build" \
   LD_LIBRARY_PATH="$PWD/build" \
   sqgi demo/sqgi-autostart/main.nut --register

   find "$tmp" -maxdepth 4 -type f -print -exec sed -n '1,80p' {} \;
   rm -rf "$tmp"

Packaging
---------

The SQGI manifest stages the native ``gautostart`` library and its typelib. It
also bundles the SQGI icon for the GTK window and staged icon-theme data.

Useful packaging checks:

.. code-block:: sh

   cd demo/sqgi-autostart
   sqgipkg --doctor
   sqgipkg --target appimage
   sqgipkg --target win-nsis
