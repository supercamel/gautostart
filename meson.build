project(
  'gautostart',
  'c',
  version: '0.1.0',
  default_options: ['c_std=c11', 'warning_level=2'],
)

api_version = '1.0'
host_system = host_machine.system()

cc = meson.get_compiler('c')
gnome = import('gnome')
pkg = import('pkgconfig')

glib_dep = dependency('glib-2.0', version: '>=2.58')
gobject_dep = dependency('gobject-2.0', version: '>=2.58')
gio_dep = dependency('gio-2.0', version: '>=2.58')
deps = [glib_dep, gobject_dep, gio_dep]
requires_private = []

config_h = configuration_data()
config_h.set_quoted('G_AUTOSTART_VERSION', meson.project_version())
config_h.set10('G_AUTOSTART_OS_WINDOWS', host_system == 'windows')
config_h.set10('G_AUTOSTART_OS_LINUX', host_system == 'linux')

platform_sources = []
if host_system == 'linux'
  libportal_dep = dependency(
    'libportal',
    version: '>=0.7',
    required: get_option('portal'),
  )
  if libportal_dep.found()
    deps += libportal_dep
    requires_private += ['libportal >= 0.7']
  endif

  config_h.set10('HAVE_LIBPORTAL', libportal_dep.found())
  platform_sources += files(
    'src/gautostart-linux.c',
    'src/gautostart-xdg.c',
  )
  if libportal_dep.found()
    platform_sources += files('src/gautostart-portal.c')
  endif
elif host_system == 'windows'
  advapi32_dep = cc.find_library('advapi32', required: true)
  deps += advapi32_dep
  config_h.set10('HAVE_LIBPORTAL', false)
  platform_sources += files('src/gautostart-windows.c')
else
  config_h.set10('HAVE_LIBPORTAL', false)
  platform_sources += files('src/gautostart-unsupported.c')
endif

configure_file(
  output: 'gautostart-config.h',
  configuration: config_h,
)

public_headers = files(
  'gautostart/gautostart.h',
  'gautostart/gautostart-manager.h',
  'gautostart/gautostart-visibility.h',
)

sources = files('src/gautostart-manager.c') + platform_sources
root_inc = include_directories('.')

gautostart_lib = library(
  'gautostart-@0@'.format(api_version),
  sources,
  public_headers,
  c_args: ['-DG_AUTOSTART_COMPILATION'],
  gnu_symbol_visibility: 'hidden',
  include_directories: root_inc,
  dependencies: deps,
  version: '0.0.0',
  soversion: '0',
  install: true,
)

install_headers(
  public_headers,
  subdir: 'gautostart-@0@/gautostart'.format(api_version),
)

pkg.generate(
  gautostart_lib,
  name: 'gautostart',
  description: 'Cross-platform current-user autostart registration library',
  filebase: 'gautostart-@0@'.format(api_version),
  subdirs: 'gautostart-@0@'.format(api_version),
  version: meson.project_version(),
  requires: ['glib-2.0', 'gobject-2.0', 'gio-2.0'],
  requires_private: requires_private,
)

introspection_dep = dependency(
  'gobject-introspection-1.0',
  required: get_option('introspection'),
)

if introspection_dep.found()
  gnome.generate_gir(
    gautostart_lib,
    sources: sources + public_headers,
    namespace: 'GAutostart',
    nsversion: api_version,
    identifier_prefix: 'GAutostart',
    symbol_prefix: 'g_autostart',
    includes: ['GObject-2.0', 'Gio-2.0'],
    header: 'gautostart/gautostart.h',
    install: true,
    dependencies: deps,
    extra_args: ['--warn-all'],
  )
endif

manager_smoke = executable(
  'manager-smoke',
  'tests/manager.c',
  include_directories: root_inc,
  dependencies: deps,
  link_with: gautostart_lib,
)

test('manager-smoke', manager_smoke)

if host_system == 'linux'
  xdg_smoke = executable(
    'xdg-smoke',
    'tests/xdg.c',
    include_directories: root_inc,
    dependencies: deps,
    link_with: gautostart_lib,
  )

  test('xdg-smoke', xdg_smoke)
endif
