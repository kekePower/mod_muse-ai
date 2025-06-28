/*
 * mod_muse_ai.c - Apache HTTP Server module for MuseWeb AI integration
 * 
 * Phase 1: Proof-of-Concept
 * This module provides a basic handler that responds to requests with a greeting.
 * Future phases will integrate with MuseWeb backend for AI-powered HTML generation.
 */

#include "httpd.h"
#include "http_protocol.h"
#include "http_config.h"
#include "http_request.h"
#include "http_log.h"
#include "ap_config.h"
#include "apr_strings.h"
#include "apr_uri.h"
#include "apr_buckets.h"
#include "util_filter.h"

/* Module configuration structure */
typedef struct {
    char *endpoint;     /* MuseWeb endpoint URL */
    int timeout;        /* Request timeout in seconds */
    int debug;          /* Debug flag */
    char *model;        /* AI model to use */
} muse_ai_config;

/* Default configuration values */
#define DEFAULT_ENDPOINT "http://127.0.0.1:11434/v1"
#define MUSE_AI_DEFAULT_TIMEOUT 300
#define DEFAULT_DEBUG 0
#define DEFAULT_MODEL "default"

/* Forward declarations */
static int muse_ai_handler(request_rec *r);
static void muse_ai_register_hooks(apr_pool_t *p);
static void *create_muse_ai_config(apr_pool_t *p, server_rec *s);
static const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag);
static const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg);
static int forward_to_museweb(request_rec *r, muse_ai_config *cfg);

/* Module declaration - must be declared before use in config functions */
module AP_MODULE_DECLARE_DATA muse_ai_module;

/* Configuration directives */
static const command_rec muse_ai_cmds[] = {
    AP_INIT_TAKE1("MuseAiEndpoint", set_muse_ai_endpoint, NULL, RSRC_CONF,
                  "MuseWeb server URL (default: http://127.0.0.1:11434/v1)"),
    AP_INIT_TAKE1("MuseAiTimeout", set_muse_ai_timeout, NULL, RSRC_CONF,
                  "Request timeout in seconds (default: 300)"),
    AP_INIT_FLAG("MuseAiDebug", set_muse_ai_debug, NULL, RSRC_CONF,
                 "Enable debug logging (default: Off)"),
    AP_INIT_TAKE1("MuseAiModel", set_muse_ai_model, NULL, RSRC_CONF,
                  "AI model to use (default: default)"),
    {NULL}
};

/* Create server configuration */
static void *create_muse_ai_config(apr_pool_t *p, server_rec *s)
{
    muse_ai_config *cfg = apr_pcalloc(p, sizeof(muse_ai_config));
    cfg->endpoint = DEFAULT_ENDPOINT;
    cfg->timeout = MUSE_AI_DEFAULT_TIMEOUT;
    cfg->debug = DEFAULT_DEBUG;
    cfg->model = DEFAULT_MODEL;
    return cfg;
}

/* Configuration directive handlers */
static const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->endpoint = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

static const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->timeout = atoi(arg);
    if (conf->timeout <= 0) {
        return "MuseAiTimeout must be a positive integer";
    }
    return NULL;
}

static const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->debug = flag;
    return NULL;
}

static const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->model = apr_pstrdup(cmd->pool, arg);
    return NULL;
}

