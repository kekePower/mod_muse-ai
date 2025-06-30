# mod_muse-ai Installation and Configuration Guide

A comprehensive step-by-step guide to install, configure, and use the mod_muse-ai Apache module for dynamic AI-powered web content generation.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Basic Configuration](#basic-configuration)
4. [Advanced Configuration](#advanced-configuration)
5. [Translation System Setup](#translation-system-setup)
6. [Setting Up .ai File Handler](#setting-up-ai-file-handler)
7. [Creating Prompt Templates](#creating-prompt-templates)
8. [Testing Your Installation](#testing-your-installation)
9. [Troubleshooting](#troubleshooting)
10. [Production Deployment](#production-deployment)
11. [Configuration Reference](#configuration-reference)

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
    # Enable the AI module for this specific location
    MuseAiEnable On
    SetHandler enhanced-muse-ai-handler
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

`mod_muse-ai` now includes a powerful, per-directory caching system that integrates with Apache's `mod_cache` and `mod_cache_socache` modules to significantly improve performance and reduce backend load.

#### Prerequisites for Caching

Before enabling caching in `mod_muse-ai`, you must ensure that `mod_cache` and `mod_cache_socache` are loaded in your main Apache configuration (`httpd.conf` or `apache2.conf`):

```apache
LoadModule cache_module modules/mod_cache.so
LoadModule cache_socache_module modules/mod_cache_socache.so
```

#### Caching Configuration

You can control caching behavior on a per-directory or per-location basis. This allows you to fine-tune caching strategies for different parts of your website.

**Directives:**

- `MuseAiCacheEnable On|Off`: Enables or disables caching for a specific directory or location. Default is `Off`.
- `MuseAiCacheTTL seconds`: Sets the cache Time-To-Live (TTL) in seconds. This determines how long a cached response is considered fresh. The default is `300` seconds (5 minutes).

**Example Configuration:**

This example demonstrates how to enable caching for a specific documentation section of a website, with a longer TTL for less frequently updated content.

```apache
# Global cache settings (can be overridden per-directory)
CacheSocache shmcb:muse-ai-cache(512000) # 500KB shared memory cache

<Location "/docs/">
    # Enable mod_muse-ai caching for this location
    MuseAiCacheEnable On
    MuseAiCacheTTL 3600 # Cache documents for 1 hour

    # Instruct mod_cache to use the socache provider for this location
    CacheEnable socache
</Location>

<Location "/blog/">
    # Enable caching for the blog with a shorter TTL
    MuseAiCacheEnable On
    MuseAiCacheTTL 600 # Cache blog posts for 10 minutes
    CacheEnable socache
</Location>
```

> **Note**: Caching is automatically disabled for requests where `MuseAiStreaming` is `On`, as caching is not compatible with streaming responses.

#### Rate Limiting (Phase 3 - In Development)

The following directives are planned for a future release and are not yet functional:

```apache
# Rate Limiting (Not yet implemented)
# MuseAiRateLimitEnable On
# MuseAiRateLimitRPM 120
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

## Translation System Setup

mod_muse-ai includes a powerful AI translation system that supports 46 languages with automatic language detection and intelligent translation routing.

### Overview

The translation system supports multiple language selection methods:
- **URL Prefixes**: `/es/page.ai`, `/fr/docs/file.ai`
- **Query Parameters**: `?lang=es`, `?locale=fr_FR`
- **Cookies**: Remembers user language preferences
- **Accept-Language Headers**: Browser language preferences
- **Fallback**: Default to original file language

### Supported Languages

The system supports 46 languages organized by proficiency tiers:

**Tier 1 (High Quality)**: English (US/UK), German, Spanish (ES/MX), French, Italian, Japanese, Portuguese (BR/PT), Chinese (Simplified/Traditional)

**Tier 2 (Good Quality)**: Arabic, Bengali, Czech, Danish, Greek, Finnish, Hebrew, Hindi, Hungarian, Korean, Norwegian, Polish, Russian, Swedish, Thai, Turkish, Ukrainian, Vietnamese

**Tier 3 (Functional)**: Bulgarian, Catalan, Croatian, Dutch, Estonian, Latvian, Lithuanian, Romanian, Slovak, Slovenian

For the complete list, see `docs/pruned-languages.csv`.

### Basic Translation Setup

#### Step 1: Enable URL Rewriting (Recommended)

Add to your Apache configuration or `.htaccess`:

```apache
# Enable mod_rewrite
RewriteEngine On

# Handle language-prefixed URLs
RewriteRule ^([a-z]{2,3})/(.+\.ai)$ /$2 [L,E=MUSE_LANG_PREFIX:$1]
RewriteRule ^([a-z]{2,3})/(.+/)$ /$2index.ai [L,E=MUSE_LANG_PREFIX:$1]
RewriteRule ^([a-z]{2,3})/?$ /index.ai [L,E=MUSE_LANG_PREFIX:$1]

# Configure .ai file handling
<FilesMatch "\.ai$">
    SetHandler muse-ai-handler
</FilesMatch>

# Handle language-prefixed URLs
<LocationMatch "^/[a-z]{2,3}/.+\.ai$">
    SetHandler muse-ai-handler
</LocationMatch>
```

#### Step 2: Alternative Simple Setup

If you can't use URL rewriting, the translation system works automatically with query parameters:

```apache
# Basic setup - no rewriting needed
<FilesMatch "\.ai$">
    SetHandler muse-ai-handler
    SetEnv MUSE_DEFAULT_LANG "en_US"
</FilesMatch>
```

Users can then access translations via:
- `page.ai?lang=es` (Spanish)
- `page.ai?locale=fr_FR` (French)
- `page.ai?language=de` (German)

### Advanced Translation Configuration

#### Complete Virtual Host Example

```apache
<VirtualHost *:80>
    ServerName multilingual.example.com
    DocumentRoot /var/www/html
    
    # Basic mod_muse_ai configuration
    MuseAiApiKey "your-api-key-here"
    MuseAiEndpoint "https://api.openai.com/v1/chat/completions"
    MuseAiModel "gpt-4"
    MuseAiMaxTokens 2000
    
    # Translation configuration
    RewriteEngine On
    
    # Language prefix rules
    RewriteRule ^/([a-z]{2,3})/(.+\.ai)$ /$2 [E=MUSE_LANG_PREFIX:$1,L]
    RewriteRule ^/([a-z]{2,3})/(.+/)$ /$2index.ai [E=MUSE_LANG_PREFIX:$1,L]
    RewriteRule ^/([a-z]{2,3})/?$ /index.ai [E=MUSE_LANG_PREFIX:$1,L]
    
    # Add trailing slash to language codes
    RewriteRule ^/([a-z]{2,3})$ /$1/ [R=301,L]
    
    # Language switching utility
    RewriteCond %{QUERY_STRING} ^to=([a-z]{2,3})&return=(.+)$
    RewriteRule ^/switch-lang$ /%1/%2? [R=302,L]
    
    # Configure handlers
    <FilesMatch "\.ai$">
        SetHandler muse-ai-handler
        SetEnv MUSE_DEFAULT_LANG "en_US"
    </FilesMatch>
    
    <LocationMatch "^/[a-z]{2,3}/.+\.ai$">
        SetHandler muse-ai-handler
    </LocationMatch>
</VirtualHost>
```

#### Using .htaccess

For shared hosting or when you can't modify the main Apache config:

```apache
# Copy examples/.htaccess-translation to your document root as .htaccess
RewriteEngine On

# Language prefix handling
RewriteRule ^([a-z]{2,3})/(.+\.ai)$ /$2 [L,E=MUSE_LANG_PREFIX:$1]
RewriteRule ^([a-z]{2,3})/(.+/)$ /$2index.ai [L,E=MUSE_LANG_PREFIX:$1]
RewriteRule ^([a-z]{2,3})/?$ /index.ai [L,E=MUSE_LANG_PREFIX:$1]

<FilesMatch "\.ai$">
    SetHandler muse-ai-handler
    SetEnv MUSE_DEFAULT_LANG "en_US"
</FilesMatch>
```

### Testing Translation

#### Step 1: Create Test Content

Create `/var/www/html/welcome.ai`:

```
Welcome to our website! This is a test page for the translation system.

Features:
- Automatic language detection
- High-quality AI translation
- Support for 46 languages
- SEO-friendly URLs

Please explore our content in your preferred language.
```

#### Step 2: Test Different Access Methods

```bash
# Test URL prefix (if rewriting enabled)
curl http://localhost/es/welcome.ai
curl http://localhost/fr/welcome.ai
curl http://localhost/de/welcome.ai

# Test query parameters
curl "http://localhost/welcome.ai?lang=es"
curl "http://localhost/welcome.ai?locale=fr_FR"
curl "http://localhost/welcome.ai?language=de"

# Test with Accept-Language header
curl -H "Accept-Language: es-ES,es;q=0.9" http://localhost/welcome.ai
curl -H "Accept-Language: fr-FR,fr;q=0.8" http://localhost/welcome.ai
```

#### Step 3: Test Language Switching

If you implemented the language switching utility:

```bash
# Switch language and redirect
curl "http://localhost/switch-lang?to=es&return=welcome.ai"
# Should redirect to: /es/welcome.ai
```

### Language Detection Priority

The system uses this priority order:

1. **URL Prefix** (`/es/page.ai`) - Highest priority
2. **Query Parameter** (`?lang=es`) - Explicit user choice
3. **Cookie** (`muse_lang=es`) - Remembered preference
4. **Accept-Language Header** - Browser preference
5. **Fallback** - Original file language or configured default

### Enhanced Language Code System

**The system now supports both short codes with smart defaults and full locale codes for precise control:**

#### Smart Defaults for Short Codes

| Short Code | Smart Default | Reasoning |
|------------|---------------|----------|
| `zh` | `zh_CN` (Simplified Chinese) | Most widely used (1+ billion users) |
| `pt` | `pt_BR` (Brazilian Portuguese) | Larger population than European Portuguese |
| `es` | `es_ES` (Spain Spanish) | Original/standard Spanish |
| `en` | `en_US` (US English) | Most common on the web |

#### Full Locale Code Support

**URL Format:** Use hyphens in URLs, converted to underscores internally

| URL Format | Internal Locale | Language |
|------------|----------------|----------|
| `/zh-cn/page` | `zh_CN` | Simplified Chinese |
| `/zh-tw/page` | `zh_TW` | Traditional Chinese |
| `/pt-br/page` | `pt_BR` | Brazilian Portuguese |
| `/pt-pt/page` | `pt_PT` | European Portuguese |
| `/es-es/page` | `es_ES` | Spain Spanish |
| `/es-mx/page` | `es_MX` | Mexican Spanish |
| `/en-us/page` | `en_US` | US English |
| `/en-gb/page` | `en_GB` | British English |

**Examples:**
- `/zh/page` → Smart default: Simplified Chinese (`zh_CN`)
- `/zh-tw/page` → Explicit: Traditional Chinese (`zh_TW`)
- `/pt/page` → Smart default: Brazilian Portuguese (`pt_BR`)
- `/pt-pt/page` → Explicit: European Portuguese (`pt_PT`)

**Best Practice:** Use short codes for common cases, full locale codes when precision matters.

### Translation Quality

The system includes proficiency tiers to help you understand translation quality:

- **Tier 1 (High)**: Excellent translations with nuanced understanding
- **Tier 2 (Good)**: Reliable translations suitable for most content
- **Tier 3 (Functional)**: Basic translations, good for understanding

The tier information is automatically included in translation instructions to the AI.

### Troubleshooting Translation

#### Common Issues

**Language not detected:**
```bash
# Check Apache error logs
tail -f /var/log/apache2/error.log

# Look for language detection messages
grep "language_selection" /var/log/apache2/error.log
```

**URL rewriting not working:**
```bash
# Verify mod_rewrite is enabled
apache2ctl -M | grep rewrite

# Check .htaccess permissions
ls -la /var/www/html/.htaccess
```

**Translation not applied:**
- Verify the language code is supported (check `docs/pruned-languages.csv`)
- Ensure the AI backend is responding
- Check that translation instructions are being added to prompts

#### Debug Mode

Enable debug logging:

```apache
<FilesMatch "\.ai$">
    SetHandler muse-ai-handler
    SetEnv MUSE_LANG_DEBUG "1"
</FilesMatch>
```

This will log detailed language detection information to the Apache error log.

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

## URL Localization

The translation system automatically updates navigation links to maintain language context. When a user visits `/es/page.ai`, all navigation links are updated to include the `/es/` prefix.

### How It Works

**Automatic Link Updates:**
```html
<!-- Original navigation -->
<nav>
    <a href="/">Home</a> | 
    <a href="/features">Features</a> | 
    <a href="/documentation">Documentation</a>
</nav>

<!-- Automatically becomes (for Spanish) -->
<nav>
    <a href="/es/">Inicio</a> | 
    <a href="/es/features">Características</a> | 
    <a href="/es/documentation">Documentación</a>
</nav>
```

**Supported URL Formats:**
- **Clean URLs**: `/es/features`, `/de/documentation` (recommended)
- **With .ai extension**: `/es/features.ai`, `/fr/page.ai`
- **Directory URLs**: `/es/docs/`, `/fr/help/`
- **Root language**: `/es/`, `/fr/`, `/de/`

**Smart Asset Handling:**
- ✅ **Navigation links**: Updated with language prefix
- ✅ **Static assets**: CSS, JS, images remain unchanged
- ✅ **External links**: GitHub, social media links unchanged
- ✅ **Anchors**: `#section` links remain unchanged

### Benefits

- **Language Persistence**: Users stay in their selected language while navigating
- **SEO-Friendly**: Clean URLs like `/es/features` instead of query parameters
- **Professional UX**: Seamless multilingual website experience
- **Automatic**: No manual link management required

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
