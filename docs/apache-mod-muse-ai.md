# mod_muse-ai Development Plan

This document outlines a staged technical roadmap for building **`mod_muse-ai`**, an Apache HTTP Server module that brings MuseWeb-powered AI capabilities directly into the web-server layer.  We start with a minimal Proof-of-Concept (PoC) that you can compile and run today, then iterate toward a production-ready, feature-rich module.

---
## Table of Contents
1. Vision & Goals
2. Prerequisites & Tooling
3. Phase 1 – Proof-of-Concept (PoC)
4. Phase 2 – Core AI Integration
5. Phase 3 – Advanced Features & Hardening
6. Phase 4 – Packaging & Release
7. Long-Term Ideas

---
## 1  Vision & Goals
* **Inline AI Capabilities** – Expose MuseWeb-style AI endpoints (text generation, summarisation, etc.) as first-class Apache handlers and filters.
* **Drop-in Deployment** – Ship as a single shared object (`mod_muse-ai.so`) installable through `apxs`/`LoadModule`.
* **Performance & Security** – Leverage Apache’s threading/event model, connection pools, and shared memory to achieve low-latency inference with strict resource isolation.
* **Extensibility** – Provide hooks so downstream sites can add custom directives without recompilation.

## 2  Prerequisites & Tooling
| Requirement | Notes |
|-------------|-------|
| Apache HTTP Server 2.4+ | With dev headers (`apache2-dev`/`httpd-devel`) |
| GCC/Clang | C11 or later |
| `apxs` | Ships with Apache dev package (used under-the-hood by Meson install step) |
| Meson & Ninja | Modern build system (`meson ≥ 0.63`, `ninja`) |
| MuseWeb HTTP endpoint | Local or remote MuseWeb service (Go-based, v1.2.0+) |
| **Optional** – `check` | C unit-testing framework |

### Current MuseWeb Architecture (v1.2.0+)
- **Language**: Go with modular architecture
- **AI Providers**: OpenAI, Ollama, custom providers supported
- **Key Features**: Streaming responses, reasoning model support, language translation, error handling
- **Dependencies**: `github.com/ollama/ollama v0.9.1`, `github.com/sashabaranov/go-openai v1.40.3`
- **Performance**: Optimized streaming with sanitization, sub-3-second generation with fast models

---
## 3  Phase 1 – Proof-of-Concept (PoC)
Goal: deliver a minimal handler that returns "Hello from mod_muse-ai" so the user can compile, load, and hit it via a browser.

### 3.1  Skeleton Module
1. Create `mod_muse_ai.c` implementing:
   * `static int muse_ai_handler(request_rec *r)` – handles **POST** requests containing prompt data and returns AI-generated HTML.
   * Inside the handler, forward the prompt to **MuseWeb HTTP endpoint** using `apr_pool_t` + `apr_socket` or `libcurl`.
   * `static void muse_ai_register_hooks(apr_pool_t *p)` – registers `muse_ai_handler` for the `AI` content handler.
2. Provide configuration directives (Phase-1 scope):
   * `MuseAiEndpoint`    – MuseWeb server URL (default `http://127.0.0.1:8080/v1`).
   * `MuseAiTimeout`     – Request timeout in seconds (default 300 for large responses).
   * `MuseAiDebug`       – Enable debug logging (default false).
3. Declare the module struct `module AP_MODULE_DECLARE_DATA muse_ai_module`.

### 3.1.1 MuseWeb API Integration
**Current MuseWeb Endpoints:**
- **`GET /{prompt_name}`** – Generate HTML from prompt file (e.g., `/home`, `/portfolio`)
- **`GET /{prompt_name}?lang=LANG_CODE`** – Generate with language translation (e.g., `?lang=es_ES`)
- **`POST /`** – Direct prompt submission with request body

**Key Features to Leverage:**
- **Streaming responses** with real-time HTML generation
- **Language translation** via URL parameters
- **Error handling** with custom error pages
- **Sanitization** removes markdown artifacts from AI output
- **Reasoning model support** with thinking disabled for clean HTML

