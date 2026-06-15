#pragma once

#if !defined(__G_AUTOSTART_H_INSIDE__) && !defined(G_AUTOSTART_COMPILATION)
# error "Only <gautostart/gautostart.h> can be included directly."
#endif

#include <gio/gio.h>
#include <gautostart/gautostart-visibility.h>

G_BEGIN_DECLS

#define G_AUTOSTART_TYPE_MANAGER (g_autostart_manager_get_type())

G_AUTOSTART_API
G_DECLARE_FINAL_TYPE (GAutostartManager, g_autostart_manager, G_AUTOSTART, MANAGER, GObject)

G_AUTOSTART_API
GAutostartManager *g_autostart_manager_new (const char *app_id,
                                            const char *display_name);

G_AUTOSTART_API
const char        *g_autostart_manager_get_app_id (GAutostartManager *self);

G_AUTOSTART_API
const char        *g_autostart_manager_get_display_name (GAutostartManager *self);

G_AUTOSTART_API
gboolean           g_autostart_manager_is_registered (GAutostartManager *self,
                                                      GError           **error);

G_AUTOSTART_API
void               g_autostart_manager_register (GAutostartManager  *self,
                                                 const char * const *command_line,
                                                 const char         *reason,
                                                 GCancellable       *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer            user_data);

G_AUTOSTART_API
gboolean           g_autostart_manager_register_finish (GAutostartManager *self,
                                                       GAsyncResult      *result,
                                                       GError           **error);

G_AUTOSTART_API
void               g_autostart_manager_unregister (GAutostartManager  *self,
                                                   GCancellable       *cancellable,
                                                   GAsyncReadyCallback callback,
                                                   gpointer            user_data);

G_AUTOSTART_API
gboolean           g_autostart_manager_unregister_finish (GAutostartManager *self,
                                                         GAsyncResult      *result,
                                                         GError           **error);

G_END_DECLS
