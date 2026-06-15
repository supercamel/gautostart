API Reference
=============

Header
------

Applications should include the umbrella header:

.. code-block:: c

   #include <gautostart/gautostart.h>

GAutostartManager
-----------------

``GAutostartManager`` is a final ``GObject`` that owns the application ID and
display name used by platform backends.

Constructor:

.. code-block:: c

   GAutostartManager *
   g_autostart_manager_new (const char *app_id,
                            const char *display_name);

``app_id`` must be non-NULL, non-empty, and valid UTF-8. ``display_name`` may be
NULL; when omitted, the manager uses ``app_id`` as the display name.

Accessors:

.. code-block:: c

   const char *g_autostart_manager_get_app_id       (GAutostartManager *self);
   const char *g_autostart_manager_get_display_name (GAutostartManager *self);

The returned strings are owned by the manager.

Status Query
------------

.. code-block:: c

   gboolean
   g_autostart_manager_is_registered (GAutostartManager  *self,
                                      GError            **error);

Returns ``TRUE`` when the application appears to be registered for autostart and
``FALSE`` otherwise. If a platform backend cannot complete the query, ``error``
is set.

Register
--------

.. code-block:: c

   void
   g_autostart_manager_register (GAutostartManager   *self,
                                 const char * const *command_line,
                                 const char          *reason,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data);

   gboolean
   g_autostart_manager_register_finish (GAutostartManager  *self,
                                        GAsyncResult       *result,
                                        GError            **error);

``command_line`` is a NULL-terminated UTF-8 argument vector. The first element
must exist and must not be empty. ``reason`` may be NULL; it is used where the
platform exposes a user-visible reason or comment.

Always finish the operation in the callback with
``g_autostart_manager_register_finish()``.

Unregister
----------

.. code-block:: c

   void
   g_autostart_manager_unregister (GAutostartManager   *self,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data);

   gboolean
   g_autostart_manager_unregister_finish (GAutostartManager  *self,
                                          GAsyncResult       *result,
                                          GError            **error);

Always finish the operation in the callback with
``g_autostart_manager_unregister_finish()``.

Errors
------

Validation errors are reported with ``G_IO_ERROR_INVALID_ARGUMENT``. Unsupported
platforms and unsupported backend operations use ``G_IO_ERROR_NOT_SUPPORTED``.
Platform I/O failures are propagated through ``GError`` using the relevant GLib
error domains.
