Usage
=====

Create a manager for the application, then register an argv-style command line
with the asynchronous API.

Registering Autostart
---------------------

.. code-block:: c

   #include <gautostart/gautostart.h>

   static void
   register_cb (GObject      *object,
                GAsyncResult *result,
                gpointer      user_data)
   {
     g_autoptr(GError) error = NULL;

     if (!g_autostart_manager_register_finish (G_AUTOSTART_MANAGER (object),
                                               result,
                                               &error))
       {
         g_warning ("Autostart registration failed: %s", error->message);
         return;
       }
   }

   void
   enable_autostart (void)
   {
     g_autoptr(GAutostartManager) manager = NULL;
     const char *argv[] = {
       "/usr/bin/example-app",
       "--background",
       NULL,
     };

     manager = g_autostart_manager_new ("com.example.App", "Example App");
     g_autostart_manager_register (manager,
                                   argv,
                                   "Start Example App when you log in",
                                   NULL,
                                   register_cb,
                                   NULL);
   }

The command line is passed as a NULL-terminated array. ``argv[0]`` must be
present, non-empty, and valid UTF-8. Each argument is escaped for the target
platform by the library.

Checking Status
---------------

Use ``g_autostart_manager_is_registered()`` to check whether the current user
has an active autostart entry:

.. code-block:: c

   g_autoptr(GError) error = NULL;
   gboolean registered;

   registered = g_autostart_manager_is_registered (manager, &error);
   if (error != NULL)
     g_warning ("Unable to query autostart state: %s", error->message);

Sandboxed Linux apps using the Background portal cannot query registration
status through this library; the portal backend reports ``G_IO_ERROR_NOT_SUPPORTED``.

Unregistering Autostart
-----------------------

.. code-block:: c

   static void
   unregister_cb (GObject      *object,
                  GAsyncResult *result,
                  gpointer      user_data)
   {
     g_autoptr(GError) error = NULL;

     if (!g_autostart_manager_unregister_finish (G_AUTOSTART_MANAGER (object),
                                                 result,
                                                 &error))
       g_warning ("Autostart removal failed: %s", error->message);
   }

   g_autostart_manager_unregister (manager, NULL, unregister_cb, NULL);

Choosing Identifiers And Commands
---------------------------------

Use a stable reverse-DNS application ID such as ``com.example.App``. On Linux
host systems this becomes the desktop-file name, with a ``.desktop`` suffix
added when needed. On Windows it is the value name under the current user's
``Run`` registry key.

For self-contained bundles such as AppImages, register a command path that will
remain stable. If the bundle is moved after registration, the stored autostart
entry still points to the old path.
