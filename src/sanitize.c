#include "mod_muse_ai.h"
#include <string.h>
#include <ctype.h>

/* Helper function to replace all occurrences of a substring */
static char *str_replace_all(apr_pool_t *pool, const char *str, const char *old, const char *new)
{
    if (!str || !old) return apr_pstrdup(pool, str ? str : "");
    
    const char *pos = str;
    char *result = apr_pstrdup(pool, "");
    
    while ((pos = strstr(pos, old)) != NULL) {
        /* Add everything before the match */
        int before_len = pos - str;
        char *before = apr_palloc(pool, before_len + 1);
        strncpy(before, str, before_len);
        before[before_len] = '\0';
        
        result = apr_pstrcat(pool, result, before, new, NULL);
        
        /* Move past the old substring */
        str = pos + strlen(old);
        pos = str;
    }
    
    /* Add any remaining part */
    result = apr_pstrcat(pool, result, str, NULL);
    return result;
}

/* Helper function to trim whitespace from both ends */
static char *trim_whitespace(apr_pool_t *pool, const char *str)
{
    if (!str) return apr_pstrdup(pool, "");
    
    /* Find start of non-whitespace */
    const char *start = str;
    while (*start && isspace(*start)) {
        start++;
    }
    
    /* Find end of non-whitespace */
    const char *end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) {
        end--;
    }
    
    /* Extract trimmed string */
    int len = end - start + 1;
    char *trimmed = apr_palloc(pool, len + 1);
    strncpy(trimmed, start, len);
    trimmed[len] = '\0';
    
    return trimmed;
}

/* Helper function to check if string ends with suffix */
static int str_ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix) return 0;
    
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) return 0;
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

/* Custom case-insensitive string search for portability */
static char *str_case_str(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    
    int needle_len = strlen(needle);
    if (needle_len == 0) return (char *)haystack;
    
    for (const char *p = haystack; *p; p++) {
        if (strncasecmp(p, needle, needle_len) == 0) {
            return (char *)p;
        }
    }
    
    return NULL;
}

/* Remove thinking tags and their content (for reasoning models) */
static char *remove_thinking_tags(apr_pool_t *pool, const char *content)
{
    if (!content) return apr_pstrdup(pool, "");
    
    char *output = apr_pstrdup(pool, content);
    
    /* Remove <think>...</think> tags (case insensitive) */
    char *think_start;
    while ((think_start = str_case_str(output, "<think>")) != NULL) {
        char *think_end = str_case_str(think_start, "</think>");
        if (think_end) {
            /* Remove everything from <think> to </think> */
            char *before = apr_palloc(pool, think_start - output + 1);
            strncpy(before, output, think_start - output);
            before[think_start - output] = '\0';
            
            char *after = think_end + strlen("</think>");
            output = apr_pstrcat(pool, before, after, NULL);
        } else {
            /* No closing tag, remove from <think> to end */
            char *before = apr_palloc(pool, think_start - output + 1);
            strncpy(before, output, think_start - output);
            before[think_start - output] = '\0';
            output = before;
            break;
        }
    }
    
    /* Remove plain text thinking tags (Qwen3 style) */
    char *plain_think_start;
    while ((plain_think_start = str_case_str(output, "think")) != NULL) {
        /* Check if this is a standalone "think" word */
        int is_standalone = 1;
        if (plain_think_start > output) {
            char prev_char = *(plain_think_start - 1);
            if (isalnum(prev_char)) is_standalone = 0;
        }
        
        char *after_think = plain_think_start + 5; /* strlen("think") */
        if (*after_think && isalnum(*after_think)) is_standalone = 0;
        
        if (is_standalone) {
            char *plain_think_end = str_case_str(after_think, "/think");
            if (plain_think_end) {
                /* Remove everything from think to /think */
                char *before = apr_palloc(pool, plain_think_start - output + 1);
                strncpy(before, output, plain_think_start - output);
                before[plain_think_start - output] = '\0';
                
                char *after = plain_think_end + strlen("/think");
                output = apr_pstrcat(pool, before, after, NULL);
            } else {
                /* No closing tag, skip this occurrence */
                plain_think_start = after_think;
                continue;
            }
        } else {
            /* Not a standalone think, skip */
            plain_think_start = after_think;
            continue;
        }
    }
    
    return output;
}