### 3.2  Build & Install  (Meson/Ninja)
Create `meson.build` with custom target that calls `apxs` to compile the shared object and install it into Apache's module directory automatically.

```bash
meson setup build --project-name mod_muse-ai
ninja -C build           # compiles mod_muse-ai.so via apxs
sudo ninja -C build install   # copies .so to modules/ and adds LoadModule if missing
```
The `-a` flag auto-adds a `LoadModule` line to `httpd.conf`.

### 3.3  Configuration Snippet
```apache
# httpd.conf
# Ensure Muse-AI handler is mapped
<Files "ai">
    SetHandler AI
</Files>
```
Or map by extension:
```apache
AddHandler AI .ai
```

### 3.4  Test
```bash
curl http://localhost/ai
# → "Hello from mod_muse-ai PoC!"
```
If you see the greeting, the PoC works.

### 3.5  Deliverables
* `mod_muse_ai.c`
* `meson.build`  (Meson/Ninja build definition)
* Quick-start README

---
## 4  Phase 2 – Core AI Integration
Expand the handler to forward requests to a MuseWeb AI backend with full streaming support.

### 4.1  Feature Scope
1. **Streaming Proxy** – Forward requests to MuseWeb Go service, stream HTML responses back in real-time.
2. **Config Directives** –
   * `MuseAiEndpoint` – URL of backend (default `http://127.0.0.1:8080/v1`).
   * `MuseAiTimeout` – per-request timeout in seconds (default 300 for large responses).
   * `MuseAiPromptsDir` – Path to prompts directory for prompt-based generation.
3. **Error Handling** – Leverage MuseWeb's built-in error pages (400, 404, 405, 500, etc.).
4. **Language Support** – Pass through `lang` parameter for multilingual content generation.
5. **Content Sanitization** – Benefit from MuseWeb's sanitization that removes markdown artifacts.

### 4.2  Implementation Checklist
- Use `mod_proxy` utilities or raw `apr_socket` for HTTP calls to MuseWeb backend.
- Handle streaming responses with chunked transfer encoding.
- Implement proper timeout handling (MuseWeb can generate large responses up to 371KB).
- Add directive parsing table and merge functions for configuration.
- Support URL parameter pass-through (especially `lang` for translation).
- Handle MuseWeb's custom error pages gracefully.

### 4.3  Milestone Exit Criteria
* Compile-time configurable MuseWeb endpoint.
* Streaming HTML generation working end-to-end.
* Language translation support via URL parameters.
* Proper error handling with MuseWeb's error pages.
* Performance: Sub-3-second generation for typical web pages.

---
## 4.4  MuseWeb Architecture Insights

