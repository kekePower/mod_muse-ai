# Simple Guide to Using mod_muse-ai

This guide provides straightforward instructions for setting up and using the mod_muse-ai Apache module, with special focus on API keys, endpoints, and the new dynamic model configuration system.

## Basic Setup (Simplest Approach)

If you want to get back to a working setup quickly, here's the simplest approach:

### Step 1: Create a minimal Apache configuration

Create a file called `muse-ai.conf` in your Apache configuration directory:

```apache
# Load the module
LoadModule muse_ai_module modules/mod_muse_ai.so

# Basic configuration - for local Ollama
MuseAiEndpoint "http://127.0.0.1:11434/v1"
MuseAiModel "llama3.2:latest"
MuseAiTimeout 300
MuseAiDebug On

# For OpenAI (uncomment these if using OpenAI instead of Ollama)
# MuseAiEndpoint "https://api.openai.com/v1"
# MuseAiModel "gpt-4.1-nano"
# MuseAiApiKey "your-api-key-here"

# Enable the handler at /ai endpoint
<Location "/ai">
    MuseAiEnable On
    SetHandler muse-ai-handler
    Require all granted
</Location>

# Enable .ai file handling
<FilesMatch "\.ai$">
    MuseAiEnable On
    SetHandler ai-file-handler
    Require all granted
</FilesMatch>
```

### Step 2: Include this configuration in your main Apache config

Add this line to your main Apache config file:

```apache
Include /path/to/muse-ai.conf
```

### Step 3: Restart Apache

```bash
sudo systemctl restart apache2  # Ubuntu/Debian
# or
sudo systemctl restart httpd    # CentOS/RHEL/Fedora
```

### Step 4: Test the module

```bash
curl "http://localhost/ai?prompt=Hello"
```

## Using API Keys

There are two ways to configure API keys:

### Method 1: Direct Configuration (Simpler)

In your Apache configuration:

```apache
# Set the API key directly (simplest approach)
MuseAiEndpoint "https://api.openai.com/v1"
MuseAiModel "gpt-4.1-nano"
MuseAiApiKey "your-openai-key-here"
```

### Method 2: Environment Variables (More Secure)

#### Setting Environment Variables for Apache

##### Option A: Apache Environment Configuration (Recommended for Debian/Ubuntu)

```bash
# Create a file in /etc/apache2/envvars.d/
sudo mkdir -p /etc/apache2/envvars.d/
sudo nano /etc/apache2/envvars.d/muse-ai-keys.conf

# Add this line to the file
export OPENAI_API_KEY="your-openai-key-here"

# Make sure Apache can read it
sudo chmod 640 /etc/apache2/envvars.d/muse-ai-keys.conf
sudo chown root:www-data /etc/apache2/envvars.d/muse-ai-keys.conf

# Restart Apache
sudo systemctl restart apache2
```

##### Option B: Using systemd Environment File (Works on all systemd-based systems)

```bash
# Create or edit the environment file
sudo mkdir -p /etc/systemd/system/apache2.service.d/
sudo nano /etc/systemd/system/apache2.service.d/env.conf
# or for CentOS/RHEL:
# sudo mkdir -p /etc/systemd/system/httpd.service.d/
# sudo nano /etc/systemd/system/httpd.service.d/env.conf

# Add this content
[Service]
Environment="OPENAI_API_KEY=your-openai-key-here"

# Reload systemd and restart Apache
sudo systemctl daemon-reload
sudo systemctl restart apache2  # or httpd for CentOS/RHEL
```

##### Option C: In Apache Configuration (Less Secure)

```apache
# In your Apache configuration file
SetEnv OPENAI_API_KEY "your-openai-key-here"
```

Then in your Apache configuration, reference the environment variable:

```apache
MuseAiEndpoint "https://api.openai.com/v1"
MuseAiModel "gpt-4.1-nano"
MuseAiApiKey "${OPENAI_API_KEY}"
```

## Using the MuseAiEnable Directive

The `MuseAiEnable` directive allows you to selectively enable or disable the mod_muse-ai functionality in different contexts:

### In Location Context

```apache
<Location "/ai">
    MuseAiEnable On  # Enable the mod_muse-ai module for this location
    SetHandler muse-ai-handler
    Require all granted
</Location>

<Location "/no-ai">
    MuseAiEnable Off  # Disable the mod_muse-ai module for this location
    Require all granted
</Location>
```

### In Directory Context

```apache
<Directory "/var/www/html/ai-enabled">
    MuseAiEnable On
    SetHandler muse-ai-handler
    Require all granted
</Directory>

<Directory "/var/www/html/ai-disabled">
    MuseAiEnable Off
    Require all granted
</Directory>
```

