#include "gautostart-config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gautostart-manager-private.h"
#include "gautostart-platform.h"

#define RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"

static char *
win32_error_message (LSTATUS status)
{
  wchar_t *message_w = NULL;
  char *message = NULL;
  DWORD length;

  length = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           (DWORD) status,
                           0,
                           (LPWSTR) &message_w,
                           0,
                           NULL);

  if (length == 0)
    return g_strdup_printf ("Windows error %ld", (long) status);

  while (length > 0 &&
         (message_w[length - 1] == L'\r' ||
          message_w[length - 1] == L'\n' ||
          message_w[length - 1] == L' '))
    message_w[--length] = L'\0';

  message = g_utf16_to_utf8 ((const gunichar2 *) message_w, -1, NULL, NULL, NULL);
  LocalFree (message_w);

  if (message == NULL)
    return g_strdup_printf ("Windows error %ld", (long) status);

  return message;
}

static GError *
error_from_lstatus (const char *operation,
                    LSTATUS     status)
{
  g_autofree char *message = win32_error_message (status);

  return g_error_new (G_IO_ERROR,
                      g_io_error_from_win32_error ((gint) status),
                      "%s: %s",
                      operation,
                      message);
}

static wchar_t *
utf8_to_wchar (const char  *text,
               glong       *items_written,
               GError     **error)
{
  return (wchar_t *) g_utf8_to_utf16 (text, -1, NULL, items_written, error);
}

static char *
quote_windows_arg (const char *arg)
{
  gboolean needs_quotes = arg[0] == '\0';
  GString *quoted;
  gsize backslashes = 0;

  for (const char *p = arg; *p != '\0'; p++)
    {
      if (*p == ' ' || *p == '\t' || *p == '"')
        {
          needs_quotes = TRUE;
          break;
        }
    }

  if (!needs_quotes)
    return g_strdup (arg);

  quoted = g_string_new ("\"");

  for (const char *p = arg; *p != '\0'; p++)
    {
      if (*p == '\\')
        {
          backslashes++;
          continue;
        }

      if (*p == '"')
        {
          for (gsize i = 0; i < (backslashes * 2) + 1; i++)
            g_string_append_c (quoted, '\\');
          g_string_append_c (quoted, '"');
          backslashes = 0;
          continue;
        }

      for (gsize i = 0; i < backslashes; i++)
        g_string_append_c (quoted, '\\');
      backslashes = 0;
      g_string_append_c (quoted, *p);
    }

  for (gsize i = 0; i < backslashes * 2; i++)
    g_string_append_c (quoted, '\\');

  g_string_append_c (quoted, '"');

  return g_string_free (quoted, FALSE);
}

static char *
command_line_to_windows_string (const char * const *command_line)
{
  GString *joined = g_string_new (NULL);

  for (gsize i = 0; command_line[i] != NULL; i++)
    {
      g_autofree char *quoted = quote_windows_arg (command_line[i]);

      if (i > 0)
        g_string_append_c (joined, ' ');

      g_string_append (joined, quoted);
    }

  return g_string_free (joined, FALSE);
}

gboolean
_g_autostart_platform_is_registered (GAutostartManager  *self,
                                     gboolean           *registered,
                                     GError            **error)
{
  g_autofree wchar_t *value_name = NULL;
  HKEY key = NULL;
  LSTATUS status;

  *registered = FALSE;

  value_name = utf8_to_wchar (_g_autostart_manager_peek_app_id (self), NULL, error);
  if (value_name == NULL)
    return FALSE;

  status = RegOpenKeyExW (HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &key);
  if (status == ERROR_FILE_NOT_FOUND)
    return TRUE;
  if (status != ERROR_SUCCESS)
    {
      g_propagate_error (error, error_from_lstatus ("Failed to open autostart registry key", status));
      return FALSE;
    }

  status = RegQueryValueExW (key, value_name, NULL, NULL, NULL, NULL);
  RegCloseKey (key);

  if (status == ERROR_FILE_NOT_FOUND)
    return TRUE;
  if (status != ERROR_SUCCESS)
    {
      g_propagate_error (error, error_from_lstatus ("Failed to query autostart registry value", status));
      return FALSE;
    }

  *registered = TRUE;
  return TRUE;
}

void
_g_autostart_platform_register (GAutostartManager   *self,
                                GTask               *task,
                                const char * const  *command_line,
                                const char          *reason)
{
  g_autofree char *command = NULL;
  g_autofree wchar_t *value_name = NULL;
  g_autofree wchar_t *value_data = NULL;
  g_autoptr(GError) error = NULL;
  glong value_data_len = 0;
  HKEY key = NULL;
  LSTATUS status;

  (void) reason;

  command = command_line_to_windows_string (command_line);
  value_name = utf8_to_wchar (_g_autostart_manager_peek_app_id (self), NULL, &error);
  value_data = utf8_to_wchar (command, &value_data_len, &error);

  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  status = RegCreateKeyExW (HKEY_CURRENT_USER,
                            RUN_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &key,
                            NULL);
  if (status != ERROR_SUCCESS)
    {
      g_task_return_error (task, error_from_lstatus ("Failed to open autostart registry key", status));
      g_object_unref (task);
      return;
    }

  status = RegSetValueExW (key,
                           value_name,
                           0,
                           REG_SZ,
                           (const BYTE *) value_data,
                           (DWORD) ((value_data_len + 1) * sizeof (wchar_t)));
  RegCloseKey (key);

  if (status != ERROR_SUCCESS)
    g_task_return_error (task, error_from_lstatus ("Failed to write autostart registry value", status));
  else
    g_task_return_boolean (task, TRUE);

  g_object_unref (task);
}

void
_g_autostart_platform_unregister (GAutostartManager *self,
                                  GTask             *task)
{
  g_autofree wchar_t *value_name = NULL;
  g_autoptr(GError) error = NULL;
  HKEY key = NULL;
  LSTATUS status;

  value_name = utf8_to_wchar (_g_autostart_manager_peek_app_id (self), NULL, &error);
  if (error != NULL)
    {
      g_task_return_error (task, g_steal_pointer (&error));
      g_object_unref (task);
      return;
    }

  status = RegCreateKeyExW (HKEY_CURRENT_USER,
                            RUN_KEY,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &key,
                            NULL);
  if (status != ERROR_SUCCESS)
    {
      g_task_return_error (task, error_from_lstatus ("Failed to open autostart registry key", status));
      g_object_unref (task);
      return;
    }

  status = RegDeleteValueW (key, value_name);
  RegCloseKey (key);

  if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND)
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error_from_lstatus ("Failed to delete autostart registry value", status));

  g_object_unref (task);
}
