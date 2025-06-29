project('mod_muse-ai', 'c',
  version : '0.1.0',
  license : 'Apache-2.0',
  default_options : [
    'c_std=c11',
    'warning_level=2',
    'werror=false'
  ]
)

# Find Apache development tools
apxs = find_program('apxs', required : true)

# Get Apache configuration
apache_cflags_str = run_command(apxs, '-q', 'CFLAGS', check: true).stdout().strip()
apache_ldflags_str = run_command(apxs, '-q', 'LDFLAGS_SHLIB', check: true).stdout().strip()
apache_libs_str = run_command(apxs, '-q', 'LIBS_SHLIB', check: true).stdout().strip()
apache_libexec = run_command(apxs, '-q', 'LIBEXECDIR', check: true).stdout().strip()

# Create a dependency object for Apache
cc = meson.get_compiler('c')
apache_dep = declare_dependency(
  include_directories: [
    include_directories('/usr/include/httpd'),
    include_directories('/usr/include/apr-1')
  ],
    compile_args: apache_cflags_str.split() + ['-D_REENTRANT', '-D_GNU_SOURCE'],
  link_args: apache_ldflags_str.split() + apache_libs_str.split()
)

# Find cJSON dependency
cjson_dep = dependency('libcjson', required: true)

message('Apache module directory: ' + apache_libexec)

# Define all C source files
source_files = [
  'src/mod_muse_ai.c',
  'src/streaming.c',
  'src/sanitize.c',
  'src/http_client.c',
  'src/utils.c',
  'src/connection_pool.c',
  'src/metrics.c',
  'src/advanced_config.c',
  'src/request_handlers.c',
  'src/supported_locales.c',
  'src/language_selection.c',
  'src/error_pages.c'
]

# Build the shared module using Meson's native capabilities
mod_muse_ai_so = shared_module('muse_ai',
  source_files,
  include_directories: include_directories('src'),
  dependencies: [apache_dep, cjson_dep],
  name_prefix: 'mod_',
  name_suffix: 'so',
  install: true,
  install_dir: apache_libexec
)

# Custom target to simplify installation during development
run_target('install-module',
  command: ['ninja', '-C', meson.project_build_root(), 'install'],
  depends: mod_muse_ai_so
)

# Development targets
run_target('apache-restart',
  command : ['sudo', 'systemctl', 'restart', 'apache2']
)

run_target('apache-reload',
  command : ['sudo', 'systemctl', 'reload', 'apache2']
)

run_target('test-module',
  command : ['curl', '-v', 'http://localhost/ai']
)

# Summary
summary({
  'Project': meson.project_name(),
  'Version': meson.project_version(),
  'Apache APXS': apxs.full_path(),
  'Apache Modules Dir': apache_libexec,
  'Build Type': get_option('buildtype'),
}, section: 'Configuration')

message('')
message('Build commands:')
message('  meson setup build')
message('  ninja -C build')
message('  ninja -C build install-module')
message('')
message('Development commands:')
message('  ninja -C build apache-restart  # Restart Apache')
message('  ninja -C build apache-reload   # Reload Apache config')
message('  ninja -C build test-module     # Test the module')
