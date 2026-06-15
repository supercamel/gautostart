#include "gautostart-config.h"

#include "gautostart-manager-private.h"
#include "gautostart-platform.h"

struct _GAutostartManager
{
  GObject parent_instance;

  char *app_id;
  char *display_name;
};

G_DEFINE_TYPE (GAutostartManager, g_autostart_manager, G_TYPE_OBJECT)

enum
{
  PROP_0,
  PROP_APP_ID,
  PROP_DISPLAY_NAME,
  N_PROPS,
};

static GParamSpec *properties[N_PROPS];

static gboolean
validate_utf8_string (const char  *value,
                      const char  *name,
                      gboolean     allow_empty,
                      GError     **error)
{
  if (value == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "%s must not be NULL",
                   name);
      return FALSE;
    }

  if (!allow_empty && value[0] == '\0')
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "%s must not be empty",
                   name);
      return FALSE;
    }

  if (!g_utf8_validate (value, -1, NULL))
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "%s must be valid UTF-8",
                   name);
      return FALSE;
    }

  return TRUE;
}

static gboolean
validate_command_line (const char * const *command_line,
                       GError           **error)
{
  if (command_line == NULL || command_line[0] == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "command-line must contain at least argv[0]");
      return FALSE;
    }

  if (command_line[0][0] == '\0')
    {
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_INVALID_ARGUMENT,
                   "command-line argv[0] must not be empty");
      return FALSE;
    }

  for (gsize i = 0; command_line[i] != NULL; i++)
    {
      if (!g_utf8_validate (command_line[i], -1, NULL))
        {
          g_set_error (error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_ARGUMENT,
                       "command-line argument %" G_GSIZE_FORMAT " must be valid UTF-8",
                       i);
          return FALSE;
        }
    }

  return TRUE;
}

