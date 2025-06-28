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
apache_cflags = run_command(apxs, '-q', 'CFLAGS', check : true).stdout().strip()
apache_includes = run_command(apxs, '-q', 'INCLUDEDIR', check : true).stdout().strip()
apache_libexec = run_command(apxs, '-q', 'LIBEXECDIR', check : true).stdout().strip()

# Parse Apache CFLAGS to get include directories
apache_include_args = []
foreach flag : apache_cflags.split()
  if flag.startswith('-I')
    apache_include_args += flag
  endif
endforeach

# Add the main Apache include directory
apache_include_args += '-I' + apache_includes

message('Apache include arguments: ' + ' '.join(apache_include_args))
message('Apache module directory: ' + apache_libexec)

# Define the module source
mod_sources = files('src/mod_muse_ai.c')

# Custom target to build the module using apxs
mod_muse_ai = custom_target('mod_muse_ai.so',
  input : mod_sources,
  output : 'mod_muse_ai.so',
  command : [
    meson.current_source_dir() / 'build_module.sh',
    '@INPUT@',
    '@OUTPUT@',
    apxs.full_path()
  ],
  build_by_default : true,
  install : false  # We'll handle installation separately
)

# Custom install target
# Write the install script
install_script_file = configure_file(
  input : 'install_template.sh.in',
  output : 'install_module.sh',
  configuration : {
    'APXS': apxs.full_path(),
    'MODULE_SO': meson.current_build_dir() / 'mod_muse_ai.so',
    'APACHE_LIBEXEC': apache_libexec
  }
)

# Make the script executable and create install target
run_target('install-module',
  command : ['bash', meson.current_build_dir() / 'install_module.sh'],
  depends : mod_muse_ai
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
