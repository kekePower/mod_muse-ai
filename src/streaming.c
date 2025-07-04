#include "mod_muse_ai.h"
#include <string.h>
#include <ctype.h>
#include <strings.h>  /* for strncasecmp */

/* Create streaming state structure */
streaming_state_t *create_streaming_state(apr_pool_t *pool)
{
    streaming_state_t *state = apr_pcalloc(pool, sizeof(streaming_state_t));
    state->streaming_started = 0;
    state->last_sent_length = 0;
    state->pending_buffer = apr_pstrdup(pool, "");
    state->html_complete = 0;
    state->buffer_start_time = apr_time_now(); /* Start timing when state is created */
    return state;
}

/* Reset streaming state for new request */
void reset_streaming_state(streaming_state_t *state)
{
    state->streaming_started = 0;
    state->last_sent_length = 0;
    state->html_complete = 0;
    /* Note: pending_buffer will be reset by caller if needed */
}

/* Find HTML document start position */
int find_html_start(const char *content)
{
    if (!content) return -1;
    
    /* Simple case-insensitive search without dynamic allocation */
    const char *pos = content;
    int len = strlen(content);
    
    /* Look for DOCTYPE (case insensitive) */
    for (int i = 0; i <= len - 9; i++) {
        if (strncasecmp(pos + i, "<!doctype", 9) == 0) {
            return i;
        }
    }
    
    /* Look for <html tag (case insensitive) */
    for (int i = 0; i <= len - 5; i++) {
        if (strncasecmp(pos + i, "<html", 5) == 0) {
            return i;
        }
    }
    
    return -1;
}

/* Find HTML document end position */
int find_html_end(const char *content)
{
    if (!content) return -1;
    
    /* Simple case-insensitive search without dynamic allocation */
    const char *pos = content;
    int len = strlen(content);
    
    /* Look for </html> tag (case insensitive) */
    for (int i = 0; i <= len - 7; i++) {
        if (strncasecmp(pos + i, "</html>", 7) == 0) {
            return i;
        }
    }
    
    return -1;
}

/* Process streaming content using MuseWeb's smart streaming approach */
char *process_streaming_content(request_rec *r, streaming_state_t *state, 
                               const char *new_content, 
                               const muse_language_selection_t *lang_selection)
{
    if (!new_content || !state) {
        return apr_pstrdup(r->pool, "");
    }
    
    /* Add new content to pending buffer */
    state->pending_buffer = apr_pstrcat(r->pool, state->pending_buffer, new_content, NULL);
    
    /* Phase 1: Look for HTML start if we haven't started streaming yet */
    if (!state->streaming_started) {
        /* Disable HTML start detection - rely on time-based buffering for proper sanitization */
        /* int html_start_pos = find_html_start(state->pending_buffer); */
        /* This is commented out to ensure we buffer content for proper sanitization */
        
        /* Time-based buffering - wait 2 seconds to accumulate content for proper sanitization */
        apr_time_t current_time = apr_time_now();
        apr_time_t elapsed = current_time - state->buffer_start_time;
        apr_time_t buffer_duration = apr_time_from_sec(2); /* 2 seconds */
        
        int buffer_len = strlen(state->pending_buffer);
        
        /* Start streaming if we've buffered for enough time OR if we have substantial content */
        if (elapsed >= buffer_duration || buffer_len > 1000) {
            /* Apply comprehensive sanitization to the buffered content before streaming */
            char *fully_sanitized = sanitize_response(r->pool, state->pending_buffer, lang_selection);
            state->pending_buffer = fully_sanitized;
            
            /* Start streaming the sanitized content */
            state->streaming_started = 1;
            state->last_sent_length = strlen(state->pending_buffer);
            return apr_pstrdup(r->pool, state->pending_buffer);
        }
        
        /* Keep buffering - we might get HTML start or more content */
        return apr_pstrdup(r->pool, "");
    }
    
    /* Phase 2: We're streaming - check if HTML is complete */
    int html_end_pos = find_html_end(state->pending_buffer);
    
    if (html_end_pos == -1) {
        /* HTML not complete yet - stream new content */
        int buffer_len = strlen(state->pending_buffer);
        
        if (buffer_len > state->last_sent_length) {
            const char *new_portion = state->pending_buffer + state->last_sent_length;
            
            /* Apply sanitization to new portion to handle markdown artifacts in streaming */
            char *sanitized_portion = cleanup_code_fences(r->pool, new_portion);
            
            state->last_sent_length = buffer_len;
            return sanitized_portion;
        }
        
        /* No new content to send */
        return apr_pstrdup(r->pool, "");
    } else {
        /* Phase 3: Found </html>! Send final portion and stop streaming */
        int html_end_full = html_end_pos + strlen("</html>");
        
        char *final_content = "";
        if (html_end_full > state->last_sent_length) {
            /* Extract final portion up to and including </html> */
            int final_len = html_end_full - state->last_sent_length;
            char *raw_final = apr_palloc(r->pool, final_len + 1);
            strncpy(raw_final, state->pending_buffer + state->last_sent_length, final_len);
            raw_final[final_len] = '\0';
            
            /* Apply sanitization to final portion */
            final_content = cleanup_code_fences(r->pool, raw_final);
        }
        
        /* Mark HTML as complete */
        state->html_complete = 1;
        
        /* Everything after </html> goes to /dev/null (discarded) */
        return final_content;
    }
}
