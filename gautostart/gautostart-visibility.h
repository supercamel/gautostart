#pragma once

#include <glib.h>

#if defined(_WIN32) && !defined(G_AUTOSTART_STATIC_COMPILATION)
# if defined(G_AUTOSTART_COMPILATION)
#  define G_AUTOSTART_API __declspec(dllexport)
# else
#  define G_AUTOSTART_API __declspec(dllimport)
# endif
#elif defined(__GNUC__) && __GNUC__ >= 4
# define G_AUTOSTART_API __attribute__((visibility("default"))) extern
#else
# define G_AUTOSTART_API extern
#endif
