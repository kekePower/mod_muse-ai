# mod_muse-ai Installation and Configuration Guide

A comprehensive step-by-step guide to install, configure, and use the mod_muse-ai Apache module for dynamic AI-powered web content generation.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Basic Configuration](#basic-configuration)
4. [Advanced Configuration](#advanced-configuration)
5. [Setting Up .ai File Handler](#setting-up-ai-file-handler)
6. [Creating Prompt Templates](#creating-prompt-templates)
7. [Testing Your Installation](#testing-your-installation)
8. [Troubleshooting](#troubleshooting)
9. [Production Deployment](#production-deployment)
10. [Configuration Reference](#configuration-reference)

---

## Prerequisites

### System Requirements

- **Apache HTTP Server 2.4+** with development headers
- **Linux/Unix operating system** (tested on Mageia, Ubuntu, CentOS, Fedora)
- **Build tools** (GCC, Meson, Ninja)
- **AI Backend Service** (Ollama, OpenAI API, or compatible service)

### Installing Dependencies

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install apache2-dev meson ninja-build gcc
```

#### CentOS/RHEL/Fedora
```bash
# CentOS/RHEL
sudo yum install httpd-devel meson ninja-build gcc

# Fedora
sudo dnf install httpd-devel meson ninja-build gcc
```

#### Verify Apache APXS
```bash
apxs -q INCLUDEDIR
# Should return something like: /usr/include/apache2
```

---

## Installation

### Step 1: Clone the Repository

```bash
cd /opt
sudo git clone https://github.com/kekePower/mod_muse-ai.git
cd mod_muse-ai
```

### Step 2: Build the Module

```bash
# Set up build directory
meson setup build --project-name mod_muse-ai

# Build the module
ninja -C build

# Install the module (requires sudo)
sudo ninja -C build install-module
```

### Step 3: Verify Installation

```bash
# Check if module was installed
ls -la /usr/lib/apache2/modules/mod_muse_ai.so
# or on CentOS/RHEL/Fedora:
ls -la /etc/httpd/modules/mod_muse_ai.so
```

---

## Basic Configuration

### Step 1: Load the Module

Add the following to your main Apache configuration file:

**Ubuntu/Debian** (`/etc/apache2/apache2.conf`):
```apache
LoadModule muse_ai_module modules/mod_muse_ai.so
```

**CentOS/RHEL/Fedora** (`/etc/httpd/conf/httpd.conf`):
```apache
LoadModule muse_ai_module modules/mod_muse_ai.so
```

### Step 2: Basic Configuration

Add these directives to configure the AI backend:

```apache
# Basic AI Configuration
MuseAiEndpoint "http://127.0.0.1:11434/v1"
MuseAiModel "llama3.2:latest"
MuseAiTimeout 300
MuseAiDebug On
MuseAiStreaming On
MuseAiMaxTokens 16384
```

**💡 Recommended**: Use the [`examples/mod_muse-ai-simple.conf`](examples/mod_muse-ai-simple.conf) configuration for now, as it includes all the currently working features. The advanced configuration options below are still in development.

### Step 3: Configure AI Handler

Add handler configuration for AI requests:

```apache
# AI Request Handler
<Location "/ai">
    SetHandler muse-ai-handler
    Require all granted
</Location>
```

### Step 4: Restart Apache

```bash
# Ubuntu/Debian
sudo systemctl restart apache2

# CentOS/RHEL/Fedora
sudo systemctl restart httpd

# Or use the build system convenience command
sudo ninja -C build apache-restart
```

---

## Advanced Configuration

> **⚠️ Development Status**: Many advanced features listed in this section are still in development. For a stable, working configuration, use the basic setup above or the [`examples/mod_muse-ai-simple.conf`](examples/mod_muse-ai-simple.conf) file.

### Commercial AI Providers

For commercial AI services (OpenAI, Google Gemini, Anthropic):

```apache
# Commercial AI Configuration
MuseAiEndpoint "https://api.openai.com/v1"
MuseAiApiKey "your-api-key-here"
MuseAiModel "gpt-4.1-nano"
MuseAiTimeout 60
```

### Performance Configuration

```apache
# Performance Settings
MuseAiPoolMaxConnections 20
MuseAiStreamingBufferSize 8192
MuseAiSecurityMaxRequestSize 1048576
```

### Caching and Rate Limiting

```apache
# Caching Configuration
MuseAiCacheEnable On
MuseAiCacheTTL 300

# Rate Limiting
MuseAiRateLimitEnable On
MuseAiRateLimitRPM 120
```

### Monitoring and Metrics

```apache
# Enable Metrics Collection
MuseAiMetricsEnable On

# Metrics Endpoint
<Location "/metrics">
    SetHandler metrics-handler
    Require all granted
</Location>

# Health Check Endpoint
<Location "/health">
    SetHandler health-handler
    Require all granted
</Location>
```

---

## Setting Up .ai File Handler

The .ai file handler allows you to create dynamic web pages using AI by placing `.ai` files in your document root.

### Step 1: Configure Prompts Directory

```apache
# Set up prompts directory
MuseAiPromptsDir "/etc/muse-ai/prompts"
```

### Step 2: Create Prompts Directory

```bash
sudo mkdir -p /etc/muse-ai/prompts
sudo chown apache:apache /etc/muse-ai/prompts  # or www-data:www-data on Ubuntu
```

### Step 3: Configure .ai File Handler

```apache
# Enable .ai file processing
<FilesMatch "\.ai$">
    SetHandler ai-file-handler
    Require all granted
</FilesMatch>
```

### Step 4: Enable .ai Files as Directory Index (Optional)

To allow `.ai` files to be served as directory index files (like `index.ai` when visiting a directory), add this configuration:

```apache
# Enable .ai files as directory index
<IfModule dir_module>
    DirectoryIndex index.html index.ai home.ai
</IfModule>
```

This allows visitors to access `http://localhost/` and automatically get `index.ai` if no `index.html` exists, or `http://localhost/somedir/` to get `somedir/index.ai` or `somedir/home.ai`.

---

## Creating Prompt Templates

### System Prompt Template

Create `/path/to/prompts/system_prompt.ai`:

```
You are a professional web content generator. Your role is to create high-quality, semantic HTML content based on user requests.

IMPORTANT RULES:
1. Always output valid HTML5 markup
2. Include proper DOCTYPE declaration
3. Use semantic HTML elements (header, main, section, article, aside, footer)
4. Ensure responsive design principles
5. Include appropriate meta tags and CSS links
6. No markdown formatting - pure HTML only

Your responses should be complete, well-structured HTML pages suitable for production use.
```

### Layout Prompt Template

Create `/path/to/prompts/layout.ai`:

```
Generate a complete HTML5 page with the following structure:

1. DOCTYPE html declaration
2. HTML element with lang attribute
3. Head section with:
   - UTF-8 charset meta tag
   - Viewport meta tag for responsive design
   - Title element
   - Link to /css/style.css stylesheet
4. Body with:
   - Header with site navigation
   - Main content area with id="content"
   - Footer with copyright and "Powered by MuseWeb" link

Use modern, clean CSS styling with:
- Responsive grid layout
- Professional typography
- Accessible color scheme
- Mobile-first design approach
```

### Example Page Content

Create `/path/to/prompts/index.ai`:

```
Create a homepage for a technology consulting company that specializes in AI integration. 

Include:
- Welcome message highlighting our AI expertise
- Overview of services (AI strategy, implementation, training)
- Brief company history and mission
- Call-to-action for consultation
- Professional, modern tone suitable for business clients

Focus on the content only - the system and layout prompts will handle the HTML structure and styling.
```

---

## Testing Your Installation

### Step 1: Test Basic AI Handler

```bash
# Test simple AI request
curl "http://localhost/ai?prompt=Hello%20world"

# Expected: AI-generated response
```

### Step 2: Test .ai File Processing

```bash
# Test .ai file handler
curl "http://localhost/index.ai"

# Expected: Complete HTML page generated from prompts
```

### Step 3: Test with Browser

Open your browser and navigate to:
- `http://localhost/ai?prompt=Write a simple HTML page`
- `http://localhost/index.ai`

### Step 4: Check Metrics and Health

```bash
# Check health status
curl "http://localhost/health"

# Check metrics
curl "http://localhost/metrics"
```

---

## Troubleshooting

### Module Not Loading

1. **Check Apache error logs:**
   ```bash
   # Ubuntu/Debian
   sudo tail -f /var/log/apache2/error.log
   
   # CentOS/RHEL/Fedora
   sudo tail -f /var/log/httpd/error_log
   ```

2. **Verify module installation:**
   ```bash
   # Check if module file exists
   ls -la /usr/lib/apache2/modules/mod_muse_ai.so
   
   # Check if module is loaded
   apache2ctl -M | grep muse
   # or
   httpd -M | grep muse
   ```

3. **Test Apache configuration:**
   ```bash
   # Ubuntu/Debian
   sudo apache2ctl configtest
   
   # CentOS/RHEL/Fedora
   sudo httpd -t
   ```

### Build Issues

1. **Missing Apache development headers:**
   ```bash
   # Verify APXS is available
   which apxs
   apxs -q INCLUDEDIR
   ```

2. **Missing build tools:**
   ```bash
   gcc --version
   meson --version
   ninja --version
   ```

3. **Permission issues:**
   ```bash
   # Ensure you have sudo access for installation
   sudo ninja -C build install-module
   ```

### Handler Not Working

1. **Check handler configuration:**
   ```apache
   # Ensure handler is properly mapped
   <Location "/ai">
       SetHandler muse-ai-handler
       Require all granted
   </Location>
   ```

2. **Enable debug logging:**
   ```apache
   MuseAiDebug On
   ```

3. **Check backend connectivity:**
   ```bash
   # Test if AI backend is running
   curl http://127.0.0.1:11434/v1/models
   ```

### .ai File Handler Issues

1. **Check prompts directory permissions:**
   ```bash
   ls -la /etc/muse-ai/prompts/
   sudo chown -R apache:apache /etc/muse-ai/prompts/
   ```

2. **Verify prompt files exist:**
   ```bash
   ls -la /etc/muse-ai/prompts/system_prompt.ai
   ls -la /etc/muse-ai/prompts/layout.ai
   ```

3. **Check .ai file permissions:**
   ```bash
   ls -la /var/www/html/*.ai
   ```

### Common Error Messages

**"Empty reply from server"**
- Check if AI backend is running
- Verify MuseAiEndpoint configuration
- Check network connectivity

**"Handler not found"**
- Verify LoadModule directive
- Check handler mapping configuration
- Restart Apache after configuration changes

**"Permission denied"**
- Check file permissions on module and prompt files
- Verify Apache user has access to required directories
- Check SELinux settings if applicable

---

## Production Deployment

### Security Considerations

1. **API Key Security:**
   ```apache
   # Store API keys securely
   MuseAiApiKey "your-secure-api-key"
   # Never log API keys in debug output
   ```

2. **Rate Limiting:**
   ```apache
   MuseAiRateLimitEnable On
   MuseAiRateLimitRPM 60
   ```

3. **Request Size Limits:**
   ```apache
   MuseAiSecurityMaxRequestSize 1048576  # 1MB limit
   ```

### Performance Optimization

1. **Connection Pooling:**
   ```apache
   MuseAiPoolMaxConnections 20
   ```

2. **Caching:**
   ```apache
   MuseAiCacheEnable On
   MuseAiCacheTTL 300
   ```

3. **Streaming Buffer:**
   ```apache
   MuseAiStreamingBufferSize 8192
   ```

### Monitoring Setup

1. **Enable Metrics:**
   ```apache
   MuseAiMetricsEnable On
   ```

2. **Set up log rotation:**
   ```bash
   # Add to logrotate configuration
   /var/log/apache2/muse-ai.log {
       daily
       missingok
       rotate 52
       compress
       notifempty
       create 644 www-data www-data
   }
   ```

---

## Configuration Reference

### Core Directives

| Directive | Type | Default | Description |
|-----------|------|---------|-------------|
| `MuseAiEndpoint` | String | `http://127.0.0.1:8080/v1` | AI backend base URL |
| `MuseAiModel` | String | `inception/mercury-small` | AI model identifier |
| `MuseAiApiKey` | String | `(none)` | API key for commercial providers |
| `MuseAiTimeout` | Integer | `300` | Request timeout in seconds |
| `MuseAiDebug` | Flag | `Off` | Enable debug logging |
| `MuseAiStreaming` | Flag | `On` | Enable streaming responses |
| `MuseAiMaxTokens` | Integer | `16384` | Maximum tokens for AI response generation (0 = no limit) |
| `MuseAiPromptsDir` | String | `(none)` | Directory containing prompt templates |

### Performance Directives

| Directive | Type | Default | Description |
|-----------|------|---------|-------------|
| `MuseAiPoolMaxConnections` | Integer | `10` | Maximum connections in pool |
| `MuseAiStreamingBufferSize` | Integer | `auto` | Streaming buffer size (auto-calculated from MuseAiMaxTokens) |
| `MuseAiSecurityMaxRequestSize` | Integer | `1048576` | Maximum request size (1MB) |

### Caching Directives

| Directive | Type | Default | Description |
|-----------|------|---------|-------------|
| `MuseAiCacheEnable` | Flag | `Off` | Enable response caching |
| `MuseAiCacheTTL` | Integer | `300` | Cache TTL in seconds |

### Rate Limiting Directives

| Directive | Type | Default | Description |
|-----------|------|---------|-------------|
| `MuseAiRateLimitEnable` | Flag | `Off` | Enable rate limiting |
| `MuseAiRateLimitRPM` | Integer | `60` | Requests per minute limit |

### Monitoring Directives

| Directive | Type | Default | Description |
|-----------|------|---------|-------------|
| `MuseAiMetricsEnable` | Flag | `On` | Enable metrics collection |
| `MuseAiReasoningModelPattern` | String | `reasoning` | Pattern for reasoning models |
| `MuseAiLoadBalanceMethod` | String | `round_robin` | Load balancing algorithm |

### Handler Types

| Handler | Path | Description |
|---------|------|-------------|
| `muse-ai-handler` | `/ai` | Basic AI request handler |
| `ai-file-handler` | `*.ai` | Dynamic .ai file processor |
| `metrics-handler` | `/metrics` | Prometheus-compatible metrics |
| `health-handler` | `/health` | System health check |

### Example Complete Configuration

```apache
# Load Module
LoadModule muse_ai_module modules/mod_muse_ai.so

# Core Configuration
MuseAiEndpoint "http://127.0.0.1:11434/v1"
MuseAiModel "llama3.2:latest"
MuseAiTimeout 300
MuseAiDebug Off
MuseAiStreaming On
MuseAiMaxTokens 16384
MuseAiPromptsDir "/etc/muse-ai/prompts"

# Performance Configuration
MuseAiPoolMaxConnections 20
# MuseAiStreamingBufferSize auto-calculated from MuseAiMaxTokens (16384 * 4 = 65536 bytes)
MuseAiSecurityMaxRequestSize 1048576

# Caching Configuration
MuseAiCacheEnable On
MuseAiCacheTTL 300

# Rate Limiting
MuseAiRateLimitEnable On
MuseAiRateLimitRPM 120

# Monitoring
MuseAiMetricsEnable On

# Virtual Host
<VirtualHost *:80>
    ServerName example.com
    DocumentRoot /var/www/html
    
    # AI Request Handler
    <Location "/ai">
        SetHandler muse-ai-handler
        Require all granted
    </Location>
    
    # .ai File Handler
    <FilesMatch "\.ai$">
        SetHandler ai-file-handler
        Require all granted
    </FilesMatch>
    
    # Metrics Endpoint
    <Location "/metrics">
        SetHandler metrics-handler
        Require all granted
    </Location>
    
    # Health Check Endpoint
    <Location "/health">
        SetHandler health-handler
        Require all granted
    </Location>
</VirtualHost>
```

---

## Development Commands

For developers working on the module:

```bash
# Build only
ninja -C build

# Install module
sudo ninja -C build install-module

# Restart Apache
sudo ninja -C build apache-restart

# Reload Apache configuration
sudo ninja -C build apache-reload

# Test the module
ninja -C build test

# Clean build
rm -rf build/
meson setup build --project-name mod_muse-ai
```

---

## Support and Contributing

- **Documentation**: See `docs/` directory for detailed technical documentation
- **Issues**: Report bugs and feature requests on GitHub
- **Contributing**: See `CONTRIBUTING.md` for development guidelines
- **License**: Apache License 2.0

For additional help, check the project's GitHub repository or contact the maintainers.
