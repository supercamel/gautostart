#pragma once

#include <gautostart/gautostart.h>

G_BEGIN_DECLS

const char *_g_autostart_manager_peek_app_id       (GAutostartManager *self);
const char *_g_autostart_manager_peek_display_name (GAutostartManager *self);

G_END_DECLS
