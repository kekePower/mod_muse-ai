#include "mod_muse_ai.h"

/* Module configuration directives */
static const command_rec muse_ai_directives[] = {
    AP_INIT_TAKE1("MuseAiEndpoint", set_muse_ai_endpoint, NULL, RSRC_CONF,
                  "MuseWeb endpoint URL (default: http://127.0.0.1:11434/v1)"),
    AP_INIT_TAKE1("MuseAiTimeout", set_muse_ai_timeout, NULL, RSRC_CONF,
                  "Request timeout in seconds (default: 300)"),
    AP_INIT_FLAG("MuseAiDebug", set_muse_ai_debug, NULL, RSRC_CONF,
                 "Enable debug logging (default: Off)"),
    AP_INIT_TAKE1("MuseAiModel", set_muse_ai_model, NULL, RSRC_CONF,
                  "AI model to use (default: default)"),
    AP_INIT_TAKE1("MuseAiApiKey", set_muse_ai_api_key, NULL, RSRC_CONF,
                  "API key for authentication (optional)"),
    AP_INIT_FLAG("MuseAiStreaming", set_muse_ai_streaming, NULL, RSRC_CONF,
                 "Enable streaming responses (default: On)"),
    {NULL}
};

/* Configuration functions */
void *create_muse_ai_config(apr_pool_t *pool, server_rec *s)
{
    muse_ai_config *cfg = apr_pcalloc(pool, sizeof(muse_ai_config));
    cfg->endpoint = DEFAULT_ENDPOINT;
    cfg->timeout = MUSE_AI_DEFAULT_TIMEOUT;
    cfg->debug = DEFAULT_DEBUG;
    cfg->model = DEFAULT_MODEL;
    cfg->api_key = NULL;  /* No API key by default */
    cfg->streaming = DEFAULT_STREAMING;
    return cfg;
}

const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->endpoint = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->timeout = atoi(arg);
    return NULL;
}

const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->debug = flag;
    return NULL;
}

const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->model = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    if (arg && strlen(arg) > 0) {
        conf->api_key = apr_pstrdup(cmd->pool, arg);
    } else {
        conf->api_key = NULL;
    }
    return NULL;
}

const char *set_muse_ai_streaming(cmd_parms *cmd, void *cfg, int flag)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->streaming = flag;
    return NULL;
}

