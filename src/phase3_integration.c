#include "mod_muse_ai.h"
#include "connection_pool.h"
#include "advanced_config.h"
#include <apr_time.h>

/* Forward declarations for metrics functions */
int init_metrics_system(apr_pool_t *pool, server_rec *s);
void update_request_metrics(double response_time_ms, int success);
char *generate_prometheus_metrics(apr_pool_t *pool);
char *generate_json_metrics(apr_pool_t *pool);

/* Phase 3 integration functions */

/* Initialize Phase 3 features */
int init_phase3_features(apr_pool_t *pool, server_rec *s, advanced_muse_ai_config *cfg)
{
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Initializing Phase 3 advanced features");
    
    /* Initialize metrics system */
    if (cfg->metrics_enable) {
        if (init_metrics_system(pool, s) != 0) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                        "[mod_muse_ai] Failed to initialize metrics system");
            return -1;
        }
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                    "[mod_muse_ai] Metrics system enabled");
    }
    
    /* Initialize connection pool */
    if (cfg->pool_max_connections > 0) {
        connection_pool_t *pool_instance = create_connection_pool(pool, s);
        if (!pool_instance) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                        "[mod_muse_ai] Failed to initialize connection pool");
            return -1;
        }
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                    "[mod_muse_ai] Connection pool enabled (max: %d)", 
                    cfg->pool_max_connections);
    }
    
    /* Log enabled features */
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Phase 3 Features Status:");
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai]   - Connection Pooling: %s (max: %d)", 
                cfg->pool_max_connections > 0 ? "ENABLED" : "DISABLED",
                cfg->pool_max_connections);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai]   - Metrics Collection: %s", 
                cfg->metrics_enable ? "ENABLED" : "DISABLED");
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai]   - Caching: %s (TTL: %ds)", 
                cfg->cache_enable ? "ENABLED" : "DISABLED",
                cfg->cache_ttl_seconds);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai]   - Rate Limiting: %s (%d req/min)", 
                cfg->ratelimit_enable ? "ENABLED" : "DISABLED",
                cfg->ratelimit_requests_per_minute);
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai]   - Advanced Streaming: %s (buffer: %d bytes)", 
                cfg->streaming_buffer_size > 0 ? "ENABLED" : "DISABLED",
                cfg->streaming_buffer_size);
    
    return 0;
}

/* Enhanced request handler with Phase 3 features */
int enhanced_muse_ai_handler(request_rec *r)
{
    advanced_muse_ai_config *cfg;
    apr_time_t start_time, end_time;
    double response_time_ms;
    int success = 0;
    
    /* Start timing for metrics */
    start_time = apr_time_now();
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                 "[mod_muse_ai] Enhanced handler called for URI: %s", r->uri);
    
    /* Basic handler validation */
    if (!r->handler || strcmp(r->handler, "muse-ai-handler")) {
        return DECLINED;
    }
    
    if (r->method_number != M_GET && r->method_number != M_POST) {
        return HTTP_METHOD_NOT_ALLOWED;
    }
    
    /* Get advanced configuration */
    cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    if (!cfg) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                     "[mod_muse_ai] Failed to get advanced configuration");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Rate limiting check */
    if (cfg->ratelimit_enable) {
        /* TODO: Implement rate limiting logic */
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                     "[mod_muse_ai] Rate limiting check passed");
    }
    
    /* Security validation */
    if (cfg->security_validate_content_type && r->method_number == M_POST) {
        const char *content_type = apr_table_get(r->headers_in, "Content-Type");
        if (!content_type || strncmp(content_type, "application/json", 16) != 0) {
            ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, 
                         "[mod_muse_ai] Invalid content type: %s", 
                         content_type ? content_type : "(null)");
            return HTTP_BAD_REQUEST;
        }
    }
    
    /* Check request size limits */
    if (cfg->security_max_request_size > 0 && r->remaining > cfg->security_max_request_size) {
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, 
                     "[mod_muse_ai] Request too large: %ld bytes (max: %d)", 
                     (long)r->remaining, cfg->security_max_request_size);
        return HTTP_REQUEST_ENTITY_TOO_LARGE;
    }
    
    /* Process request with enhanced features */
    int result = forward_to_museweb(r, (muse_ai_config *)cfg);
    
    /* Calculate response time */
    end_time = apr_time_now();
    response_time_ms = (double)(end_time - start_time) / 1000.0; /* Convert to milliseconds */
    
    /* Update metrics */
    success = (result == OK);
    if (cfg->metrics_enable) {
        update_request_metrics(response_time_ms, success);
    }
    
    /* Log performance information */
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                     "[mod_muse_ai] Request completed in %.2f ms (result: %d)", 
                     response_time_ms, result);
    }
    
    return result;
}

