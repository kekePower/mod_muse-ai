# mod_muse-ai Project Structure

This document describes the organized project structure for the mod_muse-ai Apache module.

## 📁 Directory Structure

```
mod_muse-ai/
├── README.md                    # Main project documentation
├── LICENSE                      # Apache 2.0 license
├── meson.build                  # Build configuration
├── install_template.sh.in       # Install script template
├── PROJECT_STRUCTURE.md         # This file
│
├── src/                         # Source code
│   ├── mod_muse_ai.h           # Main header file
│   ├── mod_muse_ai.c           # Original monolithic module (stable)
│   ├── mod_muse_ai_main.c      # Modular main module
│   ├── mod_muse_ai_test.c      # Test module (working)
│   ├── streaming.c             # Streaming implementation
│   ├── sanitize.c              # Content sanitization
│   ├── http_client.c           # HTTP client with SSE support
│   └── utils.c                 # Utility functions
│
├── scripts/                     # Build and utility scripts
│   ├── build_module.sh         # Original build script
│   └── build_modular.sh        # Modular build script
│
├── docs/                        # Documentation
│   ├── apache-mod-muse-ai.md   # Technical specification
│   └── muse-ai-progress.md     # Development progress log
│
├── build/                       # Build artifacts (gitignored)
│   └── mod_muse_ai.so          # Compiled module
│
└── extra/                       # External resources
    └── museweb/                # MuseWeb source code reference
```

## 🔧 Build System

### Current Stable Version
- **File**: `src/mod_muse_ai.c` (monolithic)
- **Size**: ~64KB
- **Features**: API key authentication, HTTP client, commercial provider support
- **Status**: ✅ Production ready

### Modular Version (In Development)
- **Files**: `src/mod_muse_ai_main.c` + supporting modules
- **Size**: ~115KB
- **Features**: Streaming support, modular architecture
- **Status**: ⚠️ Debugging in progress (segfault issues)

### Test Version
- **File**: `src/mod_muse_ai_test.c`
- **Size**: ~45KB
- **Features**: Basic configuration, no complex features
- **Status**: ✅ Working (for testing modular approach)

## 🚀 Build Commands

```bash
# Build current version
ninja -C build

# Install module
sudo cp build/mod_muse_ai.so /usr/lib64/httpd/modules/
sudo systemctl restart httpd

# Test module
curl http://localhost/ai
```

## 📋 Configuration Directives

All versions support these Apache directives:

- `MuseAiEndpoint` - Backend URL (default: http://127.0.0.1:11434/v1)
- `MuseAiTimeout` - Request timeout in seconds (default: 300)
- `MuseAiDebug` - Enable debug logging (default: Off)
- `MuseAiModel` - AI model to use (default: default)
- `MuseAiApiKey` - API key for commercial providers (optional)
- `MuseAiStreaming` - Enable streaming responses (default: On)

## 🎯 Development Status

### ✅ Completed (Phase 1 & 2)
- Apache module compilation and loading
- Configuration directive system
- HTTP client with socket programming
- API key authentication for commercial providers
- OpenAI API compatibility
- Multi-language support
- Error handling and debug logging

### 🚧 In Progress (Phase 3)
- Modular architecture implementation
- Real-time streaming responses
- Content sanitization
- SSE (Server-Sent Events) processing

### 📋 Planned (Phase 3 Advanced)
- Connection pooling and keep-alive
- Rate limiting and security hardening
- Prometheus metrics and observability
- Performance optimization and caching

## 🔒 Security Features

- API keys never exposed in web interface or logs
- Secure APR memory pool management
- Industry-standard Bearer token authentication
- Production-ready error handling

## 🧪 Testing

The module has been tested with:
- Local AI servers (Ollama)
- Commercial providers (Google Gemini, OpenAI compatible)
- Various prompt types and languages
- Sub-3-second response times
- No memory leaks or segfaults (stable version)

## 📚 Documentation

- **Technical Spec**: `docs/apache-mod-muse-ai.md`
- **Progress Log**: `docs/muse-ai-progress.md`
- **MuseWeb Reference**: `extra/museweb/` (streaming implementation guide)