/* Main request handler */
int muse_ai_handler(request_rec *r)
{
    muse_ai_config *cfg;
    
    /* Log that we're being called */
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                 "mod_muse_ai: Handler called for URI: %s, handler: %s", 
                 r->uri, r->handler ? r->handler : "NULL");
    
    /* Only handle requests assigned to our handler */
    if (!r->handler || strcmp(r->handler, "muse-ai-handler")) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "mod_muse_ai: DECLINING request - handler mismatch (expected: muse-ai-handler, got: %s)", 
                     r->handler ? r->handler : "NULL");
        return DECLINED;
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                 "mod_muse_ai: ACCEPTING request - handler matches!");
    
    /* Get module configuration */
    cfg = (muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    
    /* Check if this is a request for AI content generation */
    if (r->method_number == M_POST || (r->args && strstr(r->args, "prompt="))) {
        /* Forward to MuseWeb backend for AI processing */
        return forward_to_museweb(r, cfg);
    }
    
    /* Set content type */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    
    /* Return configuration display page */
    ap_rputs("<!DOCTYPE html>\n", r);
    ap_rputs("<html>\n<head>\n", r);
    ap_rputs("<title>mod_muse-ai Phase 3</title>\n", r);
    ap_rputs("<style>\n", r);
    ap_rputs("body { font-family: Arial, sans-serif; margin: 40px; background: #f9f9f9; }\n", r);
    ap_rputs(".container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n", r);
    ap_rputs(".config { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 20px 0; }\n", r);
    ap_rputs(".success { color: #2d5a2d; }\n", r);
    ap_rputs(".phase3 { background: #e8f0ff; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #0066cc; }\n", r);
    ap_rputs("</style>\n", r);
    ap_rputs("</head>\n<body>\n", r);
    ap_rputs("<div class=\"container\">\n", r);
    ap_rputs("<h1 class=\"success\">🚀 mod_muse-ai Phase 3 - Streaming Ready!</h1>\n", r);
    ap_rputs("<p><strong>Congratulations!</strong> The mod_muse-ai Apache module now supports advanced streaming capabilities.</p>\n", r);
    
    /* Configuration display */
    ap_rputs("<div class=\"config\">\n", r);
    ap_rputs("<h2>Current Configuration</h2>\n", r);
    if (cfg) {
        ap_rprintf(r, "<p><strong>MuseWeb Endpoint:</strong> %s</p>\n", cfg->endpoint);
        ap_rprintf(r, "<p><strong>Timeout:</strong> %d seconds</p>\n", cfg->timeout);
        ap_rprintf(r, "<p><strong>Debug Mode:</strong> %s</p>\n", cfg->debug ? "Enabled" : "Disabled");
        ap_rprintf(r, "<p><strong>AI Model:</strong> %s</p>\n", cfg->model ? cfg->model : "default");
        ap_rprintf(r, "<p><strong>API Key:</strong> %s</p>\n", 
                   cfg->api_key ? "Configured (hidden for security)" : "Not configured");
        ap_rprintf(r, "<p><strong>Streaming:</strong> %s</p>\n", cfg->streaming ? "Enabled" : "Disabled");
    } else {
        ap_rputs("<p><em>Configuration not available (using defaults)</em></p>\n", r);
    }
    ap_rputs("</div>\n", r);
    
    /* Phase 3 features */
    ap_rputs("<div class=\"phase3\">\n", r);
    ap_rputs("<h2>🎯 Phase 3 Features Implemented</h2>\n", r);
    ap_rputs("<ul>\n", r);
    ap_rputs("<li>✅ <strong>Smart Streaming</strong> - Real-time AI response streaming with MuseWeb's 3-phase approach</li>\n", r);
    ap_rputs("<li>✅ <strong>Modular Architecture</strong> - Clean separation into streaming.c, sanitize.c, http_client.c, utils.c</li>\n", r);
    ap_rputs("<li>✅ <strong>SSE Processing</strong> - Server-Sent Events parsing and JSON content extraction</li>\n", r);
    ap_rputs("<li>✅ <strong>Content Sanitization</strong> - Markdown code fence removal and HTML extraction</li>\n", r);
    ap_rputs("<li>✅ <strong>Streaming Configuration</strong> - MuseAiStreaming directive for enable/disable control</li>\n", r);
    ap_rputs("<li>✅ <strong>HTML Boundary Detection</strong> - Smart start/stop streaming based on HTML structure</li>\n", r);
    ap_rputs("</ul>\n", r);
    ap_rputs("</div>\n", r);
    
    ap_rputs("<h2>Phase 1 & 2 Complete ✅</h2>\n", r);
    ap_rputs("<ul>\n", r);
    ap_rputs("<li>✅ Apache module compilation and loading</li>\n", r);
    ap_rputs("<li>✅ Configuration directives working</li>\n", r);
    ap_rputs("<li>✅ HTTP client with API key authentication</li>\n", r);
    ap_rputs("<li>✅ Commercial AI provider support (OpenAI, Google, Anthropic)</li>\n", r);
    ap_rputs("</ul>\n", r);
    
    ap_rputs("<h2>Next Steps - Phase 3 Advanced</h2>\n", r);
    ap_rputs("<ul>\n", r);
    ap_rputs("<li>Connection pooling and keep-alive</li>\n", r);
    ap_rputs("<li>Rate limiting and security hardening</li>\n", r);
    ap_rputs("<li>Prometheus metrics and observability</li>\n", r);
    ap_rputs("<li>Performance optimization and caching</li>\n", r);
    ap_rputs("</ul>\n", r);
    
    ap_rprintf(r, "<p><small><strong>Request Details:</strong> %s %s | Handler: %s</small></p>\n", 
               r->method, r->uri, r->handler);
    ap_rputs("</div>\n", r);
    ap_rputs("</body>\n</html>\n", r);
    
    return OK;
}

