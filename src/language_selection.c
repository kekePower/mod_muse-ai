/**
 * @file language_selection.c
 * @brief Implementation of language selection and detection
 */

#include "language_selection.h"
#include "supported_locales.h"
#include "http_protocol.h"
#include "http_log.h"
#include "util_script.h"
#include <apr_strings.h>
#include <string.h>
#include <ctype.h>

/* Forward declarations */
static const char *parse_accept_language(const char *accept_lang, apr_pool_t *pool);
static bool is_valid_language_code(const char *code);
static const char *extract_cookie_value(const char *cookie_header, const char *cookie_name, apr_pool_t *pool);

muse_language_selection_t *muse_detect_language(request_rec *r, const char *fallback_locale)
{
    muse_language_selection_t *result;
    char lang_buffer[8];
    char remaining_uri[512];
    const char *detected_lang = NULL;
    const char *source = "fallback";
    bool is_translation_requested = false;
    
    if (!r || !fallback_locale) {
        return NULL;
    }
    
    /* Allocate result structure */
    result = apr_pcalloc(r->pool, sizeof(muse_language_selection_t));
    result->original_uri = r->uri;
    result->processed_uri = r->uri;
    
    /* Priority 1: URL prefix (/es/page, /fr/document) */
    if (muse_extract_language_from_url(r->uri, lang_buffer, sizeof(lang_buffer),
                                      remaining_uri, sizeof(remaining_uri))) {
        detected_lang = apr_pstrdup(r->pool, lang_buffer);
        result->processed_uri = apr_pstrdup(r->pool, remaining_uri);
        source = "url";
        is_translation_requested = true;
        
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "[language_selection] Language from URL: %s (remaining: %s)",
                     detected_lang, remaining_uri);
    }
    
    /* Priority 2: Query parameter (?lang=es, ?locale=fr_FR) */
    if (!detected_lang) {
        detected_lang = muse_get_language_from_query(r);
        if (detected_lang) {
            source = "query";
            is_translation_requested = true;
            
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "[language_selection] Language from query: %s", detected_lang);
        }
    }
    
    /* Priority 3: Cookie (language=es) */
    if (!detected_lang) {
        detected_lang = muse_get_language_from_cookie(r);
        if (detected_lang) {
            source = "cookie";
            is_translation_requested = true;
            
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "[language_selection] Language from cookie: %s", detected_lang);
        }
    }
    
    /* Priority 4: Accept-Language header */
    if (!detected_lang) {
        detected_lang = muse_get_language_from_header(r);
        if (detected_lang) {
            source = "header";
            is_translation_requested = false; // Implicit preference, not explicit request
            
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                         "[language_selection] Language from header: %s", detected_lang);
        }
    }
    
    /* Priority 5: Fallback */
    if (!detected_lang) {
        detected_lang = fallback_locale;
        source = "fallback";
        is_translation_requested = false;
        
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                     "[language_selection] Using fallback language: %s", detected_lang);
    }
    
    /* Normalize to full locale and validate */
    result->selected_locale = muse_normalize_language_to_locale(detected_lang, r->pool);
    if (!result->selected_locale) {
        /* If normalization failed, try fallback */
        result->selected_locale = muse_normalize_language_to_locale(fallback_locale, r->pool);
        source = "fallback";
        is_translation_requested = false;
    }
    
    /* Extract language code */
    if (result->selected_locale && muse_extract_language_code(result->selected_locale, lang_buffer, sizeof(lang_buffer))) {
        result->language_code = apr_pstrdup(r->pool, lang_buffer);
    } else {
        result->language_code = detected_lang;
    }
    
    /* Set final properties */
    result->source = apr_pstrdup(r->pool, source);
    result->is_translation_requested = is_translation_requested;
    result->is_supported = result->selected_locale ? muse_is_locale_supported(result->selected_locale) : false;
    
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                 "[language_selection] Final selection: locale=%s, source=%s, supported=%s, translation_requested=%s",
                 result->selected_locale ? result->selected_locale : "NULL",
                 result->source,
                 result->is_supported ? "yes" : "no",
                 result->is_translation_requested ? "yes" : "no");
    
    return result;
}

