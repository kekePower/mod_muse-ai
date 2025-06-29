#include "advanced_config.h"
#include <apr_strings.h>
#include <http_log.h>

/* Create advanced configuration structure - MINIMAL VERSION FOR DEBUGGING */
void *create_advanced_muse_ai_config(apr_pool_t *pool, server_rec *s)
{
    advanced_muse_ai_config *cfg;
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                "[mod_muse_ai] Creating MINIMAL configuration for debugging");
    
    cfg = apr_pcalloc(pool, sizeof(advanced_muse_ai_config));
    if (!cfg) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Failed to allocate configuration");
        return NULL;
    }
    
    /* Initialize ONLY the most basic fields */
    cfg->endpoint = "http://127.0.0.1:8080/v1";
    cfg->api_key = NULL;
    cfg->model = "test-model";
    cfg->timeout = 300;
    cfg->debug = 0;
    cfg->streaming = 1;
    cfg->max_tokens = 16384;  /* Default to 16k tokens */
    
    /* Set all other pointers to NULL to avoid crashes */
    cfg->reasoning_model_patterns = NULL;
    cfg->backend_endpoints = NULL;
    cfg->prompts_dir = NULL;  /* Will be set by directive handler */
    cfg->ratelimit_whitelist_ips = NULL;
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                "[mod_muse_ai] Minimal configuration created successfully");
    
    return cfg;
}

void *merge_advanced_muse_ai_config(apr_pool_t *p, void *base_conf, void *new_conf)
{
    advanced_muse_ai_config *base = base_conf;
    advanced_muse_ai_config *new = new_conf;
    advanced_muse_ai_config *merged = apr_pcalloc(p, sizeof(advanced_muse_ai_config));

    // MINIMAL merge function for debugging - only basic fields

    // Basic settings only
    merged->endpoint = new->endpoint ? new->endpoint : base->endpoint;
    merged->api_key = new->api_key ? new->api_key : base->api_key;
    merged->model = new->model ? new->model : base->model;
    merged->timeout = (new->timeout > 0 && new->timeout != 300) ? new->timeout : base->timeout;
    merged->max_tokens = (new->max_tokens != 16384) ? new->max_tokens : base->max_tokens;
    
    // DEBUG: Log timeout merge
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                "[mod_muse_ai] MERGE DEBUG: base->timeout=%d, new->timeout=%d, merged->timeout=%d", 
                base->timeout, new->timeout, merged->timeout);
    merged->debug = new->debug;
    merged->streaming = new->streaming;

    // Set all complex fields to NULL to avoid crashes, but preserve prompts_dir
    merged->reasoning_model_patterns = NULL;
    merged->backend_endpoints = NULL;
    merged->prompts_dir = new->prompts_dir ? new->prompts_dir : base->prompts_dir;
    merged->ratelimit_whitelist_ips = NULL;

    return merged;
}

/* Configuration directive handlers */

const char *set_pool_max_connections(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 1 || value > 100) {
        return "MuseAiPoolMaxConnections must be between 1 and 100";
    }
    
    config->pool_max_connections = value;
    return NULL;
}

const char *set_cache_enable(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        config->cache_enable = 1;
    } else if (strcasecmp(arg, "off") == 0 || strcasecmp(arg, "no") == 0 || strcasecmp(arg, "0") == 0) {
        config->cache_enable = 0;
    } else {
        return "MuseAiCacheEnable must be On or Off";
    }
    
    return NULL;
}

const char *set_cache_ttl(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 60 || value > 86400) {
        return "MuseAiCacheTTL must be between 60 and 86400 seconds";
    }
    
    config->cache_ttl_seconds = value;
    return NULL;
}

const char *set_ratelimit_enable(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        config->ratelimit_enable = 1;
    } else if (strcasecmp(arg, "off") == 0 || strcasecmp(arg, "no") == 0 || strcasecmp(arg, "0") == 0) {
        config->ratelimit_enable = 0;
    } else {
        return "MuseAiRateLimitEnable must be On or Off";
    }
    
    return NULL;
}

const char *set_ratelimit_rpm(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 1 || value > 10000) {
        return "MuseAiRateLimitRPM must be between 1 and 10000";
    }
    
    config->ratelimit_requests_per_minute = value;
    return NULL;
}

const char *set_metrics_enable(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        config->metrics_enable = 1;
    } else if (strcasecmp(arg, "off") == 0 || strcasecmp(arg, "no") == 0 || strcasecmp(arg, "0") == 0) {
        config->metrics_enable = 0;
    } else {
        return "MuseAiMetricsEnable must be On or Off";
    }
    
    return NULL;
}

const char *set_reasoning_model_pattern(cmd_parms *cmd, void *cfg, const char *pattern)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (!pattern || strlen(pattern) == 0) {
        return "MuseAiReasoningModelPattern requires a pattern string";
    }
    
    apr_table_set(config->reasoning_model_patterns, pattern, "1");
    return NULL;
}

