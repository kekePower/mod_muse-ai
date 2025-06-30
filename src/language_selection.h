/**
 * @file language_selection.h
 * @brief Language selection and detection for AI translation
 * 
 * This module handles language selection from various sources:
 * - URL prefixes (/es/page, /fr/document)
 * - Query parameters (?lang=es, ?locale=fr_FR)
 * - Cookies (language preference)
 * - Accept-Language headers
 * - Fallback to original file language
 */

#ifndef LANGUAGE_SELECTION_H
#define LANGUAGE_SELECTION_H

#include "httpd.h"
#include "http_config.h"
#include <stdbool.h>

/**
 * @brief Structure containing language selection result
 */
typedef struct {
    const char *selected_locale;     /**< Final selected locale (e.g., "es_ES") */
    const char *language_code;       /**< Language code only (e.g., "es") */
    const char *original_uri;        /**< Original URI before language processing */
    const char *processed_uri;       /**< URI with language prefix removed */
    const char *source;              /**< Source of language selection (url/query/cookie/header/fallback) */
    bool is_translation_requested;   /**< Whether translation was explicitly requested */
    bool is_supported;               /**< Whether the selected locale is supported */
    bool is_rtl;                     /**< Whether the selected language is right-to-left */
} muse_language_selection_t;

/**
 * @brief Detect and select language from request
 * 
 * @param r Apache request record
 * @param fallback_locale Default locale if no preference found
 * @return Language selection result structure
 * 
 * This function implements the priority order:
 * 1. URL prefix (/es/page -> es)
 * 2. Query parameter (?lang=es)
 * 3. Cookie (language=es)
 * 4. Accept-Language header
 * 5. Fallback locale
 */
muse_language_selection_t *muse_detect_language(request_rec *r, const char *fallback_locale);

/**
 * @brief Extract language code from URL prefix
 * 
 * @param uri The request URI (e.g., "/es/page.ai", "/fr/doc/file.ai")
 * @param language_buffer Buffer to store extracted language (min 8 chars)
 * @param buffer_size Size of the language buffer
 * @param remaining_uri_buffer Buffer to store remaining URI (min 256 chars)
 * @param remaining_buffer_size Size of the remaining URI buffer
 * @return true if language prefix found and extracted, false otherwise
 * 
 * Examples:
 * - "/es/page.ai" -> language="es", remaining="/page.ai"
 * - "/fr/docs/file.ai" -> language="fr", remaining="/docs/file.ai"
 * - "/page.ai" -> false (no language prefix)
 */
bool muse_extract_language_from_url(const char *uri, 
                                   char *language_buffer, size_t buffer_size,
                                   char *remaining_uri_buffer, size_t remaining_buffer_size);

/**
 * @brief Get language from query parameters
 * 
 * @param r Apache request record
 * @return Language code from query params or NULL if not found
 * 
 * Checks for: ?lang=es, ?locale=es_ES, ?language=es
 */
const char *muse_get_language_from_query(request_rec *r);

/**
 * @brief Get language from cookies
 * 
 * @param r Apache request record
 * @return Language code from cookies or NULL if not found
 * 
 * Checks for cookies: language, locale, muse_lang
 */
const char *muse_get_language_from_cookie(request_rec *r);

/**
 * @brief Get language from Accept-Language header
 * 
 * @param r Apache request record
 * @return Best matching supported language or NULL if none found
 * 
 * Parses Accept-Language header and finds the best supported locale
 */
const char *muse_get_language_from_header(request_rec *r);

/**
 * @brief Set language preference cookie
 * 
 * @param r Apache request record
 * @param locale Locale to set in cookie
 * @param max_age Cookie max age in seconds (0 for session cookie)
 * 
 * Sets a cookie to remember user's language preference
 */
void muse_set_language_cookie(request_rec *r, const char *locale, int max_age);

/**
 * @brief Check if URI has a language prefix
 * 
 * @param uri The URI to check
 * @return true if URI starts with a supported language code
 * 
 * Examples: "/es/page" -> true, "/fr/doc" -> true, "/page" -> false
 */
bool muse_uri_has_language_prefix(const char *uri);

/**
 * @brief Normalize language code to full locale
 * 
 * @param language_code Input language code (e.g., "es", "en", "fr")
 * @param pool Apache memory pool for allocation
 * @return Full locale code (e.g., "es_ES", "en_US", "fr_FR") or NULL if unsupported
 */
const char *muse_normalize_language_to_locale(const char *language_code, apr_pool_t *pool);

/**
 * @brief Generate redirect URL with language prefix
 * 
 * @param r Apache request record
 * @param locale Target locale for redirect
 * @param original_uri Original URI without language prefix
 * @return Full redirect URL with language prefix
 * 
 * Example: locale="es_ES", original_uri="/page.ai" -> "/es/page.ai"
 */
const char *muse_generate_language_redirect_url(request_rec *r, const char *locale, const char *original_uri);

#endif /* LANGUAGE_SELECTION_H */
