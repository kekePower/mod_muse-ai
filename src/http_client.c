#include "mod_muse_ai.h"
#include <string.h>

/* Parse SSE (Server-Sent Events) data chunk */
static char *parse_sse_chunk(apr_pool_t *pool, const char *chunk)
{
    if (!chunk) return NULL;
    
    /* Look for "data: " prefix */
    const char *data_prefix = "data: ";
    char *data_line = strstr(chunk, data_prefix);
    
    if (!data_line) return NULL;
    
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
                                   apr_socket_t *sock, streaming_state_t *state)
{
    char buffer[4096];
    apr_size_t len;
    apr_status_t rv;
    int headers_complete = 0;
    
    /* Set content type for streaming */
    ap_set_content_type(r, "text/html;charset=UTF-8");
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Starting streaming response handling");
    }
    
    while (1) {
        len = sizeof(buffer) - 1;
        rv = apr_socket_recv(sock, buffer, &len);
        
        if (rv == APR_EOF || len == 0) {
            break;
        }
        
        if (rv != APR_SUCCESS) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                         "mod_muse_ai: Error reading streaming response");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        
        buffer[len] = '\0';
        
        /* Skip HTTP headers if not yet processed */
        char *body_start = buffer;
        if (!headers_complete) {
            char *header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {
                headers_complete = 1;
                body_start = header_end + 4;
            } else {
                /* Headers not complete yet, continue reading */
                continue;
            }
        }
        
        /* Process each SSE chunk */
        char *chunk_start = body_start;
        char *chunk_end;
        
        while ((chunk_end = strstr(chunk_start, "\n\n")) != NULL) {
            /* Extract one SSE chunk */
            int chunk_len = chunk_end - chunk_start;
            char *sse_chunk = apr_palloc(r->pool, chunk_len + 1);
            strncpy(sse_chunk, chunk_start, chunk_len);
            sse_chunk[chunk_len] = '\0';
            
            /* Parse SSE data */
            char *json_data = parse_sse_chunk(r->pool, sse_chunk);
            if (json_data) {
                /* Extract content from JSON */
                char *content = extract_json_content(r->pool, json_data);
                if (content && strlen(content) > 0) {
                    /* Process through streaming pipeline */
                    char *processed_content = process_streaming_content(r, state, content);
                    
                    if (processed_content && strlen(processed_content) > 0) {
                        /* Send processed content to client */
                        ap_rputs(processed_content, r);
                        ap_rflush(r);
                        
                        if (cfg->debug) {
                            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                         "mod_muse_ai: Streamed %lu bytes", 
                                         (unsigned long)strlen(processed_content));
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
            } else {
                /* Check for [DONE] marker */
                if (strstr(sse_chunk, "[DONE]")) {
                    if (cfg->debug) {
                        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                                     "mod_muse_ai: Received [DONE] marker");
                    }
                    return OK;
                }
            }
            
            /* Move to next chunk */
            chunk_start = chunk_end + 2;
        }
    }
    
    return OK;
}

/* Make HTTP POST request to backend API with streaming support */
int make_backend_request(request_rec *r, muse_ai_config *cfg, 
                        const char *backend_url, const char *json_payload,
                        char **response_body)
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
    if (apr_uri_parse(r->pool, backend_url, &uri) != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                     "mod_muse_ai: Failed to parse backend URL: %s", backend_url);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
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
    rv = apr_socket_connect(sock, sa);
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
            uri.path ? uri.path : "/v1/chat/completions",
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
            uri.path ? uri.path : "/v1/chat/completions",
            host, port,
            (unsigned long)strlen(json_payload),
            json_payload);
    }
    
    if (cfg->debug) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "mod_muse_ai: Sending request headers");
    }
    
    /* Send request */
    apr_size_t send_len = strlen(request_headers);
    rv = apr_socket_send(sock, request_headers, &send_len);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r,
                     "mod_muse_ai: Failed to send request");
        apr_socket_close(sock);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    /* Handle response based on streaming configuration */
    if (cfg->streaming) {
        /* Create streaming state */
        streaming_state_t *state = create_streaming_state(r->pool);
        
        /* Handle streaming response */
        int result = handle_streaming_response(r, cfg, sock, state);
        apr_socket_close(sock);
        return result;
    } else {
        /* Handle non-streaming response (original behavior) */
        char buffer[8192];
        apr_size_t len;
        char *response = NULL;
        apr_size_t response_len = 0;
        
        /* Read complete response */
        response = apr_pcalloc(r->pool, 1);
        response_len = 0;
        
        while (1) {
            len = sizeof(buffer) - 1;
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