const char *set_backend_endpoint(cmd_parms *cmd, void *cfg, const char *endpoint)
{
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)cfg;
    backend_endpoint_t *new_endpoint;

    if (!endpoint || strlen(endpoint) == 0) {
        return "MuseAiBackendEndpoint requires a non-empty URL string.";
    }

    // Initialize backend_endpoints array if it's NULL (minimal config)
    if (!config->backend_endpoints) {
        config->backend_endpoints = apr_array_make(cmd->pool, 5, sizeof(backend_endpoint_t));
        if (!config->backend_endpoints) {
            return "Failed to initialize backend endpoints array.";
        }
    }

    // If this is the first backend endpoint being added, set it as the primary endpoint.
    if (config->backend_endpoints->nelts == 0) {
        config->endpoint = apr_pstrdup(cmd->pool, endpoint);
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, cmd->server, 
                     "[mod_muse_ai] Primary backend endpoint set to: %s", config->endpoint);
    }

    // Add the new endpoint to the list for load balancing.
    new_endpoint = (backend_endpoint_t *)apr_array_push(config->backend_endpoints);
    if (!new_endpoint) {
        return "Failed to allocate memory for new backend endpoint.";
    }

    new_endpoint->url = apr_pstrdup(cmd->pool, endpoint);
    new_endpoint->api_key = NULL; // API key can be set per-endpoint later if needed
    new_endpoint->weight = 1;
    new_endpoint->active_connections = 0;
    new_endpoint->total_requests = 0;
    new_endpoint->failed_requests = 0;
    new_endpoint->last_health_check = 0;
    new_endpoint->healthy = 1; // Assume healthy until a check fails

    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, cmd->server, 
                 "[mod_muse_ai] Added backend endpoint for load balancing: %s", new_endpoint->url);

    return NULL;
    return NULL;
}

const char *set_load_balance_method(cmd_parms *cmd, void *cfg, const char *method)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (strcasecmp(method, "round_robin") != 0 && 
        strcasecmp(method, "least_connections") != 0 && 
        strcasecmp(method, "random") != 0) {
        return "MuseAiLoadBalanceMethod must be round_robin, least_connections, or random";
    }
    
    config->load_balance_method = apr_pstrdup(cmd->pool, method);
    return NULL;
}

const char *set_streaming_buffer_size(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 1024 || value > 65536) {
        return "MuseAiStreamingBufferSize must be between 1024 and 65536 bytes";
    }
    
    config->streaming_buffer_size = value;
    return NULL;
}

const char *set_security_max_request_size(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 1024 || value > 10485760) { /* 1KB to 10MB */
        return "MuseAiSecurityMaxRequestSize must be between 1024 and 10485760 bytes";
    }
    
    config->security_max_request_size = value;
    return NULL;
}

const char *set_muse_ai_prompts_dir(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, 
                "[mod_muse_ai] set_muse_ai_prompts_dir called with arg: %s", arg ? arg : "(NULL)");
    
    if (!arg || strlen(arg) == 0) {
        return "MuseAiPromptsDir requires a directory path";
    }
    
    config->prompts_dir = apr_pstrdup(cmd->pool, arg);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, 
                "[mod_muse_ai] prompts_dir set to: %s", config->prompts_dir);
    return NULL;
}

const char *set_muse_ai_prompts_minify(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        config->prompts_minify = 1;
    } else if (strcasecmp(arg, "off") == 0 || strcasecmp(arg, "no") == 0 || strcasecmp(arg, "0") == 0) {
        config->prompts_minify = 0;
    } else {
        return "MuseAiPromptsMinify must be On or Off";
    }
    
    return NULL;
}

/* Configuration validation */
const char *set_muse_ai_endpoint(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    cfg->endpoint = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_api_key(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    cfg->api_key = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_model(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    cfg->model = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_timeout(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int timeout_val = atoi(arg);
    cfg->timeout = timeout_val;
    
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, cmd->server,
                "[mod_muse_ai] DEBUG: Setting timeout to %d seconds from arg '%s'", 
                timeout_val, arg);
    
    return NULL;
}

const char *set_muse_ai_debug(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        cfg->debug = 1;
    } else {
        cfg->debug = 0;
    }
    return NULL;
}

const char *set_muse_ai_streaming(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    if (strcasecmp(arg, "on") == 0 || strcasecmp(arg, "yes") == 0 || strcasecmp(arg, "1") == 0) {
        cfg->streaming = 1;
    } else {
        cfg->streaming = 0;
    }
    return NULL;
}

const char *set_muse_ai_max_tokens(cmd_parms *cmd, void *dcfg, const char *arg)
{
    (void)dcfg;
    extern module muse_ai_module;
    advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 0 || value > 200000) {
        return "MuseAiMaxTokens must be between 0 and 200000 (0 = no limit)";
    }
    
    cfg->max_tokens = value;
    return NULL;
}

