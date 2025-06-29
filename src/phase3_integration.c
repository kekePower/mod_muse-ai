#include "mod_muse_ai.h"
#include "http_protocol.h"
#include "connection_pool.h"
#include "advanced_config.h"
#include <apr_time.h>
#include "cJSON.h"

/* Forward declarations for metrics functions */
int init_metrics_system(apr_pool_t *pool, server_rec *s);
void update_request_metrics(double response_time_ms, int success);
char *generate_prometheus_metrics(apr_pool_t *pool);
char *generate_json_metrics(apr_pool_t *pool);

/* Forward declaration for the main module structure */
extern module AP_MODULE_DECLARE_DATA muse_ai_module;

/* Phase 3 integration functions */

/* Initialize Phase 3 features */
int init_phase3_features(apr_pool_t *pool, server_rec *s, advanced_muse_ai_config *cfg)
{
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Phase 3 initialization DISABLED for debugging");
    return 0; // Early return to bypass all Phase 3 initialization
    
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
    /* We only handle requests for /ai */
    if (strcmp(r->uri, "/ai") != 0) {
        return DECLINED;
    }
    advanced_muse_ai_config *cfg;
    apr_time_t start_time, end_time;
    double response_time_ms;
    int success = 0;
    char *json_payload = NULL;
    char *prompt = NULL;

    start_time = apr_time_now();
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Enhanced handler called for URI: %s", r->uri);

    if (r->method_number != M_GET && r->method_number != M_POST) {
        return HTTP_METHOD_NOT_ALLOWED;
    }

    cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    if (!cfg) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Failed to get advanced configuration");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] HANDLER USING ENDPOINT: %s", cfg->endpoint ? cfg->endpoint : "(not set)");

    /* Security validation */
    if (cfg->security_validate_content_type && r->method_number == M_POST) {
        const char *content_type = apr_table_get(r->headers_in, "Content-Type");
        if (!content_type || strncmp(content_type, "application/json", 16) != 0) {
            ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "[mod_muse_ai] Invalid content type: %s", content_type ? content_type : "(null)");
            return HTTP_BAD_REQUEST;
        }
    }

    /* Check request size limits */
    if (cfg->security_max_request_size > 0 && r->remaining > cfg->security_max_request_size) {
        ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "[mod_muse_ai] Request too large: %ld bytes (max: %d)", (long)r->remaining, cfg->security_max_request_size);
        return HTTP_REQUEST_ENTITY_TOO_LARGE;
    }

    // --- UNIFIED PROMPT RESOLUTION LOGIC ---

    // Priority 1: POST request with JSON body
    if (r->method_number == M_POST) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Handling POST request. Reading body.");
        apr_bucket_brigade *bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
        if (ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ, 8192) == APR_SUCCESS) {
            char *buf = apr_palloc(r->pool, 8193);
            apr_size_t len;
            if (apr_brigade_pflatten(bb, &buf, &len, r->pool) == APR_SUCCESS) {
                buf[len] = '\0';
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Read POST body: %s", buf);
                cJSON *json_body = cJSON_Parse(buf);
                if (json_body) {
                    cJSON *messages = cJSON_GetObjectItem(json_body, "messages");
                    if (messages && cJSON_IsArray(messages)) {
                        cJSON *last_message = cJSON_GetArrayItem(messages, cJSON_GetArraySize(messages) - 1);
                        if (last_message) {
                            cJSON *content = cJSON_GetObjectItem(last_message, "content");
                            if (content && cJSON_IsString(content)) {
                                prompt = apr_pstrdup(r->pool, content->valuestring);
                                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Extracted prompt from messages array: %s", prompt);
                            }
                        }
                    }
                    cJSON_Delete(json_body);
                }
            }
            apr_brigade_destroy(bb);
        }
    }
    // Priority 2: GET request
    else if (r->method_number == M_GET) {
        // Priority 2a: GET with ?prompt=... parameter
        if (r->args) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: START. Checking args.");
            char *decoded_args = url_decode(r->pool, r->args);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: url_decode finished.");

            if (decoded_args) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: decoded_args is not null. Value: %s", decoded_args);
                char *last_state;
                char *pair = apr_strtok(decoded_args, "&", &last_state);
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: first apr_strtok finished.");

                while (pair) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: loop start. Pair: %s", pair);
                    char *eq = strchr(pair, '=');
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: strchr finished.");

                    if (eq && strncmp(pair, "prompt", eq - pair) == 0) {
                        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: found prompt key.");
                        prompt = apr_pstrdup(r->pool, eq + 1);
                        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: extracted prompt value: %s", prompt);
                        break;
                    }
                    pair = apr_strtok(NULL, "&", &last_state);
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: next apr_strtok finished.");
                }
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: loop finished.");
            } else {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: decoded_args is null.");
            }
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] GET query handler: END.");
        }
        // Priority 2b: GET with Prompts Directory configured (file-based content)
        else if (cfg->prompts_dir && strlen(cfg->prompts_dir) > 0) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Handling GET with Prompts Directory for URI: %s", r->uri);
            char *system_prompt_path = apr_pstrcat(r->pool, cfg->prompts_dir, "/system_prompt.ai", NULL);
            char *system_prompt = read_file_contents(r->pool, system_prompt_path);
            if (!system_prompt) {
                return HTTP_INTERNAL_SERVER_ERROR;
            }
            const char *layout_filename = cfg->prompts_minify ? "/layout.min.ai" : "/layout.ai";
            char *layout_prompt_path = apr_pstrcat(r->pool, cfg->prompts_dir, layout_filename, NULL);
            char *layout_prompt = read_file_contents(r->pool, layout_prompt_path);
            char *final_system_prompt = layout_prompt ? apr_pstrcat(r->pool, system_prompt, "\n\n", layout_prompt, NULL) : system_prompt;
            
            char *page_path = apr_pstrcat(r->pool, cfg->prompts_dir, r->uri, NULL);
            if (page_path[strlen(page_path) - 1] == '/') {
                page_path[strlen(page_path) - 1] = '\0';
            }
            char *page_prompt_index_path = apr_pstrcat(r->pool, page_path, "/index.ai", NULL);
            char *page_prompt_page_path = apr_pstrcat(r->pool, page_path, "/page.ai", NULL);
            
            char *user_prompt = read_file_contents(r->pool, page_prompt_index_path);
            if (!user_prompt) user_prompt = read_file_contents(r->pool, page_prompt_page_path);

            if (user_prompt) {
                char *escaped_system = escape_json_string(r->pool, final_system_prompt);
                char *escaped_user = escape_json_string(r->pool, user_prompt);
                json_payload = apr_psprintf(r->pool, "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"system\", \"content\": \"%s\"},\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"stream\": %s\n}",
                                           cfg->model ? cfg->model : "default", escaped_system, escaped_user, cfg->streaming ? "true" : "false");
            } else {
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "[mod_muse_ai] No index.ai or page.ai found for URI '%s'", r->uri);
                return HTTP_NOT_FOUND;
            }
        }
    }

    // If no prompt was extracted from any source, we can't proceed.
    if (!prompt && !json_payload) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] No prompt found in POST body, GET parameters, or file-based sources.");
        return HTTP_BAD_REQUEST;
    }

    // If we have a prompt (e.g., from a GET request) but no JSON payload yet, construct one.
    if (prompt && !json_payload) {
        char *escaped_prompt = escape_json_string(r->pool, prompt);
        if (!escaped_prompt) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Failed to escape prompt string.");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        json_payload = apr_psprintf(r->pool, 
                                    "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"stream\": %s\n}",
                                    cfg->model ? cfg->model : "default", 
                                    escaped_prompt, 
                                    cfg->streaming ? "true" : "false");
    }

    // Final check before making the request
    if (!json_payload) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Failed to construct JSON payload.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    if (cfg->debug && json_payload) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] JSON Payload: %s", json_payload);
    }

    char *response_body = NULL;
    int status = make_backend_request(r, (muse_ai_config *)cfg, cfg->endpoint, json_payload, &response_body);
    
    /* Calculate response time */
    end_time = apr_time_now();
    response_time_ms = (double)(end_time - start_time) / 1000.0; /* Convert to milliseconds */
    
    /* Update metrics */
    success = (status == OK);
    if (cfg->metrics_enable) {
        update_request_metrics(response_time_ms, success);
    }
    
    /* Log performance information */
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                     "[mod_muse_ai] Backend request completed in %.2f ms, status: %d",
                     response_time_ms, status);
    }
    
    return status;
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
