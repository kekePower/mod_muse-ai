/*
 * mod_muse_ai.c - Apache HTTP Server module for MuseWeb AI integration
 *
 * Phase 3: Advanced Feature Integration
 * This version activates all Phase 3 features, including connection pooling,
 * caching, metrics, and a multi-handler architecture. It fixes critical
 * integration bugs by correctly wiring the advanced configuration and handlers.
 */

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "ap_config.h"

/*
 * Phase 3 Headers
 * These headers contain the definitions for the advanced configuration,
 * handlers, and initialization routines that make up Phase 3.
 */
#include "advanced_config.h"
#include "request_handlers.h"

/* Forward declaration for the module */
module AP_MODULE_DECLARE_DATA muse_ai_module;

/*
 * Post-config hook to initialize Phase 3 features.
 * This function is called by Apache after the configuration has been parsed.
 * It retrieves the module's configuration and calls the main initializer
 * for the connection pool, metrics system, and other advanced features.
 */
static int muse_ai_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    /* Suppress unused parameter warnings */
    (void)plog;
    (void)ptemp;
    
    advanced_muse_ai_config *cfg = ap_get_module_config(s->module_config, &muse_ai_module);
    if (!cfg) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, "[mod_muse_ai] FATAL: Could not retrieve server config in post_config. Module will not function.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /* The main initialization logic is in request_handlers.c */
    return init_phase3_features(pconf, s, cfg);
}

/*
 * Hook for registering handlers and other hooks.
 * This function is called by Apache to set up the module. It wires up all the
 * new Phase 3 handlers and the critical post-configuration hook.
 */
static void muse_ai_register_hooks(apr_pool_t *p)
{
    /* Suppress unused parameter warning */
    (void)p;
    
    /*
     * Register the new Phase 3 handlers for their respective locations.
     * The handlers themselves are defined in request_handlers.c.
     * - enhanced_muse_ai_handler: Handles core AI requests to /ai
     * - metrics_handler: Exposes Prometheus-compatible metrics at /metrics
     * - health_check_handler: Provides a system health check at /health
     */
    ap_hook_handler(enhanced_muse_ai_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(metrics_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(health_check_handler, NULL, NULL, APR_HOOK_MIDDLE);

    /*
     * Register the post-config hook. This is critical for initializing
     * Phase 3 features like connection pools and metrics counters after
     * Apache has finished parsing the configuration files.
     */
    ap_hook_post_config(muse_ai_post_config, NULL, NULL, APR_HOOK_MIDDLE);
}

/*
 * Module definition.
 * This structure contains all the metadata about the module that Apache needs.
 * It points to the functions for creating/merging configuration structures,
 * the table of configuration directives, and the hook registration function.
 *
 * This has been updated to use all the Phase 3 components.
 */
module AP_MODULE_DECLARE_DATA muse_ai_module = {
    STANDARD20_MODULE_STUFF,
    NULL,                           /* create per-dir config */
    NULL,                           /* merge per-dir config */
    create_advanced_muse_ai_config, /* create per-server config */
    merge_advanced_muse_ai_config,  /* merge per-server config */
    muse_ai_advanced_cmds,          /* command table */
    muse_ai_register_hooks,         /* register hooks */
    0                               /* flags */
};
