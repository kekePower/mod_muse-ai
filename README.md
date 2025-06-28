# mod_muse-ai

An Apache HTTP Server module that brings MuseWeb-powered AI capabilities directly into the web-server layer.

## Current Status: Phase 1 - Proof-of-Concept ✅

This is the initial PoC implementation that demonstrates a working Apache module with configuration directives and a basic handler.

## Prerequisites

- **Apache HTTP Server 2.4+** with development headers
  ```bash
  # Ubuntu/Debian
  sudo apt-get install apache2-dev
  
  # CentOS/RHEL/Fedora
  sudo yum install httpd-devel
  # or
  sudo dnf install httpd-devel
  ```

- **Build Tools**
  ```bash
  # Ubuntu/Debian
  sudo apt-get install meson ninja-build gcc
  
  # CentOS/RHEL/Fedora
  sudo yum install meson ninja-build gcc
  ```

- **Apache APXS** (usually included with apache2-dev/httpd-devel)

## Quick Start

### 1. Build the Module

```bash
# Clone and enter the project directory
cd /home/stig/dev/ai/mod_muse-ai

# Set up build directory
meson setup build --project-name mod_muse-ai

# Build the module
ninja -C build

# Install the module (requires sudo)
ninja -C build install-module
```

### 2. Configure Apache

Add the following to your Apache configuration (usually `/etc/apache2/apache2.conf` or `/etc/httpd/conf/httpd.conf`):

```apache
# Load the module
LoadModule muse_ai_module modules/mod_muse_ai.so

# Configure MuseWeb settings (optional - these are defaults)
MuseAiEndpoint http://127.0.0.1:8080
MuseAiTimeout 300
MuseAiDebug Off

# Map requests to the AI handler
<Files "ai">
    SetHandler AI
</Files>

# Or map by file extension
# AddHandler AI .ai
```

### 3. Restart Apache

```bash
# Ubuntu/Debian
sudo systemctl restart apache2

# CentOS/RHEL/Fedora
sudo systemctl restart httpd

# Or use the convenience target
ninja -C build apache-restart
```

### 4. Test the Module

```bash
# Test with curl
curl http://localhost/ai

# Or use the convenience target
ninja -C build test-module

# Or open in browser
firefox http://localhost/ai
```

You should see a HTML page with the mod_muse-ai PoC greeting and current configuration.

## Configuration Directives

| Directive | Description | Default |
|-----------|-------------|---------|
| `MuseAiEndpoint` | MuseWeb server URL | `http://127.0.0.1:8080` |
| `MuseAiTimeout` | Request timeout in seconds | `300` |
| `MuseAiDebug` | Enable debug logging (`On`/`Off`) | `Off` |

## Development Commands

```bash
# Build only
ninja -C build

# Install module
ninja -C build install-module

# Restart Apache
ninja -C build apache-restart

# Reload Apache configuration
ninja -C build apache-reload

# Test the module
ninja -C build test
```

## Project Structure

```
mod_muse-ai/
├── mod_muse_ai.c           # Main module source code
├── meson.build             # Build configuration
├── README.md               # This file
└── apache-mod-muse-ai.md   # Complete development plan
```

## Troubleshooting

### Module Not Loading

1. Check Apache error logs:
   ```bash
   sudo tail -f /var/log/apache2/error.log
   # or
   sudo tail -f /var/log/httpd/error_log
   ```

2. Verify module is installed:
   ```bash
   ls -la /usr/lib/apache2/modules/mod_muse_ai.so
   # or
   ls -la /etc/httpd/modules/mod_muse_ai.so
   ```

3. Check Apache configuration syntax:
   ```bash
   sudo apache2ctl configtest
   # or
   sudo httpd -t
   ```

### Build Issues

1. Ensure Apache development headers are installed:
   ```bash
   apxs -q INCLUDEDIR
   ```

2. Check compiler and build tools:
   ```bash
   gcc --version
   meson --version
   ninja --version
   ```

### Handler Not Working

1. Verify handler mapping in Apache config
2. Check that the request URL matches your configuration
3. Enable debug logging with `MuseAiDebug On`

## What's Next?

This PoC demonstrates the basic Apache module infrastructure. The next phases will add:

- **Phase 2**: Integration with MuseWeb backend for AI-powered HTML generation
- **Phase 3**: Advanced features (streaming, caching, security)
- **Phase 4**: Production packaging and release

See `apache-mod-muse-ai.md` for the complete development roadmap.

## License

Apache License 2.0

## Contributing

This is currently in early development. Please refer to the development plan in `apache-mod-muse-ai.md` for the roadmap and contribution guidelines.
