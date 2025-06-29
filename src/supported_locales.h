/**
 * @file supported_locales.h
 * @brief Internal module for managing supported translation locales
 * 
 * This module provides functions to validate locale codes and retrieve
 * the list of supported locales for AI translation services.
 * 
 * The locale data is generated from docs/locale-identifiers.csv and
 * embedded as static arrays in the compiled module.
 */

#ifndef SUPPORTED_LOCALES_H
#define SUPPORTED_LOCALES_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Structure representing a supported locale
 */
typedef struct {
    const char *code;        /**< Locale code (e.g., "en_US", "es_ES") */
    const char *language;    /**< Language name (e.g., "English", "Spanish") */
    const char *country;     /**< Country/region name (e.g., "United States", "Spain") */
    const char *tier;        /**< Proficiency tier (e.g., "Tier 1 (High)", "Tier 2 (Good)") */
    const char *notes;       /**< Translation performance notes */
} muse_locale_t;

/**
 * @brief Check if a locale code is supported for translation
 * 
 * @param locale_code The locale code to check (e.g., "en_US", "es", "fr_FR")
 * @return true if the locale is supported, false otherwise
 * 
 * This function supports both full locale codes (e.g., "en_US") and
 * language-only codes (e.g., "en"). For language-only codes, it will
 * return true if any variant of that language is supported.
 */
bool muse_is_locale_supported(const char *locale_code);

/**
 * @brief Get the full locale code for a language
 * 
 * @param language_code The language code (e.g., "en", "es", "fr")
 * @return The full locale code if found, or NULL if not supported
 * 
 * For language-only codes, this returns the first matching full locale.
 * For example, "en" might return "en_US", "es" might return "es_ES".
 */
const char *muse_get_full_locale(const char *language_code);

/**
 * @brief Get the array of all supported locales
 * 
 * @param count Pointer to store the number of locales (output parameter)
 * @return Pointer to the array of supported locales
 * 
 * The returned array is statically allocated and should not be freed.
 */
const muse_locale_t *muse_get_supported_locales(size_t *count);

/**
 * @brief Get a human-readable name for a locale
 * 
 * @param locale_code The locale code to look up
 * @return A string like "English (United States)" or NULL if not found
 * 
 * The returned string is statically allocated and should not be freed.
 */
const char *muse_get_locale_display_name(const char *locale_code);

/**
 * @brief Extract language code from a full locale
 * 
 * @param locale_code Full locale code (e.g., "en_US", "es_ES")
 * @param buffer Buffer to store the language code (should be at least 8 chars)
 * @param buffer_size Size of the buffer
 * @return true if successful, false if buffer too small or invalid input
 * 
 * Examples: "en_US" -> "en", "es_ES" -> "es", "zh_CN" -> "zh"
 */
bool muse_extract_language_code(const char *locale_code, char *buffer, size_t buffer_size);

/**
 * @brief Get the proficiency tier for a locale
 * 
 * @param locale_code The locale code to look up
 * @return The tier string (e.g., "Tier 1 (High)") or NULL if not found
 * 
 * The returned string is statically allocated and should not be freed.
 */
const char *muse_get_locale_tier(const char *locale_code);

#endif /* SUPPORTED_LOCALES_H */