const command_rec muse_ai_advanced_cmds[] = {
    AP_INIT_TAKE1("MuseAiEndpoint", set_muse_ai_endpoint, NULL, RSRC_CONF, "The endpoint URL for the MuseWeb AI service"),
    AP_INIT_TAKE1("MuseAiApiKey", set_muse_ai_api_key, NULL, RSRC_CONF, "The API key for commercial AI providers"),
    AP_INIT_TAKE1("MuseAiModel", set_muse_ai_model, NULL, RSRC_CONF, "The AI model to use for generation"),
    AP_INIT_TAKE1("MuseAiTimeout", set_muse_ai_timeout, NULL, RSRC_CONF, "Timeout in seconds for the backend connection"),
    AP_INIT_TAKE1("MuseAiDebug", set_muse_ai_debug, NULL, RSRC_CONF, "Enable debug logging (On/Off)"),
    AP_INIT_TAKE1("MuseAiStreaming", set_muse_ai_streaming, NULL, RSRC_CONF, "Enable streaming responses (On/Off)"),
    AP_INIT_TAKE1("MuseAiPoolMaxConnections", set_pool_max_connections, NULL, RSRC_CONF, "Maximum number of connections in the pool"),
    AP_INIT_TAKE1("MuseAiCacheEnable", set_cache_enable, NULL, RSRC_CONF, "Enable response caching (On/Off)"),
    AP_INIT_TAKE1("MuseAiCacheTTL", set_cache_ttl, NULL, RSRC_CONF, "Cache time-to-live in seconds"),
    AP_INIT_TAKE1("MuseAiRateLimitEnable", set_ratelimit_enable, NULL, RSRC_CONF, "Enable rate limiting (On/Off)"),
    AP_INIT_TAKE1("MuseAiRateLimitRPM", set_ratelimit_rpm, NULL, RSRC_CONF, "Rate limit in requests per minute"),
    AP_INIT_TAKE1("MuseAiMetricsEnable", set_metrics_enable, NULL, RSRC_CONF, "Enable performance metrics (On/Off)"),
    AP_INIT_TAKE1("MuseAiReasoningModelPattern", set_reasoning_model_pattern, NULL, RSRC_CONF, "Regex pattern to identify a reasoning model"),
    AP_INIT_TAKE1("MuseAiBackendEndpoint", set_backend_endpoint, NULL, RSRC_CONF, "Define a backend endpoint for load balancing"),
    AP_INIT_TAKE1("MuseAiLoadBalanceMethod", set_load_balance_method, NULL, RSRC_CONF, "Load balancing method (round_robin, least_connections, random)"),
    AP_INIT_TAKE1("MuseAiStreamingBufferSize", set_streaming_buffer_size, NULL, RSRC_CONF, "Streaming buffer size in bytes"),
    AP_INIT_TAKE1("MuseAiSecurityMaxRequestSize", set_security_max_request_size, NULL, RSRC_CONF, "Maximum allowed request body size in bytes"),
    AP_INIT_TAKE1("MuseAiPromptsDir", set_muse_ai_prompts_dir, NULL, RSRC_CONF, "Directory for prompt files"),
    AP_INIT_TAKE1("MuseAiPromptsMinify", set_muse_ai_prompts_minify, NULL, RSRC_CONF, "Enable minified layout for prompts (On/Off)"),
    AP_INIT_TAKE1("MuseAiMaxTokens", set_muse_ai_max_tokens, NULL, RSRC_CONF, "Maximum number of tokens for AI response generation (0 = no limit)"),
    {NULL}
};

int validate_advanced_config(advanced_muse_ai_config *cfg, server_rec *s)
{
    if (!cfg) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Configuration is NULL");
        return -1;
    }
    
    /* Validate basic configuration */
    if (!cfg->endpoint) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Endpoint is not configured");
        return -1;
    }
    
    if (!cfg->model) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Model is not configured");
        return -1;
    }
    
    /* Validate advanced features */
    if (cfg->streaming_chunk_size > cfg->streaming_buffer_size) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Streaming chunk size cannot be larger than buffer size");
        return -1;
    }
    
    if (cfg->cache_enable && cfg->cache_ttl_seconds <= 0) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Cache TTL must be positive when caching is enabled");
        return -1;
    }
    
    if (cfg->ratelimit_enable && cfg->ratelimit_requests_per_minute <= 0) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Rate limit RPM must be positive when rate limiting is enabled");
        return -1;
    }
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                "[mod_muse_ai] Advanced configuration validation passed");
    
    return 0;
}
