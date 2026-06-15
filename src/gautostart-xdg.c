#include "gautostart-config.h"

#include "gautostart-linux-private.h"
#include "gautostart-manager-private.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <string.h>

static char *
desktop_file_basename (const char *app_id)
{
  GString *basename = g_string_new (NULL);

  for (const char *p = app_id; *p != '\0'; p++)
    {
      if (*p == '/' || *p == '\\')
        g_string_append_c (basename, '_');
      else
        g_string_append_c (basename, *p);
    }

  if (!g_str_has_suffix (basename->str, ".desktop"))
    g_string_append (basename, ".desktop");

  return g_string_free (basename, FALSE);
}

static char *
autostart_file_path (GAutostartManager *self)
{
  g_autofree char *basename = NULL;
  g_autofree char *autostart_dir = NULL;

  basename = desktop_file_basename (_g_autostart_manager_peek_app_id (self));
  autostart_dir = g_build_filename (g_get_user_config_dir (), "autostart", NULL);

  return g_build_filename (autostart_dir, basename, NULL);
}

static gboolean
ensure_autostart_dir (GError **error)
{
  g_autofree char *autostart_dir = NULL;

  autostart_dir = g_build_filename (g_get_user_config_dir (), "autostart", NULL);

  if (g_mkdir_with_parents (autostart_dir, 0700) == -1)
    {
      int saved_errno = errno;

      g_set_error (error,
                   G_FILE_ERROR,
                   g_file_error_from_errno (saved_errno),
                   "Failed to create autostart directory '%s': %s",
                   autostart_dir,
                   g_strerror (saved_errno));
      return FALSE;
    }

  return TRUE;
}

static gboolean
exec_arg_needs_quotes (const char *arg)
{
  static const char reserved[] = " \t\n\"'\\><~|&;$*?#()`";

  if (arg[0] == '\0')
    return TRUE;

  for (const char *p = arg; *p != '\0'; p++)
    {
      if (strchr (reserved, *p) != NULL)
        return TRUE;
    }

  return FALSE;
}

static char *
quote_exec_arg (const char *arg)
{
  GString *quoted = g_string_new (NULL);
  gboolean needs_quotes = exec_arg_needs_quotes (arg);

  if (needs_quotes)
    g_string_append_c (quoted, '"');

  for (const char *p = arg; *p != '\0'; p++)
    {
      if (*p == '%')
        {
          g_string_append (quoted, "%%");
          continue;
        }

      if (needs_quotes &&
          (*p == '"' || *p == '`' || *p == '$' || *p == '\\'))
        g_string_append_c (quoted, '\\');

      g_string_append_c (quoted, *p);
    }

  if (needs_quotes)
    g_string_append_c (quoted, '"');

  return g_string_free (quoted, FALSE);
}

static char *
command_line_to_exec (const char * const *command_line)
{
  GString *exec = g_string_new (NULL);

  for (gsize i = 0; command_line[i] != NULL; i++)
    {
      g_autofree char *quoted = NULL;

      quoted = quote_exec_arg (command_line[i]);

      if (i > 0)
        g_string_append_c (exec, ' ');

      g_string_append (exec, quoted);
    }

  return g_string_free (exec, FALSE);
}

static gboolean
save_key_file (GKeyFile     *key_file,
               const char   *path,
               GError      **error)
{
  g_autofree char *data = NULL;
  gsize length = 0;

  data = g_key_file_to_data (key_file, &length, error);
  if (data == NULL)
    return FALSE;

  return g_file_set_contents (path, data, (gssize) length, error);
}

gboolean
_g_autostart_xdg_is_registered (GAutostartManager  *self,
                                gboolean           *registered,
                                GError            **error)
{
  g_autoptr(GKeyFile) key_file = NULL;
  g_autofree char *path = NULL;
  g_autofree char *exec = NULL;
  gboolean hidden = FALSE;

  path = autostart_file_path (self);
  *registered = FALSE;

  if (!g_file_test (path, G_FILE_TEST_IS_REGULAR))
    return TRUE;

  key_file = g_key_file_new ();
  if (!g_key_file_load_from_file (key_file, path, G_KEY_FILE_NONE, error))
    return FALSE;

  if (g_key_file_has_key (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL))
    {
      hidden = g_key_file_get_boolean (key_file,
                                       G_KEY_FILE_DESKTOP_GROUP,
                                       G_KEY_FILE_DESKTOP_KEY_HIDDEN,
                                       error);
      if (error != NULL && *error != NULL)
        return FALSE;
    }

  if (hidden)
    return TRUE;

  exec = g_key_file_get_string (key_file,
                                G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_EXEC,
                                NULL);
  *registered = exec != NULL && exec[0] != '\0';

  return TRUE;
}

void
_g_autostart_xdg_register (GAutostartManager   *self,
                           GTask               *task,
                           const char * const  *command_line,
                           const char          *reason)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GKeyFile) key_file = NULL;
  g_autofree char *exec = NULL;
  g_autofree char *path = NULL;
  const char *app_id;
  const char *display_name;

  if (strchr (command_line[0], '=') != NULL)
    {
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_ARGUMENT,
                               "XDG autostart executable path must not contain '='");
      g_object_unref (task);
      return;
    }

  if (!ensure_autostart_dir (&error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  app_id = _g_autostart_manager_peek_app_id (self);
  display_name = _g_autostart_manager_peek_display_name (self);
  exec = command_line_to_exec (command_line);
  path = autostart_file_path (self);

  key_file = g_key_file_new ();
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, "Application");
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, display_name);
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, exec);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TERMINAL, FALSE);
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-GAutostart-AppId", app_id);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-GAutostart-Managed", TRUE);

  if (reason != NULL && reason[0] != '\0')
    g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_COMMENT, reason);

  if (!save_key_file (key_file, path, &error))
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

void
_g_autostart_xdg_unregister (GAutostartManager *self,
                             GTask             *task)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(GKeyFile) key_file = NULL;
  g_autofree char *path = NULL;
  const char *app_id;
  const char *display_name;

  if (!ensure_autostart_dir (&error))
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  app_id = _g_autostart_manager_peek_app_id (self);
  display_name = _g_autostart_manager_peek_display_name (self);
  path = autostart_file_path (self);

  key_file = g_key_file_new ();
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_TYPE, "Application");
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, display_name);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, TRUE);
  g_key_file_set_string (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-GAutostart-AppId", app_id);
  g_key_file_set_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-GAutostart-Managed", TRUE);

  if (!save_key_file (key_file, path, &error))
    g_task_return_error (task, g_steal_pointer (&error));
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}
