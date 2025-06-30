# Advanced Optimizations & Future Roadmap

This document outlines the strategic roadmap for enhancing `mod_muse-ai` based on expert feedback from the Apache community. The goal is to evolve the module into a secure, high-performance, and enterprise-ready solution. The plan is broken down into phases, prioritizing critical security and stability improvements first.

---

## Phase 1: Core Security & Stability Hardening (High Priority)

These changes address fundamental security risks and are crucial for any safe deployment.

### 1. Secure Secrets Management

*   **The Problem**: The current `MuseAiApiKey` and `MuseAiEndpoint` directives store secrets directly in the Apache configuration files (`.conf`), which is a security risk.
*   **The Suggestion**: Store secrets in a separate, secured environment file and load them into the Apache environment using the `PassEnv` directive. This prevents secrets from being committed to version control or being exposed in world-readable configuration files.
*   **The Plan**:
    1.  **Implement `PassEnv` Support**: Modify the configuration logic to read the API key and endpoint URL from environment variables first (e.g., `MUSE_AI_API_KEY`, `MUSE_AI_ENDPOINT`). The module will use `apr_env_get()` to retrieve these values at startup.
    2.  **Fallback Mechanism**: The module will fall back to using the existing `.conf` directives if the environment variables are not set. This ensures full backward compatibility for existing installations.
    3.  **Update Documentation**: The `HOWTO.md` guide will be updated to strongly recommend using environment variables via `PassEnv` as the primary, most secure method for handling secrets.

### 2. Whitelist-Based Access Control

*   **The Problem**: The module can be enabled globally, potentially allowing `.ai` files to be processed from any directory. This could be abused if a user can upload files to an unexpected location (e.g., `/tmp` or `/uploads`).
*   **The Suggestion**: Use `<Directory>` blocks to explicitly whitelist which directories are permitted to execute `.ai` files, reducing the attack surface for Local File Inclusion (LFI) or other exploits.
*   **The Plan**:
    1.  **Create a Per-Directory Directive**: Introduce a new directive, `MuseAiEnable On|Off`, designed to be used within `<Directory>` or `<Location>` blocks.
    2.  **Enforce in Handler**: The request handlers will be updated to check if this directive is set to `On` for the requested file's directory. If the directive is not explicitly enabled for that path, the handler will decline the request.
    3.  **Secure by Default**: This change allows administrators to precisely control which parts of their site can generate content with AI. The documentation will be updated to reflect this as a best practice for all production setups.

---

## Phase 2: Performance & Cost Optimization (Medium Priority)

This phase focuses on the caching suggestion, which has a massive impact on performance, user experience, and API costs.

### 3. Smart Caching with `mod_cache_socache`

*   **The Goal**: To balance a "live feel" with performance and cost savings. We want to serve fresh content for normal traffic but use caching to handle traffic spikes and reduce redundant API calls.
*   **The Plan**:
    1.  **Short-Lived Caching by Default**: The module will use a short default Time-To-Live (TTL), such as 5 minutes. This ensures that content is regularly refreshed, keeping the site feeling dynamic, while still providing a powerful shield against short-term, repeated requests.
    2.  **Configurable TTL**: Introduce a new `MuseAiCacheTTL` directive. This will allow administrators to override the default TTL on a per-directory or per-location basis, enabling longer cache times for static content (e.g., tutorials) and shorter times for more dynamic pages.
    3.  **Standard `mod_cache` Integration**: The implementation will follow the original plan of integrating with Apache's `mod_cache_socache` and using a SHA1 hash of the prompt and model settings as the cache key. This ensures a robust and standard-compliant implementation.
    4.  **Model-Aware Caching**: Ensure cache keys include the selected model to prevent cross-model contamination. Add cache bypass query parameter (`?cache-bypass=1`) for development/testing.

---

## Phase 2.5: Dynamic Configuration (Zero-Downtime Updates)

This phase focuses on enabling dynamic configuration changes without requiring Apache restarts, improving operational flexibility.

### 4. Dynamic Model Switching

*   **The Problem**: Currently, changing the AI model requires modifying Apache configuration files and restarting the server, causing downtime and interrupting user sessions.
*   **The Suggestion**: Use a sidecar file for model configuration that can be hot-reloaded without restarting Apache.
*   **The Plan**:
    1.  **Sidecar Configuration File**: Create a JSON configuration file (`models.json`) to define available models, including model names, endpoints, and any model-specific parameters.
    2.  **Hot Reload Mechanism**: Implement a worker thread that monitors the config file's mtime. When the timestamp changes, read the updated configuration and use atomic pointer swap to update the model list without locks on the request path.
    3.  **Request-Time Model Selection**: Add support for `MuseAIModel` environment variable via `SetEnvIf` or `RequestHeader`. Validate requested model against whitelist from configuration and fall back to default model if requested model is invalid.
    4.  **Security Considerations**: Implement strict validation against the whitelist to prevent typo attacks. Add logging for model selection attempts and failures.
    5.  **Documentation**: Update HOWTO.md with examples of model configuration and document Apache directives for model selection.

---

## Phase 3: Advanced Architecture (Long-Term Roadmap)

These are more complex features for large-scale, multi-tenant, or public-facing deployments.

### 5. Per-IP Rate Limiting for Abuse Prevention

*   **The Goal**: To provide a robust, global defense against bad actors and automated scripts that make excessive requests, protecting API budgets and ensuring site availability.
*   **The Plan**: Rate limiting is the primary tool for abuse prevention. The implementation will use a shared memory cache (`socache`) to track the request rate for each client IP address. If an IP exceeds a configurable threshold (e.g., 60 requests per minute), the module will immediately return an HTTP 429 (Too Many Requests) or 503 (Service Unavailable) status, effectively blocking the abusive client without impacting other users.

### 6. Fully Asynchronous Processing

*   **The Problem**: While the module streams the AI *response*, the initial API call to the AI backend is a blocking operation that ties up an Apache worker thread/process until the first bytes are received.
*   **The Suggestion**: Use a dedicated worker thread for the outbound API call and immediately return an HTTP 202 status to free the main worker, then use Server-Sent Events (SSE) to stream the final result.
*   **The Plan**: This is a significant architectural change. The first step is to conduct performance testing under heavy load to determine if this is a real-world bottleneck with modern Apache MPMs like `event`. If it proves to be a limitation, we will investigate leveraging Apache's native asynchronous capabilities before considering a custom in-module thread pool, which adds significant complexity.