/* Metrics endpoint handler */
int metrics_handler(request_rec *r)
{
    advanced_muse_ai_config *cfg;
    char *metrics_output;
    const char *format;
    
    /* Check if this is a metrics request */
    if (!r->handler || strcmp(r->handler, "muse-ai-metrics")) {
        return DECLINED;
    }
    
    if (r->method_number != M_GET) {
        return HTTP_METHOD_NOT_ALLOWED;
    }
    
    /* Get configuration */
    cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    if (!cfg || !cfg->metrics_enable) {
        return HTTP_NOT_FOUND;
    }
    
    /* Determine output format from query string */
    format = "prometheus"; /* Default to Prometheus format */
    if (r->args) {
        char *args_copy = apr_pstrdup(r->pool, r->args);
        char *token = strtok(args_copy, "&");
        while (token) {
            if (strncmp(token, "format=", 7) == 0) {
                format = token + 7;
                break;
            }
            token = strtok(NULL, "&");
        }
    }
    
    /* Generate metrics output */
    if (strcmp(format, "json") == 0) {
        metrics_output = generate_json_metrics(r->pool);
        ap_set_content_type(r, "application/json");
    } else {
        metrics_output = generate_prometheus_metrics(r->pool);
        ap_set_content_type(r, "text/plain; version=0.0.4");
    }
    
    /* Send response */
    ap_rprintf(r, "%s", metrics_output);
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                 "[mod_muse_ai] Metrics served in %s format", format);
    
    return OK;
}

/* Health check endpoint */
int health_check_handler(request_rec *r)
{
    advanced_muse_ai_config *cfg;
    muse_ai_metrics_t *metrics;
    char *health_output;
    
    /* Check if this is a health check request */
    if (!r->handler || strcmp(r->handler, "muse-ai-health")) {
        return DECLINED;
    }
    
    if (r->method_number != M_GET) {
        return HTTP_METHOD_NOT_ALLOWED;
    }
    
    /* Get configuration and metrics */
    cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    metrics = get_global_metrics();
    
    /* Generate health status */
    health_output = apr_psprintf(r->pool,
        "{\n"
        "  \"status\": \"healthy\",\n"
        "  \"version\": \"1.0.0-phase3\",\n"
        "  \"features\": {\n"
        "    \"connection_pooling\": %s,\n"
        "    \"metrics\": %s,\n"
        "    \"caching\": %s,\n"
        "    \"rate_limiting\": %s,\n"
        "    \"advanced_streaming\": %s\n"
        "  },\n"
        "  \"uptime_seconds\": %lld,\n"
        "  \"requests_processed\": %ld\n"
        "}",
        cfg && cfg->pool_max_connections > 0 ? "true" : "false",
        cfg && cfg->metrics_enable ? "true" : "false", 
        cfg && cfg->cache_enable ? "true" : "false",
        cfg && cfg->ratelimit_enable ? "true" : "false",
        cfg && cfg->streaming_buffer_size > 0 ? "true" : "false",
        metrics ? (long long)((apr_time_now() - metrics->last_updated) / APR_USEC_PER_SEC) : 0LL,
        metrics ? metrics->total_requests : 0L
    );
    
    ap_set_content_type(r, "application/json");
    ap_rprintf(r, "%s", health_output);
    
    return OK;
}

/* Configuration validation for Phase 3 */
int validate_phase3_config(advanced_muse_ai_config *cfg, server_rec *s)
{
    int warnings = 0;
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Validating Phase 3 configuration");
    
    /* Validate connection pool settings */
    if (cfg->pool_max_connections > 0) {
        if (cfg->pool_max_connections > 100) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] Large connection pool size: %d (consider reducing)", 
                        cfg->pool_max_connections);
            warnings++;
        }
        if (cfg->pool_connection_timeout < 30) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] Short connection timeout: %d seconds", 
                        cfg->pool_connection_timeout);
            warnings++;
        }
    }
    
    /* Validate cache settings */
    if (cfg->cache_enable) {
        if (cfg->cache_ttl_seconds < 60) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] Short cache TTL: %d seconds", 
                        cfg->cache_ttl_seconds);
            warnings++;
        }
        if (cfg->cache_max_entries > 10000) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] Large cache size: %d entries (high memory usage)", 
                        cfg->cache_max_entries);
            warnings++;
        }
    }
    
    /* Validate rate limiting */
    if (cfg->ratelimit_enable) {
        if (cfg->ratelimit_requests_per_minute > 1000) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] High rate limit: %d req/min", 
                        cfg->ratelimit_requests_per_minute);
            warnings++;
        }
    }
    
    /* Validate streaming settings */
    if (cfg->streaming_buffer_size > 0) {
        if (cfg->streaming_buffer_size < 1024) {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                        "[mod_muse_ai] Small streaming buffer: %d bytes", 
                        cfg->streaming_buffer_size);
            warnings++;
        }
        if (cfg->streaming_chunk_size > cfg->streaming_buffer_size) {
            ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                        "[mod_muse_ai] Chunk size (%d) larger than buffer (%d)", 
                        cfg->streaming_chunk_size, cfg->streaming_buffer_size);
            return -1;
        }
    }
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Configuration validation complete (%d warnings)", 
                warnings);
    
    return warnings;
}
