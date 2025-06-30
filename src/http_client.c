#include "mod_muse_ai.h"
#include <string.h>
#include "advanced_streaming.h"

/* Calculate optimal buffer size based on max_tokens configuration */
static size_t calculate_buffer_size(int max_tokens) {
    size_t buffer_size;
    
    if (max_tokens <= 0) {
        /* No token limit set, use a large default buffer */
        buffer_size = 32768;  /* 32KB for unlimited responses */
    } else {
        /* Calculate buffer size: tokens * 4 characters per token (conservative estimate) */
        buffer_size = max_tokens * 4;
        
        /* Ensure minimum buffer size for functionality */
        if (buffer_size < 8192) {
            buffer_size = 8192;  /* 8KB minimum */
        }
        
        /* Cap maximum buffer size to prevent excessive memory usage */
        if (buffer_size > 1048576) {
            buffer_size = 1048576;  /* 1MB maximum */
        }
    }
    
    return buffer_size;
}

/* Parse SSE (Server-Sent Events) data chunk */
static char *parse_sse_chunk(apr_pool_t *pool, const char *chunk)
{
    if (!chunk) return NULL;
    
    /* Look for "data: " prefix anywhere in the chunk */
    const char *data_prefix = "data: ";
    char *data_line = strstr(chunk, data_prefix);
    
    if (!data_line) {
        /* Check for [DONE] marker */
        if (strstr(chunk, "[DONE]")) {
            return NULL; /* End of stream */
        }
        return NULL;
    }
    
    /* Skip the "data: " prefix */
    data_line += strlen(data_prefix);
    
    /* Find end of line */
    char *line_end = strchr(data_line, '\n');
    if (!line_end) line_end = strchr(data_line, '\r');
    
    if (line_end) {
        /* Extract the data content */
        int data_len = line_end - data_line;
        char *data_content = apr_palloc(pool, data_len + 1);
        strncpy(data_content, data_line, data_len);
        data_content[data_len] = '\0';
        
        /* Check for [DONE] marker */
        if (strcmp(data_content, "[DONE]") == 0) {
            return NULL; /* End of stream */
        }
        
        return data_content;
    }
    
    return NULL;
}

/* Extract content from JSON chunk */
static char *extract_json_content(apr_pool_t *pool, const char *json_data)
{
    if (!json_data) return NULL;
    
    /* Simple JSON parsing - look for "content":"..." pattern */
    char *content_start = strstr(json_data, "\"content\":\"");
    if (!content_start) return NULL;
    
    content_start += strlen("\"content\":\"");
    
    /* Find the closing quote, handling escaped quotes */
    char *content_end = content_start;
    while (*content_end && !(*content_end == '"' && *(content_end - 1) != '\\')) {
        content_end++;
    }
    
    if (*content_end == '"') {
        int content_len = content_end - content_start;
        char *content = apr_palloc(pool, content_len + 1);
        strncpy(content, content_start, content_len);
        content[content_len] = '\0';
        
        /* Unescape common JSON escape sequences */
        char *unescaped = apr_pstrdup(pool, content);
        
        /* Replace Unicode escape sequences */
        char *unicode_pos;
        while ((unicode_pos = strstr(unescaped, "\\u")) != NULL) {
            /* Parse the 4-digit hex code */
            if (strlen(unicode_pos) >= 6) {
                char hex_str[5];
                strncpy(hex_str, unicode_pos + 2, 4);
                hex_str[4] = '\0';
                
                /* Convert hex to integer */
                unsigned int unicode_val = (unsigned int)strtol(hex_str, NULL, 16);
                
                /* Convert common Unicode values to ASCII */
                char replacement = 0;
                if (unicode_val == 0x003c) replacement = '<';      /* \u003c -> < */
                else if (unicode_val == 0x003e) replacement = '>'; /* \u003e -> > */
                else if (unicode_val == 0x0026) replacement = '&'; /* \u0026 -> & */
                else if (unicode_val == 0x0022) replacement = '"'; /* \u0022 -> " */
                else if (unicode_val == 0x0027) replacement = '\''; /* \u0027 -> ' */
                else if (unicode_val == 0x002f) replacement = '/'; /* \u002f -> / */
                
                if (replacement) {
                    *unicode_pos = replacement;
                    memmove(unicode_pos + 1, unicode_pos + 6, strlen(unicode_pos + 6) + 1);
                } else {
                    /* Skip this Unicode sequence if we don't handle it */
                    unicode_pos += 6;
                }
            } else {
                break;
            }
        }
        
        /* Replace \\n with \n */
        char *newline_pos;
        while ((newline_pos = strstr(unescaped, "\\n")) != NULL) {
            *newline_pos = '\n';
            memmove(newline_pos + 1, newline_pos + 2, strlen(newline_pos + 2) + 1);
        }
        
        /* Replace \\\" with \" */
        char *quote_pos;
        while ((quote_pos = strstr(unescaped, "\\\"")) != NULL) {
            *quote_pos = '"';
            memmove(quote_pos + 1, quote_pos + 2, strlen(quote_pos + 2) + 1);
        }
        
        return unescaped;
    }
    
    return NULL;
}

