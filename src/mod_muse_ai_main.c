#include "mod_muse_ai.h"
#include "advanced_config.h"

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
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->endpoint = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg)
{
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->timeout = atoi(arg);
    return NULL;
}

const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag)
{
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->debug = flag;
    return NULL;
}

const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg)
{
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->model = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg)
{
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
    advanced_muse_ai_config *conf = (advanced_muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->streaming = flag;
    return NULL;
}

/* Phase 3 handler declarations */
int metrics_handler(request_rec *r);
int health_check_handler(request_rec *r);

/* Main request handler */
int muse_ai_handler(request_rec *r)
{
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Handler called for URI: %s, Handler: %s", r->uri, r->handler ? r->handler : "NULL");
    
    /* Phase 3: Support multiple handler types */
    if (r->handler && strcmp(r->handler, "muse-ai-metrics") == 0) {
        return metrics_handler(r);
    }
    if (r->handler && strcmp(r->handler, "muse-ai-health") == 0) {
        return health_check_handler(r);
    }
    
    /* Default AI handler */
    if (!r->handler || strcmp(r->handler, "muse-ai-handler")) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Declining request - handler mismatch. Expected: muse-ai-handler, Got: %s", r->handler ? r->handler : "NULL");
        return DECLINED;
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Handler accepted request for %s", r->uri);

    if (r->method_number != M_GET && r->method_number != M_POST) {
        return HTTP_METHOD_NOT_ALLOWED;
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Getting module config.");
    advanced_muse_ai_config *adv_cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    if (!adv_cfg) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "Fatal: advanced_muse_ai_config is NULL. Check module configuration.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "Successfully retrieved module config. Debug flag: %d", adv_cfg->debug);
    
    /* Create a basic config structure for backward compatibility */
    muse_ai_config basic_cfg;
    basic_cfg.endpoint = adv_cfg->endpoint;
    basic_cfg.api_key = adv_cfg->api_key;
    basic_cfg.model = adv_cfg->model;
    basic_cfg.timeout = adv_cfg->timeout;
    basic_cfg.debug = adv_cfg->debug;
    basic_cfg.streaming = adv_cfg->streaming;
    
    return forward_to_museweb(r, &basic_cfg);
}

/* Forward request to MuseWeb backend for AI processing */
int forward_to_museweb(request_rec *r, muse_ai_config *cfg)
{
    apr_uri_t uri;
    char *backend_url;
    char *prompt = NULL;
    char *lang = NULL;

    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Forwarding to MuseWeb backend: %s", cfg->endpoint);
    }

    if (r->method_number == M_POST) {
        if (cfg->debug) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Handling POST request.");
        }
        int rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR);
        if (rc != OK) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "mod_muse_ai: Failed to setup client block, status=%d", rc);
            return rc;
        }
        if (ap_should_client_block(r)) {
            if (cfg->debug) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Reading client block.");
            }
            char buffer[HUGE_STRING_LEN];
            int bytes_read;
            apr_size_t total_bytes_read = 0;
            char *post_data = "";

            while ((bytes_read = ap_get_client_block(r, buffer, sizeof(buffer))) > 0) {
                total_bytes_read += bytes_read;
                post_data = apr_pstrcat(r->pool, post_data, apr_pstrndup(r->pool, buffer, bytes_read), NULL);
            }
            if (cfg->debug) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Finished reading client block, %" APR_SIZE_T_FMT " bytes read.", total_bytes_read);
            }

            if (total_bytes_read > 0) {
                if (cfg->debug) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Parsing POST data: %s", post_data);
                }
                char *decoded_args = url_decode(r->pool, post_data);
                char *last_state;
                char *pair = apr_strtok(decoded_args, "&", &last_state);
                while (pair) {
                    char *eq = strchr(pair, '=');
                    if (eq) {
                        *eq = '\0';
                        if (strcmp(pair, "prompt") == 0) {
                            prompt = eq + 1;
                        } else if (strcmp(pair, "lang") == 0) {
                            lang = eq + 1;
                        }
                    }
                    pair = apr_strtok(NULL, "&", &last_state);
                }
                if (cfg->debug) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Parsed prompt from POST: %s", prompt ? prompt : "(null)");
                }
            }
        }
    }

    if (!prompt && r->args) {
        char *decoded_args = url_decode(r->pool, r->args);
        char *last_state;
        char *pair = apr_strtok(decoded_args, "&", &last_state);
        while (pair) {
            char *eq = strchr(pair, '=');
            if (eq) {
                *eq = '\0';
                if (strcmp(pair, "prompt") == 0) {
                    prompt = eq + 1;
                } else if (strcmp(pair, "lang") == 0) {
                    lang = eq + 1;
                }
            }
            pair = apr_strtok(NULL, "&", &last_state);
        }
    }

    if (!prompt) {
        ap_set_content_type(r, "text/html;charset=UTF-8");
        ap_rputs("<!DOCTYPE html>\n<html>\n<head>\n<title>Missing Prompt</title>\n</head>\n<body>\n", r);
        ap_rputs("<h1>❌ Error</h1>\n", r);
        ap_rputs("<p>No prompt provided. Use: <code>?prompt=your_prompt_here</code></p>\n", r);
        ap_rputs("</body>\n</html>\n", r);
        return OK;
    }

    if (apr_uri_parse(r->pool, cfg->endpoint, &uri) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "mod_muse_ai: Failed to parse endpoint URL: %s", cfg->endpoint);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    // Force the correct API endpoint - always use /v1/chat/completions
    backend_url = apr_psprintf(r->pool, "%s://%s:%d/v1/chat/completions", 
                              uri.scheme ? uri.scheme : "http", 
                              uri.hostname ? uri.hostname : "127.0.0.1", 
                              uri.port ? uri.port : 11434);
    
    // Use ERROR level to ensure this message appears in logs
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "DEBUG: mod_muse_ai: Constructed backend URL: %s", backend_url);
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "DEBUG: mod_muse_ai: Original endpoint was: %s", cfg->endpoint);

    char *final_prompt;
    if (lang && strlen(lang) > 0) {
        final_prompt = apr_psprintf(r->pool, "**VERY IMPORTANT:** Respond in %s language. %s", lang, prompt);
    } else {
        final_prompt = prompt;
    }

    char *escaped_content = escape_json_string(r->pool, final_prompt);
    char *json_payload = apr_psprintf(r->pool, "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"stream\": %s\n}", cfg->model ? cfg->model : "default", escaped_content, cfg->streaming ? "true" : "false");

    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Request payload: %s", json_payload);
    }

    char *response_body = NULL;
    
    // CRITICAL DEBUG: Show the actual URL being passed to HTTP client
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "CRITICAL DEBUG: About to call make_backend_request with URL: %s", backend_url);
    
    return make_backend_request(r, cfg, backend_url, json_payload, &response_body);
}

/* Module registration */
static void register_hooks(apr_pool_t *pool)
{
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, "[mod_muse_ai] Registering hooks - module is loading");
    ap_hook_handler(muse_ai_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, NULL, "[mod_muse_ai] Handler hook registered successfully");
}

module AP_MODULE_DECLARE_DATA muse_ai_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                       /* create per-directory config structure */
    NULL,                       /* merge per-directory config structures */
    create_muse_ai_config,      /* create per-server config structure */
    NULL,                       /* merge per-server config structures */
    muse_ai_directives,         /* command table */
    register_hooks              /* register hooks */
};