/* Main request handler */
static int muse_ai_handler(request_rec *r)
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
    
    /* Phase 2: Check if this is a request for AI content generation */
    if (r->method_number == M_POST || (r->args && strstr(r->args, "prompt="))) {
        /* Forward to MuseWeb backend for AI processing */
        return forward_to_museweb(r, cfg);
    }
    
    /* Set content type */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    
    /* Phase 1: Return a comprehensive PoC response */
    ap_rputs("<!DOCTYPE html>\n", r);
    ap_rputs("<html>\n<head>\n", r);
    ap_rputs("<title>mod_muse-ai PoC</title>\n", r);
    ap_rputs("<style>\n", r);
    ap_rputs("body { font-family: Arial, sans-serif; margin: 40px; background: #f9f9f9; }\n", r);
    ap_rputs(".container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n", r);
    ap_rputs(".config { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 20px 0; }\n", r);
    ap_rputs(".success { color: #2d5a2d; }\n", r);
    ap_rputs("</style>\n", r);
    ap_rputs("</head>\n<body>\n", r);
    ap_rputs("<div class=\"container\">\n", r);
    ap_rputs("<h1 class=\"success\">🎵 mod_muse-ai SUCCESS!</h1>\n", r);
    ap_rputs("<p><strong>Congratulations!</strong> The mod_muse-ai Apache module is successfully loaded and handling requests.</p>\n", r);
    
    ap_rputs("<div class=\"config\">\n", r);
    ap_rputs("<h2>Current Configuration</h2>\n", r);
    if (cfg && cfg->endpoint) {
        ap_rprintf(r, "<p><strong>MuseWeb Endpoint:</strong> %s</p>\n", cfg->endpoint);
        ap_rprintf(r, "<p><strong>Timeout:</strong> %d seconds</p>\n", cfg->timeout);
        ap_rprintf(r, "<p><strong>Debug Mode:</strong> %s</p>\n", cfg->debug ? "Enabled" : "Disabled");
        ap_rprintf(r, "<p><strong>AI Model:</strong> %s</p>\n", cfg->model ? cfg->model : "default");
    } else {
        ap_rputs("<p><em>Configuration not available (using defaults)</em></p>\n", r);
    }
    ap_rputs("</div>\n", r);
    
    ap_rputs("<h2>Phase 1 Complete ✅</h2>\n", r);
    ap_rputs("<ul>\n", r);
    ap_rputs("<li>✅ Apache module compilation and loading</li>\n", r);
    ap_rputs("<li>✅ Configuration directives working</li>\n", r);
    ap_rputs("<li>✅ Request handler registration and invocation</li>\n", r);
    ap_rputs("<li>✅ Both Location and AddHandler mapping working</li>\n", r);
    ap_rputs("</ul>\n", r);
    
    ap_rputs("<h2>Next Steps - Phase 2</h2>\n", r);
    ap_rputs("<ul>\n", r);
    ap_rputs("<li>Integrate with MuseWeb backend for AI-powered content generation</li>\n", r);
    ap_rputs("<li>Add HTTP client for backend communication</li>\n", r);
    ap_rputs("<li>Implement streaming response support</li>\n", r);
    ap_rputs("</ul>\n", r);
    
    ap_rprintf(r, "<p><small><strong>Request Details:</strong> %s %s | Handler: %s</small></p>\n", 
               r->method, r->uri, r->handler);
    ap_rputs("</div>\n", r);
    ap_rputs("</body>\n</html>\n", r);
    
    return OK;
}

