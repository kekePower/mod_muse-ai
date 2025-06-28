/* 
 * Test version of modular mod_muse_ai with just streaming configuration
 * This is to isolate segfault issues
 */

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "ap_config.h"
#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_uri.h"
#include <string.h>

/* Module configuration structure */
typedef struct {
    char *endpoint;     /* MuseWeb endpoint URL */
    int timeout;        /* Request timeout in seconds */
    int debug;          /* Debug flag */
    char *model;        /* AI model to use */
    char *api_key;      /* API key for authentication */
    int streaming;      /* Enable streaming responses */
} muse_ai_config;

/* Default configuration values */
#define DEFAULT_ENDPOINT "http://127.0.0.1:11434/v1"
#define MUSE_AI_DEFAULT_TIMEOUT 300
#define DEFAULT_DEBUG 0
#define DEFAULT_MODEL "default"
#define DEFAULT_STREAMING 1  /* Enable streaming by default */

/* Forward declarations */
static int muse_ai_handler(request_rec *r);
static void *create_muse_ai_config(apr_pool_t *pool, server_rec *s);
static const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag);
static const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg);
static const char *set_muse_ai_streaming(cmd_parms *cmd, void *cfg, int flag);

/* Module declaration - must be declared before use in config functions */
module AP_MODULE_DECLARE_DATA muse_ai_module;

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
static void *create_muse_ai_config(apr_pool_t *pool, server_rec *s)
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

static const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    if (arg && strlen(arg) > 0) {
        conf->api_key = apr_pstrdup(cmd->pool, arg);
    } else {
        conf->api_key = NULL;
    }
    return NULL;
}

static const char *set_muse_ai_streaming(cmd_parms *cmd, void *cfg, int flag)
{
    muse_ai_config *conf = (muse_ai_config *)ap_get_module_config(cmd->server->module_config, &muse_ai_module);
    conf->streaming = flag;
    return NULL;
}

/* Main request handler - simplified test version */
static int muse_ai_handler(request_rec *r)
{
    muse_ai_config *cfg;
    
    /* Only handle requests assigned to our handler */
    if (!r->handler || strcmp(r->handler, "muse-ai-handler")) {
        return DECLINED;
    }
    
    /* Get module configuration */
    cfg = (muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    
    /* Set content type */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    
    /* Return test page */
    ap_rputs("<!DOCTYPE html>\n", r);
    ap_rputs("<html>\n<head>\n", r);
    ap_rputs("<title>mod_muse-ai Test</title>\n", r);
    ap_rputs("</head>\n<body>\n", r);
    ap_rputs("<h1>🧪 mod_muse-ai Modular Test</h1>\n", r);
    ap_rputs("<p>This is a test version to isolate segfault issues.</p>\n", r);
    
    /* Configuration display */
    if (cfg) {
        ap_rprintf(r, "<p><strong>Streaming:</strong> %s</p>\n", cfg->streaming ? "Enabled" : "Disabled");
        ap_rprintf(r, "<p><strong>Debug:</strong> %s</p>\n", cfg->debug ? "Enabled" : "Disabled");
        ap_rprintf(r, "<p><strong>Model:</strong> %s</p>\n", cfg->model ? cfg->model : "default");
    } else {
        ap_rputs("<p>Configuration not available</p>\n", r);
    }
    
    ap_rputs("</body>\n</html>\n", r);
    
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
