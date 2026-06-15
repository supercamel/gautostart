#pragma once

#include <gio/gio.h>
#include <gautostart/gautostart.h>

G_BEGIN_DECLS

gboolean _g_autostart_platform_is_registered (GAutostartManager  *self,
                                              gboolean           *registered,
                                              GError            **error);

void _g_autostart_platform_register   (GAutostartManager  *self,
                                       GTask              *task,
                                       const char * const *command_line,
                                       const char         *reason);

void _g_autostart_platform_unregister (GAutostartManager *self,
                                       GTask             *task);

G_END_DECLS
