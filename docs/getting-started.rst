Getting Started
===============

Requirements
------------

GAutostart builds with Meson and a C11 compiler.

Required dependencies:

* GLib 2.58 or newer
* GObject 2.58 or newer
* GIO 2.58 or newer

Optional dependencies:

* gobject-introspection, for ``GAutostart-1.0.gir`` and the matching typelib
* libportal 0.7 or newer on Linux, for sandboxed Background portal requests

Build
-----

Build and test from the repository root:

.. code-block:: sh

   meson setup build
   meson compile -C build
   meson test -C build

The default Linux build enables libportal automatically when it is available.
To make the choice explicit:

.. code-block:: sh

   meson setup build -Dportal=auto
   meson setup build-no-portal -Dportal=disabled

Install
-------

Install with Meson after compiling:

.. code-block:: sh

   meson install -C build

Installed C projects can discover the library through pkg-config:

.. code-block:: sh

   pkg-config --cflags --libs gautostart-1.0

Include the public header:

.. code-block:: c

   #include <gautostart/gautostart.h>
