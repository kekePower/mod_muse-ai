#include "mod_muse_ai.h"
#include "advanced_config.h"
#include "phase3_integration.h"

/* Module configuration directives */
static const command_rec muse_ai_directives[] = {
    /* Phase 2 Core Directives */
    AP_INIT_TAKE1("MuseAiEndpoint", set_muse_ai_endpoint, NULL, RSRC_CONF, "The endpoint URL for the MuseWeb AI service"),
    AP_INIT_TAKE1("MuseAiTimeout", set_muse_ai_timeout, NULL, RSRC_CONF, "The timeout in seconds for the AI service request"),
    AP_INIT_FLAG("MuseAiDebug", set_muse_ai_debug, NULL, RSRC_CONF, "Enable or disable debug logging (On/Off)"),
    AP_INIT_TAKE1("MuseAiModel", set_muse_ai_model, NULL, RSRC_CONF, "The AI model to use for the request"),
    AP_INIT_TAKE1("MuseAiApiKey", set_muse_ai_api_key, NULL, RSRC_CONF, "The API key for commercial AI providers"),
    AP_INIT_FLAG("MuseAiStreaming", set_muse_ai_streaming, NULL, RSRC_CONF, "Enable or disable streaming (On/Off)"),
    
    /* Phase 3 Advanced Directives */
    AP_INIT_TAKE1("MuseAiPoolMaxConnections", set_pool_max_connections, NULL, RSRC_CONF, "Maximum connections in connection pool"),
    AP_INIT_TAKE1("MuseAiCacheEnable", set_cache_enable, NULL, RSRC_CONF, "Enable response caching (On/Off)"),
    AP_INIT_TAKE1("MuseAiCacheTTL", set_cache_ttl, NULL, RSRC_CONF, "Cache TTL in seconds"),
    AP_INIT_TAKE1("MuseAiRateLimitEnable", set_ratelimit_enable, NULL, RSRC_CONF, "Enable rate limiting (On/Off)"),
    AP_INIT_TAKE1("MuseAiRateLimitRPM", set_ratelimit_rpm, NULL, RSRC_CONF, "Rate limit requests per minute"),
    AP_INIT_TAKE1("MuseAiMetricsEnable", set_metrics_enable, NULL, RSRC_CONF, "Enable metrics collection (On/Off)"),
    AP_INIT_TAKE1("MuseAiReasoningModelPattern", set_reasoning_model_pattern, NULL, RSRC_CONF, "Pattern to detect reasoning models"),
    AP_INIT_TAKE1("MuseAiBackendEndpoint", set_backend_endpoint, NULL, RSRC_CONF, "Additional backend endpoint"),
    AP_INIT_TAKE1("MuseAiLoadBalanceMethod", set_load_balance_method, NULL, RSRC_CONF, "Load balancing method (round_robin, random, least_connections)"),
    AP_INIT_TAKE1("MuseAiStreamingBufferSize", set_streaming_buffer_size, NULL, RSRC_CONF, "Streaming buffer size in bytes"),
    AP_INIT_TAKE1("MuseAiSecurityMaxRequestSize", set_security_max_request_size, NULL, RSRC_CONF, "Maximum request size in bytes"),

    /* Prompts Directory Directives */
    AP_INIT_TAKE1("MuseAiPromptsDir", set_muse_ai_prompts_dir, NULL, RSRC_CONF|ACCESS_CONF, "Directory for AI prompt files"),
    AP_INIT_TAKE1("MuseAiPromptsMinify", set_muse_ai_prompts_minify, NULL, RSRC_CONF|ACCESS_CONF, "Use minified layout prompts (On/Off)"),

    {NULL}
};

/* Configuration functions */
void *create_muse_ai_config(apr_pool_t *pool, server_rec *s)
{
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, "[mod_muse_ai] Entering create_muse_ai_config");
    if (!pool) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "[mod_muse_ai] FATAL: pool is NULL in create_muse_ai_config");
        return NULL;
    }
    
    /* Phase 3: Create advanced config structure */
    advanced_muse_ai_config *cfg = create_advanced_muse_ai_config(pool, s);
    if (!cfg) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "[mod_muse_ai] FATAL: Failed to create advanced config");
        return NULL;
    }
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, s, "[mod_muse_ai] Advanced config created successfully");
    return cfg;
}

const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->endpoint = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->timeout = atoi(arg);
    return NULL;
}

const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->debug = flag;
    return NULL;
}

const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->model = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    if (arg && strlen(arg) > 0) {
        conf->api_key = apr_pstrdup(cmd->pool, arg);
    } else {
        conf->api_key = NULL;
    }
    return NULL;
}

const char *set_muse_ai_streaming(cmd_parms *cmd, void *cfg, int flag)
{
    (void)cfg;
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->streaming = flag;
    return NULL;
}

/* Main request handler - now a router for Phase 3 endpoints */
static int muse_ai_handler(request_rec *r)
{
    /*
     * This function acts as a router. The Apache configuration maps different
     * Locations (/ai, /metrics, /health) to different handler names using
     * the 'SetHandler' directive.
     *
     * This single hook function (muse_ai_handler) is registered with Apache,
     * and it gets called for any of those locations. We then inspect the
     * request URI (r->uri) to delegate to the correct implementation.
     */

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                  "[mod_muse_ai] Router received request for URI: %s", r->uri);

    if (strcmp(r->uri, "/ai") == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Dispatching to enhanced_muse_ai_handler for /ai");
        return enhanced_muse_ai_handler(r);
    }
    if (strcmp(r->uri, "/metrics") == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Dispatching to metrics_handler for /metrics");
        return metrics_handler(r);
    }
    if (strcmp(r->uri, "/health") == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Dispatching to health_check_handler for /health");
        return health_check_handler(r);
    }

    return DECLINED;
}

/* Post-config hook to initialize Phase 3 features */
int muse_ai_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    (void)plog;
    (void)ptemp;
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, "[mod_muse_ai] Post-config hook running, initializing Phase 3 features.");
    
    // We need to walk the server list since this hook can be called for different vhosts
    server_rec *current_server;
    for (current_server = s; current_server; current_server = current_server->next) {
        advanced_muse_ai_config *cfg = (advanced_muse_ai_config *)ap_get_module_config(current_server->module_config, &muse_ai_module);
        if (cfg && !cfg->phase3_initialized) { // Initialize only once
            init_phase3_features(pconf, current_server, cfg);
        }
    }
    
    return OK;
}

/* Module registration */
static void register_hooks(apr_pool_t *pool)
{
    (void)pool;
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, "[mod_muse_ai] Registering hooks - module is loading");
    
    /* Register the post-config hook to initialize Phase 3 features */
    ap_hook_post_config(muse_ai_post_config, NULL, NULL, APR_HOOK_MIDDLE);
    
    /* Register the content handler for AI requests, metrics, and health checks */
    ap_hook_handler(muse_ai_handler, NULL, NULL, APR_HOOK_FIRST);
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, "[mod_muse_ai] All hooks registered successfully");
}

module AP_MODULE_DECLARE_DATA muse_ai_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                       /* create per-directory config structure */
    NULL,                       /* merge per-directory config structures */
    create_muse_ai_config,      /* create per-server config structure */
    NULL,                       /* merge per-server config structures */
    muse_ai_directives,         /* command table */
    register_hooks,             /* register hooks */
    0                           /* flags */
};
