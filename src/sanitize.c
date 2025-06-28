#include "mod_muse_ai.h"
#include <string.h>
#include <ctype.h>

/* Remove markdown code fences and cleanup content */
char *cleanup_code_fences(apr_pool_t *pool, const char *content)
{
    if (!content) return apr_pstrdup(pool, "");
    
    char *output = apr_pstrdup(pool, content);
    
    /* Step 1: Universal HTML extraction - handle AI responses with explanatory text */
    if (strstr(output, "<!DOCTYPE")) {
        /* Find the start of the HTML document */
        char *doctype_start = strstr(output, "<!DOCTYPE");
        if (doctype_start) {
            /* Find the end of the HTML document */
            char *html_end = strstr(output, "</html>");
            if (html_end) {
                /* Extract just the HTML content */
                int html_length = (html_end + strlen("</html>")) - doctype_start;
                char *clean_html = apr_palloc(pool, html_length + 1);
                strncpy(clean_html, doctype_start, html_length);
                clean_html[html_length] = '\0';
                output = clean_html;
            }
        }
    }
    
    /* Step 2: Remove markdown code fence patterns */
    /* Remove ```html, ```javascript, etc. */
    char *fence_start;
    while ((fence_start = strstr(output, "```")) != NULL) {
        char *fence_end = fence_start + 3;
        
        /* Skip language identifier if present */
        while (*fence_end && *fence_end != '\n' && *fence_end != '\r') {
            fence_end++;
        }
        
        /* Skip newline after opening fence */
        if (*fence_end == '\n' || *fence_end == '\r') {
            fence_end++;
        }
        
        /* Find closing fence */
        char *closing_fence = strstr(fence_end, "```");
        if (closing_fence) {
            /* Remove the fences and keep the content */
            int content_len = closing_fence - fence_end;
            char *before = apr_palloc(pool, fence_start - output + 1);
            strncpy(before, output, fence_start - output);
            before[fence_start - output] = '\0';
            
            char *content_part = apr_palloc(pool, content_len + 1);
            strncpy(content_part, fence_end, content_len);
            content_part[content_len] = '\0';
            
            char *after = closing_fence + 3;
            /* Skip newline after closing fence */
            if (*after == '\n' || *after == '\r') {
                after++;
            }
            
            output = apr_pstrcat(pool, before, content_part, after, NULL);
        } else {
            /* No closing fence found, remove opening fence */
            char *before = apr_palloc(pool, fence_start - output + 1);
            strncpy(before, output, fence_start - output);
            before[fence_start - output] = '\0';
            
            output = apr_pstrcat(pool, before, fence_end, NULL);
        }
    }
    
    /* Step 3: Remove standalone "html" lines that appear before DOCTYPE */
    char *html_line;
    while ((html_line = strstr(output, "\nhtml\n")) != NULL) {
        char *before = apr_palloc(pool, html_line - output + 1);
        strncpy(before, output, html_line - output);
        before[html_line - output] = '\0';
        
        char *after = html_line + strlen("\nhtml\n");
        output = apr_pstrcat(pool, before, "\n", after, NULL);
    }
    
    /* Handle html at start of string */
    if (strncmp(output, "html\n", 5) == 0) {
        output = apr_pstrdup(pool, output + 5);
    }
    
    /* Step 4: Remove inline backticks */
    char *backtick;
    while ((backtick = strchr(output, '`')) != NULL) {
        char *before = apr_palloc(pool, backtick - output + 1);
        strncpy(before, output, backtick - output);
        before[backtick - output] = '\0';
        
        char *after = backtick + 1;
        output = apr_pstrcat(pool, before, after, NULL);
    }
    
    return output;
}

/* Extract HTML content from mixed AI response */
char *extract_html_content(apr_pool_t *pool, const char *content)
{
    if (!content) return apr_pstrdup(pool, "");
    
    /* Look for HTML document boundaries */
    char *doctype_start = strstr(content, "<!DOCTYPE");
    char *html_start = strstr(content, "<html");
    
    char *start_pos = NULL;
    if (doctype_start) {
        start_pos = doctype_start;
    } else if (html_start) {
        start_pos = html_start;
    }
    
    if (!start_pos) {
        /* No HTML found, return cleaned content */
        return cleanup_code_fences(pool, content);
    }
    
    /* Find HTML end */
    char *html_end = strstr(start_pos, "</html>");
    if (html_end) {
        /* Extract HTML content */
        int html_length = (html_end + strlen("</html>")) - start_pos;
        char *html_content = apr_palloc(pool, html_length + 1);
        strncpy(html_content, start_pos, html_length);
        html_content[html_length] = '\0';
        
        return cleanup_code_fences(pool, html_content);
    }
    
    /* No closing tag found, return from start position */
    return cleanup_code_fences(pool, start_pos);
}

/* Sanitize AI response content */
char *sanitize_response(apr_pool_t *pool, const char *content)
{
    if (!content) return apr_pstrdup(pool, "");
    
    char *cleaned = cleanup_code_fences(pool, content);
    
    /* Remove any remaining explanatory text patterns */
    /* This is a simplified version - could be enhanced with more patterns */
    
    /* Remove common AI explanation patterns */
    char *patterns[] = {
        "Here's the HTML:",
        "Here is the HTML:",
        "I'll create",
        "I've created",
        "Hope you like it",
        "Let me know if you need",
        NULL
    };
    
    for (int i = 0; patterns[i] != NULL; i++) {
        char *pattern_pos = strstr(cleaned, patterns[i]);
        if (pattern_pos) {
            /* Find end of line */
            char *line_end = strchr(pattern_pos, '\n');
            if (line_end) {
                /* Remove the entire line */
                char *before = apr_palloc(pool, pattern_pos - cleaned + 1);
                strncpy(before, cleaned, pattern_pos - cleaned);
                before[pattern_pos - cleaned] = '\0';
                
                char *after = line_end + 1;
                cleaned = apr_pstrcat(pool, before, after, NULL);
            }
        }
    }
    
    return cleaned;
}
