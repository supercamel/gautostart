#include "gautostart-config.h"

#include "gautostart-platform.h"

gboolean
_g_autostart_platform_is_registered (GAutostartManager  *self,
                                     gboolean           *registered,
                                     GError            **error)
{
  (void) self;
  (void) registered;

  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_NOT_SUPPORTED,
               "Autostart is not supported on this platform");
  return FALSE;
}

void
_g_autostart_platform_register (GAutostartManager   *self,
                                GTask               *task,
                                const char * const  *command_line,
                                const char          *reason)
{
  (void) self;
  (void) command_line;
  (void) reason;

  g_task_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Autostart is not supported on this platform");
  g_object_unref (task);
}

void
_g_autostart_platform_unregister (GAutostartManager *self,
                                  GTask             *task)
{
  (void) self;

  g_task_return_new_error (task,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Autostart is not supported on this platform");
  g_object_unref (task);
}
