#include "mod_muse_ai.h"
#include "request_handlers.h"
#include "http_protocol.h"
#include "connection_pool.h"
#include "advanced_config.h"
#include "language_selection.h"
#include "supported_locales.h"
#include "error_pages.h"
#include <apr_time.h>
#include "cJSON.h"
#include "http_core.h"
#include <ctype.h>

/* Forward declarations for metrics functions */
int init_metrics_system(apr_pool_t *pool, server_rec *s);
void update_request_metrics(double response_time_ms, int success);
char *generate_prometheus_metrics(apr_pool_t *pool);
char *generate_json_metrics(apr_pool_t *pool);

/* Forward declaration for the main module structure */
extern module AP_MODULE_DECLARE_DATA muse_ai_module;

/* Language error handling is now handled by error_pages.c */

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

/* AI file handler for .ai files in document root */
int ai_file_handler(request_rec *r)
{
    advanced_muse_ai_config *cfg;
    char *ai_file_path = NULL;
    char *system_prompt = NULL;
    char *layout_prompt = NULL;
    char *page_prompt = NULL;
    char *json_payload = NULL;
    char *final_system_prompt = NULL;
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] AI file handler called for URI: %s", r->uri);
    
    /* Only handle GET requests for .ai files */
    if (r->method_number != M_GET) {
        return HTTP_METHOD_NOT_ALLOWED;
    }
    
    /* Get configuration */
    cfg = (advanced_muse_ai_config *)ap_get_module_config(r->server->module_config, &muse_ai_module);
    if (!cfg) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Failed to get configuration");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Detect language for translation */
    muse_language_selection_t *lang_selection = muse_detect_language(r, "en_US");
    
    /* Check for language errors and provide helpful error pages */
    if (lang_selection && lang_selection->is_translation_requested && !lang_selection->is_supported) {
        return generate_language_error_page(r, lang_selection);
    }
    
    if (lang_selection) {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, 
                     "[mod_muse_ai] Language detection: locale=%s, source=%s, translation_requested=%s",
                     lang_selection->selected_locale ? lang_selection->selected_locale : "NULL",
                     lang_selection->source ? lang_selection->source : "unknown",
                     lang_selection->is_translation_requested ? "yes" : "no");
        
        /* Update request URI if language prefix was detected */
        if (lang_selection->processed_uri && strcmp(r->uri, lang_selection->processed_uri) != 0) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "[mod_muse_ai] URI updated from %s to %s", r->uri, lang_selection->processed_uri);
            r->uri = apr_pstrdup(r->pool, lang_selection->processed_uri);
        }
        
        /* Set language preference cookie if translation was explicitly requested */
        if (lang_selection->is_translation_requested && lang_selection->selected_locale) {
            muse_set_language_cookie(r, lang_selection->selected_locale, 86400); /* 24 hours */
        }
    }
    
    /* Construct path to .ai file using Apache's filename translation */
    /* Apache translates URIs to filenames automatically in r->filename */
    ai_file_path = r->filename;
    
    /* If r->filename is not set, construct it manually using server document root */
    if (!ai_file_path) {
        core_server_config *core_cfg = ap_get_core_module_config(r->server->module_config);
        if (core_cfg && core_cfg->ap_document_root) {
            ai_file_path = apr_pstrcat(r->pool, core_cfg->ap_document_root, r->uri, NULL);
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Could not determine document root");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Looking for AI file: %s", ai_file_path);
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Prompts directory configured as: %s", cfg->prompts_dir ? cfg->prompts_dir : "(NULL)");
    
    /* Read the page-specific .ai file */
    page_prompt = read_file_contents(r->pool, ai_file_path);
    if (!page_prompt) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "[mod_muse_ai] Could not read AI file: %s", ai_file_path);
        return HTTP_NOT_FOUND;
    }
    
    /* Read system prompts from MuseAiPromptsDir if configured */
    if (cfg->prompts_dir && strlen(cfg->prompts_dir) > 0) {
        char *system_prompt_path = apr_pstrcat(r->pool, cfg->prompts_dir, "/system_prompt.ai", NULL);
        system_prompt = read_file_contents(r->pool, system_prompt_path);
        
        if (system_prompt) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Loaded system prompt from: %s", system_prompt_path);
            
            /* Read layout prompt */
            const char *layout_filename = cfg->prompts_minify ? "/layout.min.ai" : "/layout.ai";
            char *layout_prompt_path = apr_pstrcat(r->pool, cfg->prompts_dir, layout_filename, NULL);
            layout_prompt = read_file_contents(r->pool, layout_prompt_path);
            
            if (layout_prompt) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Loaded layout prompt from: %s", layout_prompt_path);
                final_system_prompt = apr_pstrcat(r->pool, system_prompt, "\n\n", layout_prompt, NULL);
            } else {
                final_system_prompt = system_prompt;
            }
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r, "[mod_muse_ai] Could not load system prompt from: %s", system_prompt_path);
        }
    }
    
    /* Create JSON payload with translation support */
    if (final_system_prompt) {
        /* Both system and user content */
        char *enhanced_system_prompt = final_system_prompt;
        
        /* Add translation instructions if needed */
        if (lang_selection && lang_selection->is_translation_requested && 
            lang_selection->selected_locale && lang_selection->is_supported) {
            
            const char *display_name = muse_get_locale_display_name(lang_selection->selected_locale);
            const char *tier = muse_get_locale_tier(lang_selection->selected_locale);
            
            /* Convert locale to URL-friendly format for URL prefixes */
            char url_lang_buffer[16];
            const char *url_lang_code = "en"; /* default */
            
            /* Convert es_MX -> es-mx, zh_CN -> zh-cn, etc. */
            if (lang_selection->selected_locale) {
                strncpy(url_lang_buffer, lang_selection->selected_locale, sizeof(url_lang_buffer) - 1);
                url_lang_buffer[sizeof(url_lang_buffer) - 1] = '\0';
                
                /* Convert to lowercase and replace _ with - */
                for (char *p = url_lang_buffer; *p; p++) {
                    if (*p == '_') {
                        *p = '-';
                    } else {
                        *p = tolower(*p);
                    }
                }
                url_lang_code = url_lang_buffer;
            }
            
            enhanced_system_prompt = apr_psprintf(r->pool,
                "%s\n\n"
                "**TRANSLATION INSTRUCTIONS:**\n"
                "- Translate the final output to %s (%s)\n"
                "- Translation quality tier: %s\n"
                "- Maintain the original meaning, tone, and formatting\n"
                "- Preserve any HTML tags, markdown, or special formatting\n"
                "- Use natural, fluent language appropriate for the target locale\n"
                "- If technical terms don't translate well, keep them in English with brief explanation\n"
                "\n"
                "**CRITICAL URL LOCALIZATION REQUIREMENT:**\n"
                "- MUST update navigation links in <nav> to include language prefix '/%s/'\n"
                "- REQUIRED changes: href=\"/\" becomes href=\"/%s/\", href=\"/features\" becomes href=\"/%s/features\"\n"
                "- NEVER modify: CSS links (/css/), JavaScript (/js/), images, or external URLs\n"
                "- Example: <a href=\"/\">Home</a> must become <a href=\"/%s/\">Home</a>\n"
                "- This is essential for maintaining language context during navigation",
                final_system_prompt,
                display_name ? display_name : lang_selection->selected_locale,
                lang_selection->selected_locale,
                tier ? tier : "Unknown",
                url_lang_code,
                url_lang_code,
                url_lang_code,
                url_lang_code);
                
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "[mod_muse_ai] Added translation instructions for %s", lang_selection->selected_locale);
        }
        
        char *escaped_system = escape_json_string(r->pool, enhanced_system_prompt);
        char *escaped_user = escape_json_string(r->pool, page_prompt);
        if (cfg->max_tokens > 0) {
            json_payload = apr_psprintf(r->pool, 
                "{\n"
                "  \"model\": \"%s\",\n"
                "  \"messages\": [\n"
                "    {\"role\": \"system\", \"content\": \"%s\"},\n"
                "    {\"role\": \"user\", \"content\": \"%s\"}\n"
                "  ],\n"
                "  \"max_tokens\": %d,\n"
                "  \"stream\": %s\n"
                "}",
                cfg->model ? cfg->model : "default",
                escaped_system,
                escaped_user,
                cfg->max_tokens,
                cfg->streaming ? "true" : "false");
        } else {
            json_payload = apr_psprintf(r->pool, 
                "{\n"
                "  \"model\": \"%s\",\n"
                "  \"messages\": [\n"
                "    {\"role\": \"system\", \"content\": \"%s\"},\n"
                "    {\"role\": \"user\", \"content\": \"%s\"}\n"
                "  ],\n"
                "  \"stream\": %s\n"
                "}",
                cfg->model ? cfg->model : "default",
                escaped_system,
                escaped_user,
                cfg->streaming ? "true" : "false");
        }
    } else {
        /* Only user content */
        char *escaped_user = escape_json_string(r->pool, page_prompt);
        if (cfg->max_tokens > 0) {
            json_payload = apr_psprintf(r->pool,
                "{\n"
                "  \"model\": \"%s\",\n"
                "  \"messages\": [\n"
                "    {\"role\": \"user\", \"content\": \"%s\"}\n"
                "  ],\n"
                "  \"max_tokens\": %d,\n"
                "  \"stream\": %s\n"
                "}",
                cfg->model ? cfg->model : "default",
                escaped_user,
                cfg->max_tokens,
                cfg->streaming ? "true" : "false");
        } else {
            json_payload = apr_psprintf(r->pool,
                "{\n"
                "  \"model\": \"%s\",\n"
                "  \"messages\": [\n"
                "    {\"role\": \"user\", \"content\": \"%s\"}\n"
                "  ],\n"
                "  \"stream\": %s\n"
                "}",
                cfg->model ? cfg->model : "default",
                escaped_user,
                cfg->streaming ? "true" : "false");
        }
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Generated JSON payload for AI file request");
    
    /* Create basic config structure for backend request */
    muse_ai_config basic_cfg = {
        .endpoint = cfg->endpoint,
        .timeout = cfg->timeout,
        .debug = cfg->debug,
        .model = cfg->model,
        .api_key = cfg->api_key,
        .streaming = cfg->streaming,
        .max_tokens = cfg->max_tokens
    };
    
    /* Forward to backend */
    char *response_body = NULL;
    int status = make_backend_request(r, &basic_cfg, cfg->endpoint, json_payload, &response_body);
    
    return status;
}

