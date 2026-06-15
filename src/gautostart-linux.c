#include "gautostart-config.h"

#include "gautostart-linux-private.h"
#include "gautostart-platform.h"

gboolean
_g_autostart_platform_is_registered (GAutostartManager  *self,
                                     gboolean           *registered,
                                     GError            **error)
{
#if HAVE_LIBPORTAL
  if (_g_autostart_portal_should_use ())
    return _g_autostart_portal_is_registered (self, registered, error);
#endif

  return _g_autostart_xdg_is_registered (self, registered, error);
}

void
_g_autostart_platform_register (GAutostartManager   *self,
                                GTask               *task,
                                const char * const  *command_line,
                                const char          *reason)
{
#if HAVE_LIBPORTAL
  if (_g_autostart_portal_should_use ())
    {
      _g_autostart_portal_register (self, task, command_line, reason);
      return;
    }
#endif

  _g_autostart_xdg_register (self, task, command_line, reason);
}

void
_g_autostart_platform_unregister (GAutostartManager *self,
                                  GTask             *task)
{
#if HAVE_LIBPORTAL
  if (_g_autostart_portal_should_use ())
    {
      _g_autostart_portal_unregister (self, task);
      return;
    }
#endif

  _g_autostart_xdg_unregister (self, task);
}