/* Enhanced code fence cleanup using MuseWeb's proven approach */
char *cleanup_code_fences(apr_pool_t *pool, const char *content)
{
    if (!content) return apr_pstrdup(pool, "");
    
    char *output = apr_pstrdup(pool, content);
    
    /* Step 0: Universal HTML extraction - handle AI responses with explanatory text */
    /* This ensures we extract clean HTML regardless of prompt instructions or backticks */
    if (strstr(output, "<!DOCTYPE")) {
        /* Find the start of the HTML document */
        char *doctype_pos = strstr(output, "<!DOCTYPE");
        if (doctype_pos > output) {
            /* Remove everything before DOCTYPE (explanatory text, etc.) */
            output = apr_pstrdup(pool, doctype_pos);
        }
        
        /* Find the end of the HTML document */
        char *html_end_pos = strstr(output, "</html>");
        if (html_end_pos) {
            /* Remove everything after </html> */
            int html_end_full = (html_end_pos - output) + strlen("</html>");
            char *clean_html = apr_palloc(pool, html_end_full + 1);
            strncpy(clean_html, output, html_end_full);
            clean_html[html_end_full] = '\0';
            output = clean_html;
        }
    } else if (strstr(output, "<html")) {
        /* Handle HTML without DOCTYPE */
        char *html_start_pos = strstr(output, "<html");
        if (html_start_pos > output) {
            /* Remove everything before <html */
            output = apr_pstrdup(pool, html_start_pos);
        }
        
        /* Find the end of the HTML document */
        char *html_end_pos = strstr(output, "</html>");
        if (html_end_pos) {
            /* Remove everything after </html> */
            int html_end_full = (html_end_pos - output) + strlen("</html>");
            char *clean_html = apr_palloc(pool, html_end_full + 1);
            strncpy(clean_html, output, html_end_full);
            clean_html[html_end_full] = '\0';
            output = clean_html;
        }
    }
    
    /* Early return if no backticks present - most common case for clean HTML */
    if (!strchr(output, '`')) {
        return output;
    }
    
    /* Step 1: Remove common code fence patterns with direct string operations (fastest) */
    /* Enhanced to handle various AI output formats from different prompt sets */
    output = str_replace_all(pool, output, "```html\n", "");
    output = str_replace_all(pool, output, "```HTML\n", "");
    output = str_replace_all(pool, output, "```html", "");
    output = str_replace_all(pool, output, "```HTML", "");
    /* Handle other common fence variations */
    output = str_replace_all(pool, output, "```xml\n", "");
    output = str_replace_all(pool, output, "```xml", "");
    output = str_replace_all(pool, output, "```markup\n", "");
    output = str_replace_all(pool, output, "```markup", "");
    /* Handle generic fences */
    output = str_replace_all(pool, output, "```\n", "");
    output = str_replace_all(pool, output, "```", "");
    
    /* Step 2: Handle orphaned "html" at the very beginning */
    /* This is the most common leftover from ```html removal */
    /* Be very precise to avoid removing legitimate HTML content */
    char *first_newline = strchr(output, '\n');
    if (first_newline) {
        char *first_line = apr_palloc(pool, (first_newline - output) + 1);
        strncpy(first_line, output, first_newline - output);
        first_line[first_newline - output] = '\0';
        
        /* Trim whitespace from first line */
        char *trimmed = trim_whitespace(pool, first_line);
        
        /* Only remove if the first line is EXACTLY "html" or "HTML" and nothing else */
        if (strcmp(trimmed, "html") == 0 || strcmp(trimmed, "HTML") == 0) {
            /* Remove the first line containing only "html" */
            output = apr_pstrdup(pool, first_newline + 1);
        }
    }
    
    /* Step 3: Handle inline code backticks (preserve content, remove backticks) */
    /* Only process single backticks that don't contain HTML-like content */
    if (strchr(output, '`') && !strstr(output, "```")) {
        /* Simple approach: remove all remaining single backticks */
        output = str_replace_all(pool, output, "`", "");
    }
    
    /* Step 4: Handle trailing backticks at the very end (common in streaming) */
    output = trim_whitespace(pool, output);
    if (str_ends_with(output, "```")) {
        int len = strlen(output);
        char *trimmed = apr_palloc(pool, len - 2);
        strncpy(trimmed, output, len - 3);
        trimmed[len - 3] = '\0';
        output = trim_whitespace(pool, trimmed);
    } else if (str_ends_with(output, "`")) {
        int len = strlen(output);
        char *trimmed = apr_palloc(pool, len);
        strncpy(trimmed, output, len - 1);
        trimmed[len - 1] = '\0';
        output = trim_whitespace(pool, trimmed);
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

/* Enhanced sanitize response using MuseWeb's proven approach */
char *sanitize_response(apr_pool_t *pool, const char *content, const muse_language_selection_t *lang_selection)
{
    if (!content) return apr_pstrdup(pool, "");
    
    /* Step 1: Remove thinking tags first (for reasoning models) */
    char *cleaned = remove_thinking_tags(pool, content);
    
    /* Step 2: Clean up code fences and markdown artifacts */
    cleaned = cleanup_code_fences(pool, cleaned);
    
    /* Step 3: Handle orphaned "html" text that appears alone on a line */
    /* Be very specific to avoid removing legitimate HTML content */
    char *first_newline = strchr(cleaned, '\n');
    if (first_newline) {
        char *first_line = apr_palloc(pool, (first_newline - cleaned) + 1);
        strncpy(first_line, cleaned, first_newline - cleaned);
        first_line[first_newline - cleaned] = '\0';
        
        char *trimmed_first = trim_whitespace(pool, first_line);
        /* Only remove if the first line is EXACTLY "html" or "HTML" and nothing else */
        if (strcmp(trimmed_first, "html") == 0 || strcmp(trimmed_first, "HTML") == 0) {
            /* Remove the first line containing only "html" */
            cleaned = apr_pstrdup(pool, first_newline + 1);
        }
    }
    
    /* Step 4: Fix common DOCTYPE issues where the opening < got removed */
    cleaned = trim_whitespace(pool, cleaned);
    if (strncmp(cleaned, "!DOCTYPE", 8) == 0) {
        cleaned = apr_pstrcat(pool, "<", cleaned, NULL);
    } else if (strncmp(cleaned, "html", 4) == 0) {
        /* Only add < if this looks like a legitimate HTML tag (contains attributes or >) */
        if (strchr(cleaned, '>') || strchr(cleaned, ' ')) {
            cleaned = apr_pstrcat(pool, "<", cleaned, NULL);
        }
    }
    
    /* Step 5: Ensure we have a complete HTML document if the content appears to be HTML */
    if (strstr(cleaned, "<html") && !strstr(cleaned, "<!DOCTYPE html>")) {
        /* Add DOCTYPE if missing */
        if (strncmp(cleaned, "<!", 2) != 0) {
            cleaned = apr_pstrcat(pool, "<!DOCTYPE html>\n", cleaned, NULL);
        }
    }
    
    /* Step 6: Remove common AI explanation patterns */
    char *patterns[] = {
        "Here's the HTML:",
        "Here is the HTML:",
        "I'll create",
        "I've created",
        "Hope you like it",
        "Let me know if you need",
        "Here's a",
        "Here is a",
        "I've generated",
        "I'll generate",
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
    
    /* Step 7: Final cleanup - remove excessive whitespace */
    /* Replace multiple consecutive newlines with maximum of 2 newlines */
    if (strstr(cleaned, "\n\n\n")) {
        cleaned = str_replace_all(pool, cleaned, "\n\n\n\n", "\n\n");
        cleaned = str_replace_all(pool, cleaned, "\n\n\n", "\n\n");
    }
    
    /* Step 8: Add dir="rtl" for RTL languages */
    if (lang_selection && lang_selection->is_rtl) {
        char *html_tag = str_case_str(cleaned, "<html");
        if (html_tag) {
            char *tag_end = strchr(html_tag, '>');
            if (tag_end) {
                char *existing_dir = str_case_str(html_tag, "dir=");
                if (!existing_dir || existing_dir > tag_end) {
                    /* No dir attribute, add it */
                    char *before = apr_pstrndup(pool, cleaned, html_tag - cleaned + 5); // up to "<html"
                    char *after = apr_pstrdup(pool, html_tag + 5);
                    cleaned = apr_pstrcat(pool, before, " dir=\"rtl\"", after, NULL);
                }
            }
        }
    }

    /* Step 9: Final trim */
    cleaned = trim_whitespace(pool, cleaned);
    
    return cleaned;
}