/* Forward request to MuseWeb backend for AI processing */
int forward_to_museweb(request_rec *r, muse_ai_config *cfg)
{
    apr_uri_t uri;
    char *backend_url;
    char *prompt = NULL;
    char *lang = NULL;
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Forwarding to MuseWeb backend: %s", cfg->endpoint);
    }
    
    /* Parse query parameters */
    if (r->args) {
        char *args_copy = apr_pstrdup(r->pool, r->args);
        char *token = strtok(args_copy, "&");
        
        while (token != NULL) {
            if (strncmp(token, "prompt=", 7) == 0) {
                prompt = url_decode(r->pool, token + 7);
            } else if (strncmp(token, "lang=", 5) == 0) {
                lang = url_decode(r->pool, token + 5);
            }
            token = strtok(NULL, "&");
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
    
    /* Build backend URL */
    if (apr_uri_parse(r->pool, cfg->endpoint, &uri) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "mod_muse_ai: Failed to parse endpoint URL: %s", cfg->endpoint);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    backend_url = apr_psprintf(r->pool, "%s://%s:%d%s",
                              uri.scheme ? uri.scheme : "http",
                              uri.hostname ? uri.hostname : "127.0.0.1",
                              uri.port ? uri.port : 11434,
                              uri.path ? uri.path : "/v1/chat/completions");
    
    /* Prepare prompt content with language support */
    char *final_prompt;
    if (lang && strlen(lang) > 0) {
        final_prompt = apr_psprintf(r->pool,
            "**VERY IMPORTANT:** Respond in %s language. %s", lang, prompt);
    } else {
        final_prompt = prompt;
    }
    
    /* Escape JSON content */
    char *escaped_content = escape_json_string(r->pool, final_prompt);
    
    /* Build proper OpenAI API JSON payload */
    char *json_payload = apr_psprintf(r->pool,
        "{\n"
        "  \"model\": \"%s\",\n"
        "  \"messages\": [\n"
        "    {\"role\": \"user\", \"content\": \"%s\"}\n"
        "  ],\n"
        "  \"stream\": %s\n"
        "}",
        cfg->model ? cfg->model : "default",
        escaped_content,
        cfg->streaming ? "true" : "false");
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Request payload: %s", json_payload);
    }
    
    /* Make request to backend */
    char *response_body = NULL;
    int result = make_backend_request(r, cfg, backend_url, json_payload, &response_body);
    
    if (result != OK) {
        ap_set_content_type(r, "text/html;charset=UTF-8");
        ap_rputs("<!DOCTYPE html>\n<html>\n<head>\n<title>AI Backend Error</title>\n</head>\n<body>\n", r);
        ap_rputs("<h1>❌ Backend Connection Error</h1>\n", r);
        ap_rprintf(r, "<p>Failed to connect to MuseWeb backend at: %s</p>\n", backend_url);
        ap_rputs("<p>Please check that your MuseWeb server (e.g., Ollama) is running and accessible.</p>\n", r);
        ap_rputs("</body>\n</html>\n", r);
        return result;
    }
    
    /* For non-streaming mode, process the complete response */
    if (!cfg->streaming && response_body) {
        /* Extract and display response content */
        ap_set_content_type(r, "text/html;charset=UTF-8");
        ap_rputs("<!DOCTYPE html>\n<html>\n<head>\n<title>AI Generated Content</title>\n</head>\n<body>\n", r);
        ap_rputs("<h1>🤖 MuseWeb AI Response</h1>\n", r);
        ap_rprintf(r, "<p><strong>Prompt:</strong> %s</p>\n", prompt);
        ap_rprintf(r, "<p><strong>AI Model:</strong> %s</p>\n", cfg->model ? cfg->model : "default");
        ap_rprintf(r, "<p><strong>API Key:</strong> %s</p>\n", 
                   cfg->api_key ? "Configured (hidden for security)" : "Not configured");
        ap_rputs("</div>\n", r);
        ap_rprintf(r, "<p><strong>Backend:</strong> %s</p>\n", backend_url);
        ap_rputs("<div style='background: #f0fff0; padding: 20px; margin: 20px 0; border-radius: 5px; border-left: 4px solid #4CAF50;'>\n", r);
        ap_rputs("<h2>🎯 AI Response</h2>\n", r);
        ap_rputs("<pre style='background: #f8f8f8; padding: 15px; border-radius: 3px; overflow-x: auto; white-space: pre-wrap;'>\n", r);
        ap_rputs(response_body, r);
        ap_rputs("</pre>\n", r);
        ap_rputs("</div>\n", r);
        ap_rputs("</body>\n</html>\n", r);
    }
    
    /* For streaming mode, the response was already sent by make_backend_request */
    return OK;
}

/* Module registration */
static void register_hooks(apr_pool_t *pool)
{
    ap_hook_handler(muse_ai_handler, NULL, NULL, APR_HOOK_MIDDLE);
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
