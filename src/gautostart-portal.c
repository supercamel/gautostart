#include "gautostart-config.h"

#include "gautostart-linux-private.h"
#include "gautostart-manager-private.h"

#include <libportal/portal.h>

typedef struct
{
  GTask *task;
  XdpPortal *portal;
  GStrv command_line;
  GPtrArray *commandline_array;
} PortalCall;

static GStrv
strv_dup_const (const char * const *strv)
{
  gsize len = 0;
  GStrv copy;

  while (strv[len] != NULL)
    len++;

  copy = g_new0 (char *, len + 1);

  for (gsize i = 0; i < len; i++)
    copy[i] = g_strdup (strv[i]);

  return copy;
}

static GPtrArray *
strv_to_ptr_array (GStrv strv)
{
  GPtrArray *array = g_ptr_array_new ();

  for (gsize i = 0; strv[i] != NULL; i++)
    g_ptr_array_add (array, strv[i]);

  return array;
}

static void
portal_call_free (PortalCall *call)
{
  g_clear_object (&call->task);
  g_clear_object (&call->portal);
  g_clear_pointer (&call->command_line, g_strfreev);
  g_clear_pointer (&call->commandline_array, g_ptr_array_unref);
  g_free (call);
}

static void
request_background_cb (GObject      *object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  PortalCall *call = user_data;
  g_autoptr(GError) error = NULL;
  gboolean ok;

  (void) object;

  ok = xdp_portal_request_background_finish (call->portal, result, &error);

  if (error != NULL)
    g_task_return_error (call->task, g_steal_pointer (&error));
  else if (!ok)
    g_task_return_new_error (call->task,
                             G_IO_ERROR,
                             G_IO_ERROR_PERMISSION_DENIED,
                             "Autostart request was not granted");
  else
    g_task_return_boolean (call->task, TRUE);

  portal_call_free (call);
}

static XdpPortal *
new_portal_or_complete (GTask *task)
{
  g_autoptr(GError) error = NULL;
  XdpPortal *portal;

  portal = xdp_portal_initable_new (&error);
  if (portal == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
    }

  return portal;
}

gboolean
_g_autostart_portal_should_use (void)
{
  return xdp_portal_running_under_sandbox ();
}

gboolean
_g_autostart_portal_is_registered (GAutostartManager  *self,
                                   gboolean           *registered,
                                   GError            **error)
{
  (void) self;
  (void) registered;

  g_set_error (error,
               G_IO_ERROR,
               G_IO_ERROR_NOT_SUPPORTED,
               "Portal autostart status queries are not supported");
  return FALSE;
}

void
_g_autostart_portal_register (GAutostartManager   *self,
                              GTask               *task,
                              const char * const  *command_line,
                              const char          *reason)
{
  PortalCall *call;
  XdpPortal *portal;
  const char *request_reason;

  portal = new_portal_or_complete (task);
  if (portal == NULL)
    return;

  call = g_new0 (PortalCall, 1);
  call->task = task;
  call->portal = portal;
  call->command_line = strv_dup_const (command_line);
  call->commandline_array = strv_to_ptr_array (call->command_line);

  request_reason = reason != NULL ? reason : _g_autostart_manager_peek_display_name (self);

  xdp_portal_request_background (call->portal,
                                 NULL,
                                 (char *) request_reason,
                                 call->commandline_array,
                                 XDP_BACKGROUND_FLAG_AUTOSTART,
                                 g_task_get_cancellable (task),
                                 request_background_cb,
                                 call);
}

void
_g_autostart_portal_unregister (GAutostartManager *self,
                                GTask             *task)
{
  PortalCall *call;
  XdpPortal *portal;

  (void) self;

  portal = new_portal_or_complete (task);
  if (portal == NULL)
    return;

  call = g_new0 (PortalCall, 1);
  call->task = task;
  call->portal = portal;

  xdp_portal_request_background (call->portal,
                                 NULL,
                                 NULL,
                                 NULL,
                                 XDP_BACKGROUND_FLAG_NONE,
                                 g_task_get_cancellable (task),
                                 request_background_cb,
                                 call);
}
