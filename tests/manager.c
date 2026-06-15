#include <gautostart/gautostart.h>

int
main (void)
{
  g_autoptr(GAutostartManager) manager = NULL;

  manager = g_autostart_manager_new ("com.example.App", "Example App");

  g_assert_nonnull (manager);
  g_assert_cmpstr (g_autostart_manager_get_app_id (manager), ==, "com.example.App");
  g_assert_cmpstr (g_autostart_manager_get_display_name (manager), ==, "Example App");
  g_assert_true (G_AUTOSTART_IS_MANAGER (manager));

  return 0;
}