/* Handle streaming response from backend */
static int handle_streaming_response(request_rec *r, muse_ai_config *cfg, 
                                   apr_socket_t *sock, streaming_state_t *state, 
                                   const muse_language_selection_t *lang_selection)
{
    /* Calculate dynamic buffer sizes based on max_tokens */
    size_t buffer_size = calculate_buffer_size(cfg->max_tokens);
    size_t line_buffer_size = buffer_size / 2;  /* Line buffer is half of total buffer */
    size_t accumulated_buffer_size = buffer_size;
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Dynamic buffer sizing - max_tokens: %d, buffer_size: %zu, line_buffer: %zu",
                     cfg->max_tokens, buffer_size, line_buffer_size);
    }
    
    /* Allocate dynamic buffers */
    char *line_buffer = apr_palloc(r->pool, line_buffer_size);
    char *accumulated_buffer = apr_palloc(r->pool, accumulated_buffer_size);
    int accumulated_len = 0;
    apr_size_t len;
    apr_status_t rv;
    int headers_complete = 0;
    
    /* Set proper headers for streaming response */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    apr_table_setn(r->headers_out, "Cache-Control", "no-cache");
    apr_table_setn(r->headers_out, "Connection", "keep-alive");
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Starting streaming response handling");
    }
    
    while (1) {
        len = line_buffer_size - 1;
        
        if (cfg->debug) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "mod_muse_ai: About to receive data from socket");
        }
        
        rv = apr_socket_recv(sock, line_buffer, &len);
        
        if (cfg->debug) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "mod_muse_ai: Received %lu bytes, status: %d", 
                         (unsigned long)len, rv);
        }
        
        if (rv == APR_EOF || len == 0) {
            if (cfg->debug) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                             "mod_muse_ai: End of stream (EOF or len=0)");
            }
            break;
        }
        
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                         "mod_muse_ai: Error reading streaming response");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        
        line_buffer[len] = '\0';
        
        if (cfg->debug) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "mod_muse_ai: Raw data received: '%.200s'", line_buffer);
        }
        
        /* Skip HTTP headers if not yet processed */
        char *body_start = line_buffer;
        if (!headers_complete) {
            char *header_end = strstr(line_buffer, "\r\n\r\n");
            if (header_end) {
                headers_complete = 1;
                body_start = header_end + 4;
                len = len - (body_start - line_buffer);
            } else {
                /* Headers not complete yet, continue reading */
                continue;
            }
        }
        
        /* Add to accumulated buffer */
        if (accumulated_len + len < accumulated_buffer_size - 1) {
            memcpy(accumulated_buffer + accumulated_len, body_start, len);
            accumulated_len += len;
            accumulated_buffer[accumulated_len] = '\0';
        }
        
        /* Process complete lines ending with \n */
        char *line_start = accumulated_buffer;
        char *line_end;
        
        while ((line_end = strchr(line_start, '\n')) != NULL) {
            /* Extract one line */
            int line_len = line_end - line_start;
            char *line = apr_palloc(r->pool, line_len + 1);
            strncpy(line, line_start, line_len);
            line[line_len] = '\0';
            
            /* Trim whitespace */
            char *trimmed_line = line;
            while (*trimmed_line == ' ' || *trimmed_line == '\t' || *trimmed_line == '\r') {
                trimmed_line++;
            }
            
            /* Skip empty lines */
            if (strlen(trimmed_line) == 0) {
                line_start = line_end + 1;
                continue;
            }
            
            /* Skip [DONE] messages */
            if (strstr(trimmed_line, "[DONE]")) {
                if (cfg->debug) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                 "mod_muse_ai: Received [DONE] marker");
                }
                return OK;
            }
            
            /* Process SSE data lines */
            if (strncmp(trimmed_line, "data: ", 6) == 0) {
                char *json_data = trimmed_line + 6; /* Skip "data: " prefix */
                
                if (cfg->debug) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                 "mod_muse_ai: Processing SSE line: '%.100s...'", json_data);
                }
                
                /* Extract content from JSON */
                char *content = extract_json_content(r->pool, json_data);
                if (cfg->debug) {
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                 "mod_muse_ai: Extracted content: '%s'", content ? content : "(null)");
                }
                
                if (content && strlen(content) > 0) {
                    /* Process through streaming pipeline */
                    char *processed_content = process_streaming_content(r, state, content, lang_selection);
                    
                    if (processed_content && strlen(processed_content) > 0) {
                        /* Send processed content to client */
                        ap_rputs(processed_content, r);
                        ap_rflush(r);
                        
                        if (cfg->debug) {
                            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                         "mod_muse_ai: Streamed %lu bytes: '%.50s...'", 
                                         (unsigned long)strlen(processed_content),
                                         processed_content);
                        }
                    }
                    
                    /* Check if HTML is complete */
                    if (state->html_complete) {
                        if (cfg->debug) {
                            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                         "mod_muse_ai: HTML complete, stopping stream");
                        }
                        return OK;
                    }
                }
            }
            
            /* Move to next line */
            line_start = line_end + 1;
        }
        
        /* Move remaining partial line to beginning of buffer */
        if (line_start < accumulated_buffer + accumulated_len) {
            int remaining_len = accumulated_len - (line_start - accumulated_buffer);
            memmove(accumulated_buffer, line_start, remaining_len);
            accumulated_len = remaining_len;
        } else {
            accumulated_len = 0;
        }
    }
    
    return OK;
}