/* Forward request to MuseWeb backend for AI processing */
static int forward_to_museweb(request_rec *r, muse_ai_config *cfg)
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
        char *token, *last;
        
        for (token = apr_strtok(args_copy, "&", &last); token; token = apr_strtok(NULL, "&", &last)) {
            if (strncmp(token, "prompt=", 7) == 0) {
                prompt = token + 7;
                /* URL decode the prompt */
                ap_unescape_url(prompt);
            } else if (strncmp(token, "lang=", 5) == 0) {
                lang = token + 5;
                ap_unescape_url(lang);
            }
        }
    }
    
    /* For POST requests, read the body for prompt data */
    if (r->method_number == M_POST) {
        /* TODO: Read POST body for prompt data */
        /* For now, use a default prompt */
        prompt = "Generate a simple HTML page";
    }
    
    /* If no prompt specified, show usage */
    if (!prompt) {
        ap_set_content_type(r, "text/html;charset=UTF-8");
        ap_rputs("<!DOCTYPE html>\n<html>\n<head>\n<title>MuseWeb AI - Usage</title>\n</head>\n<body>\n", r);
        ap_rputs("<h1>🎵 MuseWeb AI Integration</h1>\n", r);
        ap_rputs("<p>To generate AI content, use one of these methods:</p>\n", r);
        ap_rputs("<h2>GET Request with Query Parameter:</h2>\n", r);
        ap_rputs("<pre>http://localhost/ai?prompt=Create a homepage for a bakery</pre>\n", r);
        ap_rputs("<h2>With Language Translation:</h2>\n", r);
        ap_rputs("<pre>http://localhost/ai?prompt=Create a homepage&lang=es_ES</pre>\n", r);
        ap_rputs("<h2>POST Request:</h2>\n", r);
        ap_rputs("<pre>curl -X POST http://localhost/ai -d 'prompt=Your content request'</pre>\n", r);
        ap_rputs("</body>\n</html>\n", r);
        return OK;
    }
    
    /* Build OpenAI API compatible backend URL */
    /* For OpenAI API, we use /v1/chat/completions endpoint */
    backend_url = apr_psprintf(r->pool, "%s/v1/chat/completions", cfg->endpoint);
    
    /* Note: For OpenAI API, we'll need to send a POST request with JSON body containing:
     * {
     *   "model": "model_name",
     *   "messages": [{"role": "user", "content": "prompt_with_lang_instruction"}],
     *   "stream": true
     * }
     */
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Backend URL: %s", backend_url);
    }
    
    /* Parse the backend URL */
    if (apr_uri_parse(r->pool, backend_url, &uri) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "mod_muse_ai: Failed to parse backend URL: %s", backend_url);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Build OpenAI API JSON payload with safe escaping */
    char *json_content;
    if (lang) {
        json_content = apr_psprintf(r->pool, 
            "Generate content in %s language: %s", lang, prompt);
    } else {
        json_content = prompt;
    }
    
    /* Simple JSON payload for preview (actual implementation will need proper JSON escaping) */
    char *json_payload = apr_psprintf(r->pool,
        "{\n"
        "  \"model\": \"%s\",\n"
        "  \"messages\": [\n"
        "    {\"role\": \"user\", \"content\": \"[PROMPT_CONTENT]\"}\n"
        "  ],\n"
        "  \"stream\": true\n"
        "}",
        cfg->model ? cfg->model : "default");
    
    /* For Phase 2 initial implementation, return a simulated response */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    ap_rputs("<!DOCTYPE html>\n<html>\n<head>\n<title>AI Generated Content</title>\n</head>\n<body>\n", r);
    ap_rputs("<h1>🤖 OpenAI API Integration (Phase 2 Preview)</h1>\n", r);
    ap_rprintf(r, "<p><strong>Prompt:</strong> %s</p>\n", prompt);
    if (lang) {
        ap_rprintf(r, "<p><strong>Language:</strong> %s</p>\n", lang);
    }
    ap_rprintf(r, "<p><strong>Model:</strong> %s</p>\n", cfg->model ? cfg->model : "default");
    ap_rprintf(r, "<p><strong>API Endpoint:</strong> %s</p>\n", backend_url);
    
    ap_rputs("<div style='background: #f0f8ff; padding: 20px; margin: 20px 0; border-radius: 5px;'>\n", r);
    ap_rputs("<h2>OpenAI API Request Preview</h2>\n", r);
    ap_rputs("<pre style='background: #f8f8f8; padding: 10px; border-radius: 3px; overflow-x: auto;'>\n", r);
    ap_rprintf(r, "POST %s\n", backend_url);
    ap_rputs("Content-Type: application/json\n\n", r);
    ap_rprintf(r, "%s", json_payload);
    ap_rputs("</pre>\n", r);
    ap_rprintf(r, "<p><strong>Actual Content:</strong> %s</p>\n", json_content);
    ap_rputs("<p>The [PROMPT_CONTENT] placeholder will be replaced with the actual content above, properly JSON-escaped.</p>\n", r);
    ap_rputs("</div>\n", r);
    ap_rputs("</body>\n</html>\n", r);
    
    return OK;
}

/* Register hooks */
static void muse_ai_register_hooks(apr_pool_t *p)
{
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                "mod_muse_ai: Registering handler hook with name 'muse-ai-handler'");
    ap_hook_handler(muse_ai_handler, NULL, NULL, APR_HOOK_FIRST);
}

/* Module definition */
module AP_MODULE_DECLARE_DATA muse_ai_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                       /* create per-directory config structure */
    NULL,                       /* merge per-directory config structures */
    create_muse_ai_config,      /* create per-server config structure */
    NULL,                       /* merge per-server config structures */
    muse_ai_cmds,              /* command table */
    muse_ai_register_hooks     /* register hooks */
};
