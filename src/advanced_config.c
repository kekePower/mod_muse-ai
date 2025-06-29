#include "advanced_config.h"
#include <apr_strings.h>
#include <http_log.h>

/* Create advanced configuration structure */
advanced_muse_ai_config *create_advanced_muse_ai_config(apr_pool_t *pool, server_rec *s)
{
    advanced_muse_ai_config *cfg;
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                "[mod_muse_ai] Creating advanced configuration");
    
    cfg = apr_pcalloc(pool, sizeof(advanced_muse_ai_config));
    if (!cfg) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Failed to allocate advanced configuration");
        return NULL;
    }
    
    /* Initialize basic configuration */
    cfg->endpoint = "http://127.0.0.1:8080/v1";
    cfg->api_key = NULL;
    cfg->model = "google/gemini-2.5-flash-lite-preview-06-17";
    cfg->timeout = 300;
    cfg->debug = 0;
    cfg->streaming = 1;
    
    /* Initialize Phase 3 advanced features */
    
    /* Connection Pooling */
    cfg->pool_max_connections = 10;
    cfg->pool_connection_timeout = 300;
    cfg->pool_idle_timeout = 60;
    cfg->pool_enable_keepalive = 1;
    
    /* Caching */
    cfg->cache_enable = 0;
    cfg->cache_ttl_seconds = 300;
    cfg->cache_max_entries = 1000;
    cfg->cache_key_prefix = "muse_ai_";
    
    /* Rate Limiting */
    cfg->ratelimit_enable = 0;
    cfg->ratelimit_requests_per_minute = 60;
    cfg->ratelimit_burst_size = 10;
    cfg->ratelimit_whitelist_ips = NULL;
    
    /* Performance Monitoring */
    cfg->metrics_enable = 1;
    cfg->metrics_endpoint = "/metrics";
    cfg->metrics_include_request_details = 0;
    
    /* Reasoning Models Support */
    cfg->reasoning_model_patterns = apr_table_make(pool, 10);
    cfg->reasoning_disable_thinking = 1;
    
    /* Advanced Streaming */
    cfg->streaming_buffer_size = 8192;
    cfg->streaming_chunk_size = 1024;
    cfg->streaming_sanitization_enable = 1;
    
    /* Security */
    cfg->security_validate_content_type = 1;
    cfg->security_max_request_size = 1048576; /* 1MB */
    cfg->security_allowed_origins = "*";
    
    /* Load Balancing */
    cfg->backend_endpoints = apr_array_make(pool, 5, sizeof(backend_endpoint_t));
    cfg->load_balance_method = "round_robin";
    cfg->health_check_interval = 30;
    
    /* Timeouts and Retries */
    cfg->connect_timeout = 10;
    cfg->read_timeout = 300;
    cfg->write_timeout = 30;
    cfg->max_retries = 3;
    cfg->retry_delay_ms = 1000;
    
    /* Add default reasoning model patterns */
    apr_table_set(cfg->reasoning_model_patterns, "deepseek-r1", "1");
    apr_table_set(cfg->reasoning_model_patterns, "deepseek", "1");
    apr_table_set(cfg->reasoning_model_patterns, "gemini-2.5-flash", "1");
    apr_table_set(cfg->reasoning_model_patterns, "qwen", "1");
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, 
                "[mod_muse_ai] Advanced configuration created successfully");
    
    return cfg;
}

/* Configuration directive handlers */

const char *set_pool_max_connections(cmd_parms *cmd, void *cfg, const char *arg)
{
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
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    backend_endpoint_t *backend;
    
    if (!endpoint || strlen(endpoint) == 0) {
        return "MuseAiBackendEndpoint requires a URL";
    }
    
    backend = (backend_endpoint_t *)apr_array_push(config->backend_endpoints);
    backend->url = apr_pstrdup(cmd->pool, endpoint);
    backend->api_key = NULL;
    backend->weight = 1;
    backend->active_connections = 0;
    backend->total_requests = 0;
    backend->failed_requests = 0;
    backend->last_health_check = 0;
    backend->healthy = 1;
    
    return NULL;
}

const char *set_load_balance_method(cmd_parms *cmd, void *cfg, const char *method)
{
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
    extern module muse_ai_module;
    advanced_muse_ai_config *config = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    int value = atoi(arg);
    
    if (value < 1024 || value > 10485760) { /* 1KB to 10MB */
        return "MuseAiSecurityMaxRequestSize must be between 1024 and 10485760 bytes";
    }
    
    config->security_max_request_size = value;
    return NULL;
}

/* Configuration validation */
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
