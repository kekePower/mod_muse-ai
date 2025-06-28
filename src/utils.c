#include "mod_muse_ai.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>  /* for sprintf */

/* URL decode function */
char *url_decode(apr_pool_t *pool, const char *encoded)
{
    if (!encoded) return apr_pstrdup(pool, "");
    
    int len = strlen(encoded);
    char *decoded = apr_palloc(pool, len + 1);
    int i, j = 0;
    
    for (i = 0; i < len; i++) {
        if (encoded[i] == '%' && i + 2 < len) {
            /* Decode %XX hex sequence */
            char hex[3] = {encoded[i+1], encoded[i+2], '\0'};
            char *endptr;
            long val = strtol(hex, &endptr, 16);
            
            if (*endptr == '\0') {
                decoded[j++] = (char)val;
                i += 2; /* Skip the hex digits */
            } else {
                decoded[j++] = encoded[i];
            }
        } else if (encoded[i] == '+') {
            /* Replace + with space */
            decoded[j++] = ' ';
        } else {
            decoded[j++] = encoded[i];
        }
    }
    
    decoded[j] = '\0';
    return decoded;
}

/* Escape JSON string */
char *escape_json_string(apr_pool_t *pool, const char *str)
{
    if (!str) return apr_pstrdup(pool, "");
    
    int len = strlen(str);
    /* Worst case: every character needs escaping */
    char *escaped = apr_palloc(pool, len * 2 + 1);
    int i, j = 0;
    
    for (i = 0; i < len; i++) {
        switch (str[i]) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            case '\b':
                escaped[j++] = '\\';
                escaped[j++] = 'b';
                break;
            case '\f':
                escaped[j++] = '\\';
                escaped[j++] = 'f';
                break;
            default:
                if (str[i] < 32) {
                    /* Escape other control characters */
                    j += sprintf(escaped + j, "\\u%04x", (unsigned char)str[i]);
                } else {
                    escaped[j++] = str[i];
                }
                break;
        }
    }
    
    escaped[j] = '\0';
    return escaped;
}