/* Enhanced request handler with Phase 3 features */
int enhanced_muse_ai_handler(request_rec *r)
{
    /* Check if this is a .ai file request */
    if (r->uri && strlen(r->uri) > 3 && strcmp(r->uri + strlen(r->uri) - 3, ".ai") == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] Detected .ai file request, routing to ai_file_handler");
        return ai_file_handler(r);
    }
    
    /* Check if this is an extensionless request that might have a corresponding .ai file */
    if (r->uri && strcmp(r->uri, "/ai") != 0) {
        /* Construct potential .ai file path */
        char *potential_ai_uri = apr_pstrcat(r->pool, r->uri, ".ai", NULL);
        char *potential_ai_path = NULL;
        
        /* Get document root to construct full file path */
        core_server_config *core_cfg = ap_get_core_module_config(r->server->module_config);
        if (core_cfg && core_cfg->ap_document_root) {
            potential_ai_path = apr_pstrcat(r->pool, core_cfg->ap_document_root, potential_ai_uri, NULL);
        } else {
            /* If we can't get document root, try using r->filename approach */
            if (r->filename) {
                potential_ai_path = apr_pstrcat(r->pool, r->filename, ".ai", NULL);
            }
        }
        
        /* Check if the .ai file exists */
        if (potential_ai_path) {
            apr_finfo_t finfo;
            if (apr_stat(&finfo, potential_ai_path, APR_FINFO_TYPE, r->pool) == APR_SUCCESS && 
                finfo.filetype == APR_REG) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                    "[mod_muse_ai] Found corresponding .ai file for extensionless request: %s -> %s", 
                    r->uri, potential_ai_path);
                
                /* Temporarily modify the request to point to the .ai file */
                char *original_uri = r->uri;
                char *original_filename = r->filename;
                
                r->uri = potential_ai_uri;
                r->filename = potential_ai_path;
                
                /* Call the ai_file_handler */
                int result = ai_file_handler(r);
                
                /* Restore original URI and filename */
                r->uri = original_uri;
                r->filename = original_filename;
                
                return result;
            } else {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                    "[mod_muse_ai] No corresponding .ai file found for: %s (checked: %s)", 
                    r->uri, potential_ai_path);
            }
        }
        
        /* No .ai file found, decline to handle this request */
        return DECLINED;
    }
    
    /* Handle the traditional /ai endpoint */
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
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] HANDLER CONFIG: timeout=%d, debug=%d, api_key=%s", 
                 cfg->timeout, cfg->debug, cfg->api_key ? "(set)" : "(not set)");

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
        /* Calculate dynamic buffer size for POST request reading */
        size_t read_buffer_size = cfg->max_tokens > 0 ? (cfg->max_tokens * 4) : 32768;
        if (read_buffer_size < 8192) read_buffer_size = 8192;  /* Minimum 8KB */
        if (read_buffer_size > 1048576) read_buffer_size = 1048576;  /* Maximum 1MB */
        
        if (ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ, read_buffer_size) == APR_SUCCESS) {
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
                if (cfg->max_tokens > 0) {
                    json_payload = apr_psprintf(r->pool, "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"system\", \"content\": \"%s\"},\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"max_tokens\": %d,\n  \"stream\": %s\n}",
                                               cfg->model ? cfg->model : "default", escaped_system, escaped_user, cfg->max_tokens, cfg->streaming ? "true" : "false");
                } else {
                    json_payload = apr_psprintf(r->pool, "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"system\", \"content\": \"%s\"},\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"stream\": %s\n}",
                                               cfg->model ? cfg->model : "default", escaped_system, escaped_user, cfg->streaming ? "true" : "false");
                }
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
        if (cfg->max_tokens > 0) {
            json_payload = apr_psprintf(r->pool, 
                                        "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"max_tokens\": %d,\n  \"stream\": %s\n}",
                                        cfg->model ? cfg->model : "default", 
                                        escaped_prompt, 
                                        cfg->max_tokens,
                                        cfg->streaming ? "true" : "false");
        } else {
            json_payload = apr_psprintf(r->pool, 
                                        "{\n  \"model\": \"%s\",\n  \"messages\": [\n    {\"role\": \"user\", \"content\": \"%s\"}\n  ],\n  \"stream\": %s\n}",
                                        cfg->model ? cfg->model : "default", 
                                        escaped_prompt, 
                                        cfg->streaming ? "true" : "false");
        }
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
    
    /* Create temporary basic config from advanced config */
    muse_ai_config basic_cfg = {
        .endpoint = cfg->endpoint,
        .timeout = cfg->timeout,
        .debug = cfg->debug,
        .model = cfg->model,
        .api_key = cfg->api_key,
        .streaming = cfg->streaming,
        .max_tokens = cfg->max_tokens
    };
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "[mod_muse_ai] BASIC CONFIG: timeout=%d, debug=%d", 
                     basic_cfg.timeout, basic_cfg.debug);
    }
    
    int status = make_backend_request(r, &basic_cfg, cfg->endpoint, json_payload, &response_body);
    
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
