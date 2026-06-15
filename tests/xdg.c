#include <gautostart/gautostart.h>

#include <glib/gstdio.h>

typedef struct
{
  GMainLoop *loop;
  gboolean ok;
  GError *error;
} AsyncState;

static void
register_cb (GObject      *object,
             GAsyncResult *result,
             gpointer      user_data)
{
  AsyncState *state = user_data;

  state->ok = g_autostart_manager_register_finish (G_AUTOSTART_MANAGER (object),
                                                   result,
                                                   &state->error);
  g_main_loop_quit (state->loop);
}

static void
unregister_cb (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  AsyncState *state = user_data;

  state->ok = g_autostart_manager_unregister_finish (G_AUTOSTART_MANAGER (object),
                                                     result,
                                                     &state->error);
  g_main_loop_quit (state->loop);
}

static void
run_desktop_file_validate (const char *path)
{
  g_autofree char *validator = NULL;
  g_autofree char *standard_output = NULL;
  g_autofree char *standard_error = NULL;
  g_autoptr(GError) error = NULL;
  char *argv[] = { NULL, NULL, NULL };
  int wait_status = 0;

  validator = g_find_program_in_path ("desktop-file-validate");
  if (validator == NULL)
    return;

  argv[0] = validator;
  argv[1] = (char *) path;

  if (!g_spawn_sync (NULL,
                     argv,
                     NULL,
                     G_SPAWN_DEFAULT,
                     NULL,
                     NULL,
                     &standard_output,
                     &standard_error,
                     &wait_status,
                     &error))
    g_error ("desktop-file-validate failed to run: %s", error->message);

  if (!g_spawn_check_wait_status (wait_status, &error))
    g_error ("desktop-file-validate failed: %s%s",
             error->message,
             standard_error != NULL ? standard_error : "");
}

static void
remove_tree (const char *path)
{
  g_autoptr(GDir) dir = NULL;
  const char *name;

  dir = g_dir_open (path, 0, NULL);
  if (dir == NULL)
    {
      g_remove (path);
      return;
    }

  while ((name = g_dir_read_name (dir)) != NULL)
    {
      g_autofree char *child = g_build_filename (path, name, NULL);

      remove_tree (child);
    }

  g_rmdir (path);
}

int
main (void)
{
  g_autoptr(GAutostartManager) manager = NULL;
  g_autoptr(GMainLoop) loop = NULL;
  g_autoptr(GKeyFile) key_file = NULL;
  g_autofree char *tmp_dir = NULL;
  g_autofree char *config_dir = NULL;
  g_autofree char *desktop_path = NULL;
  g_autofree char *name = NULL;
  g_autofree char *exec = NULL;
  g_autoptr(GError) error = NULL;
  const char *argv[] = {
    "/tmp/Example App/example-app",
    "--profile=has spaces",
    "--literal-percent=50%",
    "--money=$5",
    NULL,
  };
  AsyncState state = { 0 };

  tmp_dir = g_dir_make_tmp ("gautostart-xdg-XXXXXX", &error);
  g_assert_no_error (error);

  config_dir = g_build_filename (tmp_dir, "config", NULL);
  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);

  manager = g_autostart_manager_new ("com.example.App", "Example App");
  loop = g_main_loop_new (NULL, FALSE);

  state.loop = loop;
  g_assert_false (g_autostart_manager_is_registered (manager, &error));
  g_assert_no_error (error);

  g_autostart_manager_register (manager,
                                argv,
                                "Start with your desktop session",
                                NULL,
                                register_cb,
                                &state);
  g_main_loop_run (loop);

  g_assert_no_error (state.error);
  g_assert_true (state.ok);
  g_assert_true (g_autostart_manager_is_registered (manager, &error));
  g_assert_no_error (error);

  desktop_path = g_build_filename (config_dir, "autostart", "com.example.App.desktop", NULL);
  g_assert_true (g_file_test (desktop_path, G_FILE_TEST_IS_REGULAR));
  run_desktop_file_validate (desktop_path);

  key_file = g_key_file_new ();
  g_assert_true (g_key_file_load_from_file (key_file, desktop_path, G_KEY_FILE_NONE, &error));
  g_assert_no_error (error);
  name = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (name, ==, "Example App");

  exec = g_key_file_get_string (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (exec, ==, "\"/tmp/Example App/example-app\" \"--profile=has spaces\" --literal-percent=50%% \"--money=\\$5\"");

  g_clear_pointer (&state.error, g_error_free);
  state.ok = FALSE;
  g_autostart_manager_unregister (manager, NULL, unregister_cb, &state);
  g_main_loop_run (loop);

  g_assert_no_error (state.error);
  g_assert_true (state.ok);
  g_assert_false (g_autostart_manager_is_registered (manager, &error));
  g_assert_no_error (error);
  run_desktop_file_validate (desktop_path);

  g_key_file_free (g_steal_pointer (&key_file));
  key_file = g_key_file_new ();
  g_assert_true (g_key_file_load_from_file (key_file, desktop_path, G_KEY_FILE_NONE, &error));
  g_assert_no_error (error);
  g_assert_true (g_key_file_get_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_HIDDEN, &error));
  g_assert_no_error (error);

  remove_tree (tmp_dir);

  return 0;
}
