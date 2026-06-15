#pragma once

#include <gio/gio.h>
#include <gautostart/gautostart.h>

G_BEGIN_DECLS

gboolean _g_autostart_xdg_is_registered    (GAutostartManager  *self,
                                            gboolean           *registered,
                                            GError            **error);
void     _g_autostart_xdg_register         (GAutostartManager  *self,
                                            GTask              *task,
                                            const char * const *command_line,
                                            const char         *reason);
void     _g_autostart_xdg_unregister       (GAutostartManager *self,
                                            GTask             *task);

#if HAVE_LIBPORTAL
gboolean _g_autostart_portal_should_use    (void);
gboolean _g_autostart_portal_is_registered (GAutostartManager  *self,
                                            gboolean           *registered,
                                            GError            **error);
void     _g_autostart_portal_register      (GAutostartManager  *self,
                                            GTask              *task,
                                            const char * const *command_line,
                                            const char         *reason);
void     _g_autostart_portal_unregister    (GAutostartManager *self,
                                            GTask             *task);
#endif

G_END_DECLS