/* Make HTTP POST request to backend API with streaming support */
int make_backend_request(request_rec *r, muse_ai_config *cfg, 
                        const char *backend_url, const char *json_payload,
                        char **response_body, const muse_language_selection_t *lang_selection)
{
    apr_socket_t *sock;
    apr_sockaddr_t *sa;
    apr_uri_t uri;
    apr_status_t rv;
    char *host;
    apr_port_t port;
    char *request_headers;
    
    *response_body = NULL;
    
    /* Parse the backend URL */
    ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r,
                 "mod_muse_ai: HTTP CLIENT - Received backend_url: %s", backend_url);
    
    if (apr_uri_parse(r->pool, backend_url, &uri) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "mod_muse_ai: Failed to parse backend URL: %s", backend_url);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    ap_log_rerror(APLOG_MARK, APLOG_NOTICE, 0, r,
                 "mod_muse_ai: HTTP CLIENT - Parsed uri.path: %s", uri.path ? uri.path : "(null)");
    
    host = uri.hostname ? uri.hostname : "127.0.0.1";
    port = uri.port ? uri.port : 11434;
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Connecting to %s:%d", host, port);
    }
    
    /* Create socket address */
    rv = apr_sockaddr_info_get(&sa, host, APR_INET, port, 0, r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                     "mod_muse_ai: Failed to resolve host %s:%d", host, port);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Create socket */
    rv = apr_socket_create(&sock, APR_INET, SOCK_STREAM, APR_PROTO_TCP, r->pool);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                     "mod_muse_ai: Failed to create socket");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Set socket timeout */
    apr_socket_timeout_set(sock, apr_time_from_sec(cfg->timeout));
    
    /* Connect to backend */
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: About to connect to %s:%d with timeout %ld seconds", 
                     host, port, cfg->timeout);
    }
    
    rv = apr_socket_connect(sock, sa);
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Socket connect returned with status: %d", rv);
    }
    
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                     "mod_muse_ai: Failed to connect to %s:%d", host, port);
        apr_socket_close(sock);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Build HTTP request with optional Authorization header */
    if (cfg->api_key && strlen(cfg->api_key) > 0) {
        request_headers = apr_psprintf(r->pool,
            "POST %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Authorization: Bearer %s\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            uri.path ? apr_pstrcat(r->pool, uri.path, "/chat/completions", NULL) : "/v1/chat/completions",
            host, port,
            cfg->api_key,
            (unsigned long)strlen(json_payload),
            json_payload);
    } else {
        request_headers = apr_psprintf(r->pool,
            "POST %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %lu\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            uri.path ? apr_pstrcat(r->pool, uri.path, "/chat/completions", NULL) : "/v1/chat/completions",
            host, port,
            (unsigned long)strlen(json_payload),
            json_payload);
    }
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: HTTP request built successfully, length: %lu", 
                     (unsigned long)strlen(request_headers));
    }
    
    // CRITICAL DEBUG: Show the full HTTP request being sent
    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                 "CRITICAL DEBUG: Full HTTP request headers:\n%s", request_headers);
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Sending request headers");
    }
    
    /* Send request robustly in a loop */
    apr_size_t request_len = strlen(request_headers);
    const char *p = request_headers;
    while (request_len > 0) {
        apr_size_t sent_len = request_len;
        rv = apr_socket_send(sock, p, &sent_len);
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                         "mod_muse_ai: Failed to send request");
            apr_socket_close(sock);
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        p += sent_len;
        request_len -= sent_len;
    }
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Request sent successfully, handling response");
    }
    
    /* Handle response based on streaming configuration */
    if (cfg->streaming) {
        /* Create streaming state */
        streaming_state_t *state = create_streaming_state(r->pool);
        
        /* Handle streaming response */
        int result = handle_streaming_response(r, cfg, sock, state, lang_selection);
        apr_socket_close(sock);
        return result;
    } else {
        /* Handle non-streaming response (original behavior) */
        size_t buffer_size = calculate_buffer_size(cfg->max_tokens);
        char *buffer = apr_palloc(r->pool, buffer_size);
        apr_size_t len;
        char *response = NULL;
        apr_size_t response_len = 0;
        
        /* Read complete response */
        response = apr_pcalloc(r->pool, 1);
        response_len = 0;
        
        while (1) {
            len = buffer_size - 1;
            rv = apr_socket_recv(sock, buffer, &len);
            
            if (rv == APR_EOF || len == 0) {
                break;
            }
            
            if (rv != APR_SUCCESS) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                             "mod_muse_ai: Error reading response");
                apr_socket_close(sock);
                return HTTP_INTERNAL_SERVER_ERROR;
            }
            
            buffer[len] = '\0';
            
            /* Append to response */
            char *new_response = apr_palloc(r->pool, response_len + len + 1);
            if (response_len > 0) {
                memcpy(new_response, response, response_len);
            }
            memcpy(new_response + response_len, buffer, len);
            new_response[response_len + len] = '\0';
            response = new_response;
            response_len += len;
        }
        
        apr_socket_close(sock);
        *response_body = response;
        return OK;
    }
}