### Key Technical Learnings from MuseWeb Development
1. **Streaming Sanitization Challenge** – AI models often output markdown code fences (```html) that must be cleaned in real-time during streaming. MuseWeb solved this with buffer-based processing that handles cross-chunk patterns.

2. **Reasoning Model Support** – Models like DeepSeek R1 require `thinking: false` parameter to prevent reasoning output from cluttering HTML generation.

3. **Language Translation** – URL parameter approach (`?lang=es_ES`) with instruction injection works reliably across all AI models. Critical: use strong emphasis (`**VERY IMPORTANT:**`) in prompts.

4. **Error Handling** – Custom error pages (400, 404, 405, 500) with modern design improve UX significantly over raw HTTP errors.

5. **Performance Optimization** – 
   - Pre-checks before regex operations save CPU cycles
   - Single-pass cleaning is more efficient than multiple passes
   - HTTP server write timeout must be extended (300s) for large AI responses

6. **Provider Compatibility** – Both OpenAI and Ollama backends supported with unified streaming interface.

---
## 5  Phase 3 – Advanced Features & Hardening
| Area | Features |
|------|----------|
| Performance | Connection pool, keep-alive, HTTP/2 push, shared memory cache of frequent prompts |
| Security | Rate limiting, request size limits, authentication tokens |
| Observability | Prometheus exporter, custom Apache log format, tracing via OpenTelemetry |
| Config UX | `MuseAiModel`, `MuseAiCacheTTL`, `MuseAiReasoningModels`, dynamic reload via `GracefulRestart` |
| Filters | Implement `output filter` for on-the-fly augmentation of HTML responses (e.g., summarise long articles) |
| Streaming | SSE / chunked transfer with real-time sanitization (leverage MuseWeb's buffer-based approach) |
| AI Features | Priority-based reasoning model detection, configurable model patterns, thinking parameter control |
| Multilingual | Advanced language translation with context preservation across generated URLs |
| Error Recovery | Graceful handling of streaming failures, partial content recovery, timeout management |

---
## 6  Phase 4 – Packaging & Release
1. **CI** – GitHub Actions building on Ubuntu, CentOS, macOS.
2. **RPM/DEB Packages** – generate versioned packages.
3. **Semantic Versioning** – `v0.x` until first stable.
4. **Documentation** – Official docs site, examples, screencasts.
5. **Community** – CLA, contribution guide, issue templates.

---
## 7  Long-Term Ideas
* Native LLM inference via ONNX Runtime with GPU offload.
* Lua/Wasmtime hooks so users can script inference chains.
* Cluster-wide caching via memcached/redis.

---
### Timeline (T-shirt Sizing)
| Phase | Est. Duration |
|-------|--------------|
| 1 – PoC | 2 days |
| 2 – Core AI | 1-2 weeks |
| 3 – Advanced | 3-4 weeks |
| 4 – Packaging | 1 week |

---
## Appendix A  PoC Source (code snippet)
> NOTE: Full source lives in the repo. Shown here for reference.
```c
#include "httpd.h"
#include "http_protocol.h"
#include "http_config.h"
#include "ap_config.h"

static int muse_ai_handler(request_rec *r) {
    if (!r->handler || strcmp(r->handler, "AI")) {
        return DECLINED;
    }
    ap_set_content_type(r, "text/plain;charset=UTF-8");
    ap_rputs("Hello from mod_muse-ai PoC!\n", r);
    return OK;
}
static void muse_ai_register_hooks(apr_pool_t *p) {
    ap_hook_handler(muse_ai_handler, NULL, NULL, APR_HOOK_MIDDLE);
}
module AP_MODULE_DECLARE_DATA muse_ai_module = {
    STANDARD20_MODULE_STUFF,
    NULL, NULL, NULL, NULL, NULL, muse_ai_register_hooks
};
```

---
## Current MuseWeb Status (v1.2.0+)

### Production Readiness
- **Stable Version**: v1.2.0-dev10+ with comprehensive error handling
- **Deployment**: Successfully deployed and tested in production environments
- **Performance**: Sub-3-second generation with optimized models (Gemini 2.5 Flash Lite)
- **Reliability**: Robust streaming with sanitization, panic recovery, graceful error handling

### Key Dependencies (Stable)
```go
// go.mod
require (
    github.com/ollama/ollama v0.9.1  // Downgraded for stability
    github.com/sashabaranov/go-openai v1.40.3
    gopkg.in/yaml.v3 v3.0.1
)
```

### Configuration Example
```yaml
# config.yaml
api_key: "your-api-key"
model: "gpt-4o-mini"
base_url: "https://api.openai.com/v1"
max_tokens: 4000
debug: false
reasoning_models:
  - "deepseek-r1-distill"  # Most specific first
  - "r1-distill"
  - "sonar-reasoning-pro"
  - "sonar-reasoning"
  - "qwen3"
  - "deepseek"  # General fallback
  - "qwen"     # General fallback
```

### Ready for Apache Module Integration
MuseWeb is production-ready and can serve as a reliable backend for the Apache module. All major streaming, sanitization, and error handling issues have been resolved.

---
You can now proceed to implement **Phase 1** and iterate!