### In FilesMatch Context

```apache
<FilesMatch "\.ai$">
    MuseAiEnable On
    SetHandler ai-file-handler
    Require all granted
</FilesMatch>
```

### In .htaccess Files

```apache
# In a .htaccess file
MuseAiEnable On
SetHandler muse-ai-handler
```

## Different AI Provider Endpoints

Here are the correct endpoints for common AI providers:

### Ollama (Local)
```apache
MuseAiEndpoint "http://127.0.0.1:11434/v1"
MuseAiModel "llama3.2:latest"  # or whatever model you have installed
MuseAiApiKey ""  # No API key needed for Ollama
```

### OpenAI
```apache
MuseAiEndpoint "https://api.openai.com/v1"
MuseAiModel "gpt-4.1-nano"  # or gpt-4, gpt-3.5-turbo, etc.
MuseAiApiKey "your-openai-key-here"
```

### Google Gemini
```apache
MuseAiEndpoint "https://generativelanguage.googleapis.com/v1beta"
MuseAiModel "gemini-2.5-flash-lite"  # or other Gemini model
MuseAiApiKey "your-gemini-key-here"
```

### Anthropic Claude
```apache
MuseAiEndpoint "https://api.anthropic.com/v1"
MuseAiModel "claude-sonnet-4-20250514"  # or other Claude model
MuseAiApiKey "your-anthropic-key-here"
```

## The New Dynamic Model Configuration

If you want to use multiple models and switch between them without restarting Apache, you can use the new dynamic model configuration:

### Step 1: Create a models.json file

Create this file in your Apache configuration directory:

```json
{
  "models": [
    {
      "name": "default",
      "endpoint": "http://127.0.0.1:11434/v1",
      "api_key": "",
      "model": "llama3.2:latest"
    },
    {
      "name": "openai",
      "endpoint": "https://api.openai.com/v1",
      "api_key": "${OPENAI_API_KEY}",
      "model": "gpt-4.1-nano"
    }
  ],
  "default_model": "default"
}
```

### Step 2: Set environment variables for API keys

Use one of the methods described in the "Setting Environment Variables for Apache" section above.

### Step 3: Configure different endpoints to use different models

```apache
# Default model endpoint (uses "default" from models.json)
<Location "/ai/default">
    MuseAiEnable On
    SetHandler muse-ai-handler
    Require all granted
</Location>

# OpenAI model endpoint
<Location "/ai/openai">
    MuseAiEnable On
    SetHandler muse-ai-handler
    SetEnv MuseAIModel "openai"
    Require all granted
</Location>

# You can also use it in Directory context
<Directory "/var/www/html/ai-premium">
    MuseAiEnable On
    SetHandler muse-ai-handler
    SetEnv MuseAIModel "openai"
    Require all granted
</Directory>
```

### Step 4: Hot-reloading configuration

The module automatically monitors the `models.json` file for changes and reloads it when modified, without requiring an Apache restart. This allows you to:

- Add new models
- Update existing model configurations
- Change the default model
- Rotate API keys

## Troubleshooting

If you're having issues:

1. **Check Apache error logs**:
   ```bash
   tail -f /var/log/apache2/error.log  # Ubuntu/Debian
   # or
   tail -f /var/log/httpd/error_log    # CentOS/RHEL/Fedora
   ```

2. **Enable debug logging**:
   ```apache
   MuseAiDebug On
   ```

3. **Verify module is loaded**:
   ```bash
   apachectl -M | grep muse_ai
   ```

4. **Test with a simple curl command**:
   ```bash
   curl "http://localhost/ai?prompt=Hello"
   ```

5. **Verify API key is working** (for commercial providers):
   ```bash
   # Replace with your actual API key and endpoint
   curl -H "Authorization: Bearer your-api-key" https://api.openai.com/v1/models
   ```

6. **Check if models.json is being read**:
   Look for messages in the Apache error log about loading models.json.

7. **Test environment variable resolution**:
   Add debug logging and check if environment variables are being resolved correctly.

## Important Security Notes

1. The environment variables must be accessible to the Apache process, which typically runs as `www-data` (Debian/Ubuntu) or `apache` (CentOS/RHEL).

2. Make sure any files containing API keys have restricted permissions (640 or 600) and are owned by root with the Apache group.

3. Using systemd environment files or Apache environment configuration files are the most secure options as they keep the API keys out of the Apache configuration files.

4. Never set these environment variables in your personal `.bashrc` or `.profile` as they won't be available to the Apache process.

5. Regularly rotate your API keys as a security best practice.