bool muse_extract_language_from_url(const char *uri, 
                                   char *language_buffer, size_t buffer_size,
                                   char *remaining_uri_buffer, size_t remaining_buffer_size)
{
    const char *start, *end;
    size_t lang_len;
    
    if (!uri || !language_buffer || !remaining_uri_buffer || buffer_size < 3 || remaining_buffer_size < 2) {
        return false;
    }
    
    /* Skip leading slash */
    if (uri[0] != '/') {
        return false;
    }
    start = uri + 1;
    
    /* Find next slash or end of string */
    end = strchr(start, '/');
    if (!end) {
        end = start + strlen(start);
    }
    
    lang_len = end - start;
    
    /* Language codes should be 2-3 characters */
    if (lang_len < 2 || lang_len > 3 || lang_len >= buffer_size) {
        return false;
    }
    
    /* Extract potential language code */
    strncpy(language_buffer, start, lang_len);
    language_buffer[lang_len] = '\0';
    
    /* Validate it's a supported language */
    if (!muse_is_locale_supported(language_buffer)) {
        return false;
    }
    
    /* Build remaining URI */
    if (*end == '\0') {
        /* No remaining path, use root */
        strcpy(remaining_uri_buffer, "/");
    } else {
        /* Copy remaining path */
        if (strlen(end) >= remaining_buffer_size) {
            return false;
        }
        strcpy(remaining_uri_buffer, end);
    }
    
    return true;
}

const char *muse_get_language_from_query(request_rec *r)
{
    const char *lang_param = NULL;
    apr_table_t *query_params;
    
    if (!r || !r->args) {
        return NULL;
    }
    
    /* Parse query parameters */
    query_params = apr_table_make(r->pool, 10);
    ap_args_to_table(r, &query_params);
    
    /* Check various parameter names */
    lang_param = apr_table_get(query_params, "lang");
    if (!lang_param) {
        lang_param = apr_table_get(query_params, "language");
    }
    if (!lang_param) {
        lang_param = apr_table_get(query_params, "locale");
    }
    
    /* Validate the parameter */
    if (lang_param && is_valid_language_code(lang_param)) {
        return apr_pstrdup(r->pool, lang_param);
    }
    
    return NULL;
}

const char *muse_get_language_from_cookie(request_rec *r)
{
    const char *cookie_header;
    const char *lang_value = NULL;
    
    if (!r) {
        return NULL;
    }
    
    cookie_header = apr_table_get(r->headers_in, "Cookie");
    if (!cookie_header) {
        return NULL;
    }
    
    /* Try different cookie names */
    lang_value = extract_cookie_value(cookie_header, "language", r->pool);
    if (!lang_value) {
        lang_value = extract_cookie_value(cookie_header, "locale", r->pool);
    }
    if (!lang_value) {
        lang_value = extract_cookie_value(cookie_header, "muse_lang", r->pool);
    }
    
    /* Validate the value */
    if (lang_value && is_valid_language_code(lang_value)) {
        return lang_value;
    }
    
    return NULL;
}

const char *muse_get_language_from_header(request_rec *r)
{
    const char *accept_lang;
    
    if (!r) {
        return NULL;
    }
    
    accept_lang = apr_table_get(r->headers_in, "Accept-Language");
    if (!accept_lang) {
        return NULL;
    }
    
    return parse_accept_language(accept_lang, r->pool);
}

void muse_set_language_cookie(request_rec *r, const char *locale, int max_age)
{
    char *cookie_value;
    
    if (!r || !locale) {
        return;
    }
    
    if (max_age > 0) {
        cookie_value = apr_psprintf(r->pool, "muse_lang=%s; Max-Age=%d; Path=/; SameSite=Lax",
                                   locale, max_age);
    } else {
        cookie_value = apr_psprintf(r->pool, "muse_lang=%s; Path=/; SameSite=Lax", locale);
    }
    
    apr_table_add(r->headers_out, "Set-Cookie", cookie_value);
    
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                 "[language_selection] Set language cookie: %s", cookie_value);
}

bool muse_uri_has_language_prefix(const char *uri)
{
    char lang_buffer[8];
    char remaining_buffer[512];
    
    return muse_extract_language_from_url(uri, lang_buffer, sizeof(lang_buffer),
                                         remaining_buffer, sizeof(remaining_buffer));
}

const char *muse_normalize_language_to_locale(const char *language_code, apr_pool_t *pool)
{
    const char *full_locale;
    
    if (!language_code || !pool) {
        return NULL;
    }
    
    /* If it's already a full locale and supported, return it */
    if (strchr(language_code, '_') && muse_is_locale_supported(language_code)) {
        return apr_pstrdup(pool, language_code);
    }
    
    /* Try to get full locale from language code */
    full_locale = muse_get_full_locale(language_code);
    if (full_locale) {
        return apr_pstrdup(pool, full_locale);
    }
    
    return NULL;
}