static void
g_autostart_manager_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GAutostartManager *self = G_AUTOSTART_MANAGER (object);

  switch (prop_id)
    {
    case PROP_APP_ID:
      g_value_set_string (value, self->app_id);
      break;

    case PROP_DISPLAY_NAME:
      g_value_set_string (value, self->display_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_autostart_manager_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GAutostartManager *self = G_AUTOSTART_MANAGER (object);

  switch (prop_id)
    {
    case PROP_APP_ID:
      g_free (self->app_id);
      self->app_id = g_value_dup_string (value);
      break;

    case PROP_DISPLAY_NAME:
      g_free (self->display_name);
      self->display_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_autostart_manager_constructed (GObject *object)
{
  GAutostartManager *self = G_AUTOSTART_MANAGER (object);

  G_OBJECT_CLASS (g_autostart_manager_parent_class)->constructed (object);

  if (self->display_name == NULL && self->app_id != NULL)
    self->display_name = g_strdup (self->app_id);
}

static void
g_autostart_manager_finalize (GObject *object)
{
  GAutostartManager *self = G_AUTOSTART_MANAGER (object);

  g_clear_pointer (&self->app_id, g_free);
  g_clear_pointer (&self->display_name, g_free);

  G_OBJECT_CLASS (g_autostart_manager_parent_class)->finalize (object);
}

static void
g_autostart_manager_class_init (GAutostartManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = g_autostart_manager_get_property;
  object_class->set_property = g_autostart_manager_set_property;
  object_class->constructed = g_autostart_manager_constructed;
  object_class->finalize = g_autostart_manager_finalize;

  properties[PROP_APP_ID] =
    g_param_spec_string ("app-id",
                         "App ID",
                         "Stable application identifier",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  properties[PROP_DISPLAY_NAME] =
    g_param_spec_string ("display-name",
                         "Display Name",
                         "Human-readable application name",
                         NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
g_autostart_manager_init (GAutostartManager *self)
{
  (void) self;
}

/**
 * g_autostart_manager_new:
 * @app_id: a stable application identifier
 * @display_name: (nullable): a human-readable application name
 *
 * Creates a manager for registering the current user session autostart entry.
 *
 * On Windows, @app_id is used as the `Run` registry value name. On Linux host
 * apps, @app_id is used as the XDG autostart desktop filename.
 *
 * Returns: (transfer full): a new #GAutostartManager
 */
GAutostartManager *
g_autostart_manager_new (const char *app_id,
                         const char *display_name)
{
  g_return_val_if_fail (app_id != NULL, NULL);
  g_return_val_if_fail (app_id[0] != '\0', NULL);
  g_return_val_if_fail (g_utf8_validate (app_id, -1, NULL), NULL);
  g_return_val_if_fail (display_name == NULL || g_utf8_validate (display_name, -1, NULL), NULL);

  return g_object_new (G_AUTOSTART_TYPE_MANAGER,
                       "app-id", app_id,
                       "display-name", display_name,
                       NULL);
}

/**
 * g_autostart_manager_get_app_id:
 * @self: a #GAutostartManager
 *
 * Returns the stable application identifier.
 *
 * Returns: (transfer none): the application identifier
 */
const char *
g_autostart_manager_get_app_id (GAutostartManager *self)
{
  g_return_val_if_fail (G_AUTOSTART_IS_MANAGER (self), NULL);

  return self->app_id;
}

/**
 * g_autostart_manager_get_display_name:
 * @self: a #GAutostartManager
 *
 * Returns the human-readable application name.
 *
 * Returns: (transfer none): the display name
 */
const char *
g_autostart_manager_get_display_name (GAutostartManager *self)
{
  g_return_val_if_fail (G_AUTOSTART_IS_MANAGER (self), NULL);

  return self->display_name;
}

/**
 * g_autostart_manager_is_registered:
 * @self: a #GAutostartManager
 * @error: return location for a #GError, or %NULL
 *
 * Checks whether this application appears to be registered for autostart.
 *
 * Returns: %TRUE when autostart is registered, otherwise %FALSE
 */
gboolean
g_autostart_manager_is_registered (GAutostartManager  *self,
                                   GError            **error)
{
  gboolean registered = FALSE;

  g_return_val_if_fail (G_AUTOSTART_IS_MANAGER (self), FALSE);

  if (!validate_utf8_string (self->app_id, "app-id", FALSE, error))
    return FALSE;

  if (!_g_autostart_platform_is_registered (self, &registered, error))
    return FALSE;

  return registered;
}

/**
 * g_autostart_manager_register:
 * @self: a #GAutostartManager
 * @command_line: (array zero-terminated=1) (element-type utf8): command line to run at session startup
 * @reason: (nullable): user-visible reason for the request
 * @cancellable: (nullable): optional cancellable
 * @callback: (scope async) (nullable): callback to invoke when complete
 * @user_data: user data for @callback
 *
 * Registers @command_line to start automatically when the current user logs in.
 *
 * On Linux host apps this writes an XDG autostart desktop file. Sandboxed
 * Linux apps use the Background portal when libportal support is available.
 * On Windows this writes the command line to the current user's `Run` registry
 * key.
 */
void
g_autostart_manager_register (GAutostartManager   *self,
                              const char * const *command_line,
                              const char          *reason,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  g_autoptr(GError) error = NULL;
  GTask *task;

  g_return_if_fail (G_AUTOSTART_IS_MANAGER (self));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_autostart_manager_register);

  if (g_task_return_error_if_cancelled (task))
    {
      g_object_unref (task);
      return;
    }

  if (!validate_utf8_string (self->app_id, "app-id", FALSE, &error) ||
      !validate_command_line (command_line, &error) ||
      (reason != NULL && !validate_utf8_string (reason, "reason", TRUE, &error)))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  _g_autostart_platform_register (self, task, command_line, reason);
}

/**
 * g_autostart_manager_register_finish:
 * @self: a #GAutostartManager
 * @result: the async result
 * @error: return location for a #GError, or %NULL
 *
 * Finishes a call to g_autostart_manager_register().
 *
 * Returns: %TRUE on success, otherwise %FALSE with @error set
 */
gboolean
g_autostart_manager_register_finish (GAutostartManager  *self,
                                     GAsyncResult       *result,
                                     GError            **error)
{
  g_return_val_if_fail (G_AUTOSTART_IS_MANAGER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == g_autostart_manager_register, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * g_autostart_manager_unregister:
 * @self: a #GAutostartManager
 * @cancellable: (nullable): optional cancellable
 * @callback: (scope async) (nullable): callback to invoke when complete
 * @user_data: user data for @callback
 *
 * Removes the current user's session autostart entry for this application.
 */
void
g_autostart_manager_unregister (GAutostartManager   *self,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  g_autoptr(GError) error = NULL;
  GTask *task;

  g_return_if_fail (G_AUTOSTART_IS_MANAGER (self));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_autostart_manager_unregister);

  if (g_task_return_error_if_cancelled (task))
    {
      g_object_unref (task);
      return;
    }

  if (!validate_utf8_string (self->app_id, "app-id", FALSE, &error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  _g_autostart_platform_unregister (self, task);
}

/**
 * g_autostart_manager_unregister_finish:
 * @self: a #GAutostartManager
 * @result: the async result
 * @error: return location for a #GError, or %NULL
 *
 * Finishes a call to g_autostart_manager_unregister().
 *
 * Returns: %TRUE on success, otherwise %FALSE with @error set
 */
gboolean
g_autostart_manager_unregister_finish (GAutostartManager  *self,
                                       GAsyncResult       *result,
                                       GError            **error)
{
  g_return_val_if_fail (G_AUTOSTART_IS_MANAGER (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == g_autostart_manager_unregister, FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

const char *
_g_autostart_manager_peek_app_id (GAutostartManager *self)
{
  return self->app_id;
}

const char *
_g_autostart_manager_peek_display_name (GAutostartManager *self)
{
  return self->display_name;
}
