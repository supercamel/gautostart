Platform Behavior
=================

Linux Host Apps
---------------

When an app is not running inside a sandbox, GAutostart writes an XDG autostart
desktop file under:

.. code-block:: text

   $XDG_CONFIG_HOME/autostart

If ``XDG_CONFIG_HOME`` is not set, GLib normally resolves this to
``~/.config/autostart``.

The application ID becomes the desktop-file basename. Slashes and backslashes
are replaced with underscores, and ``.desktop`` is appended when it is not
already present.

Registration writes a desktop entry with:

* ``Type=Application``
* ``Name`` from the manager display name
* ``Exec`` from the escaped command line
* ``Terminal=false``
* ``Comment`` from the registration reason, when provided
* ``X-GAutostart-AppId`` and ``X-GAutostart-Managed`` metadata

Unregistration writes a desktop entry with ``Hidden=true``. This follows the
XDG autostart behavior for disabling both the user's entry and any system-wide
entry with the same filename.

Sandboxed Linux Apps
--------------------

When libportal support is built and the app is running under a sandbox,
GAutostart uses the XDG Desktop Portal Background API.

Registration requests the ``AUTOSTART`` background flag. Unregistration sends a
background request without the autostart flag. The portal controls whether the
request is granted and may show UI to the user.

The portal backend does not support registration status queries, so
``g_autostart_manager_is_registered()`` fails with
``G_IO_ERROR_NOT_SUPPORTED`` in this mode.

Windows
-------

Windows registration writes the escaped command line to the current user's
registry key:

.. code-block:: text

   HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run

The application ID is used as the registry value name. Unregistration deletes
that value. Querying status checks whether the value exists.

Unsupported Platforms
---------------------

On platforms without a backend, register, unregister, and status queries fail
with ``G_IO_ERROR_NOT_SUPPORTED``.