const char *muse_generate_language_redirect_url(request_rec *r, const char *locale, const char *original_uri)
{
    char lang_code[8];
    const char *base_uri;
    
    if (!r || !locale || !original_uri) {
        return NULL;
    }
    
    /* Extract language code from locale */
    if (!muse_extract_language_code(locale, lang_code, sizeof(lang_code))) {
        return NULL;
    }
    
    /* Ensure original URI starts with / */
    base_uri = (original_uri[0] == '/') ? original_uri : apr_pstrcat(r->pool, "/", original_uri, NULL);
    
    /* Generate URL with language prefix */
    return apr_psprintf(r->pool, "/%s%s", lang_code, base_uri);
}

/* Static helper functions */

static const char *parse_accept_language(const char *accept_lang, apr_pool_t *pool)
{
    char *lang_list, *token, *saveptr;
    char *lang_code;
    float quality;
    const char *best_lang = NULL;
    float best_quality = 0.0;
    
    if (!accept_lang || !pool) {
        return NULL;
    }
    
    /* Make a copy for tokenization */
    lang_list = apr_pstrdup(pool, accept_lang);
    
    /* Parse comma-separated language preferences */
    token = apr_strtok(lang_list, ",", &saveptr);
    while (token) {
        /* Trim whitespace */
        while (isspace(*token)) token++;
        
        /* Extract language code and quality */
        lang_code = token;
        quality = 1.0; /* Default quality */
        
        /* Look for quality value (;q=0.8) */
        char *q_pos = strstr(token, ";q=");
        if (q_pos) {
            *q_pos = '\0'; /* Terminate language code */
            quality = strtof(q_pos + 3, NULL);
        }
        
        /* Remove any trailing whitespace from language code */
        char *end = lang_code + strlen(lang_code) - 1;
        while (end > lang_code && isspace(*end)) {
            *end = '\0';
            end--;
        }
        
        /* Convert to lowercase */
        for (char *p = lang_code; *p; p++) {
            *p = tolower(*p);
        }
        
        /* Replace - with _ for locale format */
        for (char *p = lang_code; *p; p++) {
            if (*p == '-') *p = '_';
        }
        
        /* Check if this language is supported and has better quality */
        if (quality > best_quality && muse_is_locale_supported(lang_code)) {
            best_lang = apr_pstrdup(pool, lang_code);
            best_quality = quality;
        }
        
        /* Also try just the language part (before _) */
        char *underscore = strchr(lang_code, '_');
        if (underscore) {
            *underscore = '\0';
            if (quality > best_quality && muse_is_locale_supported(lang_code)) {
                const char *full_locale = muse_get_full_locale(lang_code);
                if (full_locale) {
                    best_lang = apr_pstrdup(pool, full_locale);
                    best_quality = quality;
                }
            }
        }
        
        token = apr_strtok(NULL, ",", &saveptr);
    }
    
    return best_lang;
}

static bool is_valid_language_code(const char *code)
{
    if (!code || strlen(code) < 2 || strlen(code) > 10) {
        return false;
    }
    
    /* Check if it contains only valid characters */
    for (const char *p = code; *p; p++) {
        if (!isalnum(*p) && *p != '_' && *p != '-') {
            return false;
        }
    }
    
    return true;
}

static const char *extract_cookie_value(const char *cookie_header, const char *cookie_name, apr_pool_t *pool)
{
    const char *start, *end;
    char *search_pattern;
    size_t value_len;
    
    if (!cookie_header || !cookie_name || !pool) {
        return NULL;
    }
    
    /* Create search pattern "name=" */
    search_pattern = apr_pstrcat(pool, cookie_name, "=", NULL);
    
    /* Find the cookie */
    start = strstr(cookie_header, search_pattern);
    if (!start) {
        return NULL;
    }
    
    /* Move past the name= part */
    start += strlen(search_pattern);
    
    /* Find the end of the value (semicolon or end of string) */
    end = strchr(start, ';');
    if (!end) {
        end = start + strlen(start);
    }
    
    /* Extract the value */
    value_len = end - start;
    if (value_len == 0) {
        return NULL;
    }
    
    char *value = apr_pstrndup(pool, start, value_len);
    
    /* Trim whitespace */
    while (isspace(*value)) value++;
    char *value_end = value + strlen(value) - 1;
    while (value_end > value && isspace(*value_end)) {
        *value_end = '\0';
        value_end--;
    }
    
    return strlen(value) > 0 ? value : NULL;
}
