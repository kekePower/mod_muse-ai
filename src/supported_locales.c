/**
 * @file supported_locales.c
 * @brief Implementation of supported locales module
 * 
 * This file is auto-generated from docs/locale-identifiers.csv
 * Do not edit manually - run scripts/generate_locales.py to regenerate
 * 
 * Generated on: 2025-06-29 20:02:28
 * Total locales: 46
 */

#include "supported_locales.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/**
 * @brief Static array of all supported locales
 */
static const muse_locale_t supported_locales[] = {
    {"ar_SA", "Arabic", "Saudi Arabia", "Tier 2 (Good)", "Good and Reliable. Works well for Modern Standard Arabic (MSA)."},
    {"bg_BG", "Bulgarian", "Bulgaria", "Tier 3 (Functional)", "Functional. Best for understanding the gist or for basic communication."},
    {"bn_BD", "Bengali", "Bangladesh", "Tier 2 (Good)", "Good and Reliable. Suitable for most professional and personal use."},
    {"ca_ES", "Catalan", "Spain", "Tier 3 (Functional)", "Functional. Works well, especially when translating to/from Spanish."},
    {"cs_CZ", "Czech", "Czech Republic", "Tier 2 (Good)", "Good and Reliable. Consistent performance for most content."},
    {"da_DK", "Danish", "Denmark", "Tier 2 (Good)", "Good and Reliable. Solid choice for general-purpose translation."},
    {"de_DE", "German", "Germany", "Tier 1 (High)", "Excellent. Highly accurate and natural-sounding translations."},
    {"el_GR", "Greek", "Greece", "Tier 2 (Good)", "Good and Reliable. Strong performance in modern Greek."},
    {"en_GB", "English", "United Kingdom", "Tier 1 (High)", "Excellent. Fully aware of British spelling and common idioms."},
    {"en_US", "English", "United States", "Tier 1 (High)", "Excellent. Translations are nuanced, accurate, and preserve context and tone."},
    {"es_ES", "Spanish", "Spain", "Tier 1 (High)", "Excellent. Aware of Castilian vocabulary and norms."},
    {"es_MX", "Spanish", "Mexico", "Tier 1 (High)", "Excellent. The primary standard for Latin American Spanish."},
    {"fa_IR", "Persian", "Iran", "Tier 3 (Functional)", "Functional. Reliable for standard Farsi text; review is recommended."},
    {"fi_FI", "Finnish", "Finland", "Tier 2 (Good)", "Good and Reliable. Handles complex grammar well for most cases."},
    {"fr_FR", "French", "France", "Tier 1 (High)", "Excellent. Consistently high-quality translation."},
    {"he_IL", "Hebrew", "Israel", "Tier 2 (Good)", "Good and Reliable. Consistent and accurate translations."},
    {"hi_IN", "Hindi", "India", "Tier 2 (Good)", "Good and Reliable. Understands Devanagari script and common usage."},
    {"hr_HR", "Croatian", "Croatia", "Tier 3 (Functional)", "Functional. Good for general understanding; may lack natural flow."},
    {"hu_HU", "Hungarian", "Hungary", "Tier 2 (Good)", "Good and Reliable. Suitable for a wide range of translation needs."},
    {"id_ID", "Indonesian", "Indonesia", "Tier 2 (Good)", "Good and Reliable. Very functional and widely applicable."},
    {"it_IT", "Italian", "Italy", "Tier 1 (High)", "Excellent. Reliable for all types of translation tasks."},
    {"ja_JP", "Japanese", "Japan", "Tier 1 (High)", "Excellent. Strong grasp of grammar, script, and cultural context."},
    {"ko_KR", "Korean", "South Korea", "Tier 2 (Good)", "Good and Reliable. Strong understanding of Hangul and modern usage."},
    {"lt_LT", "Lithuanian", "Lithuania", "Tier 3 (Functional)", "Functional. Can produce literal translations; best for simple texts."},
    {"lv_LV", "Latvian", "Latvia", "Tier 3 (Functional)", "Functional. Similar to Lithuanian; best to review for important use."},
    {"ms_MY", "Malay", "Malaysia", "Tier 3 (Functional)", "Functional. Suitable for standard requests and getting the main idea."},
    {"nb_NO", "Norwegian", "Norway", "Tier 2 (Good)", "Good and Reliable. Strong support for the Bokmål standard."},
    {"nl_NL", "Dutch", "Netherlands", "Tier 2 (Good)", "Good and Reliable. High-quality translations for general content."},
    {"pl_PL", "Polish", "Poland", "Tier 2 (Good)", "Good and Reliable. A solid choice for professional use cases."},
    {"pt_BR", "Portuguese", "Brazil", "Tier 1 (High)", "Excellent. The most common and well-supported variant of Portuguese."},
    {"pt_PT", "Portuguese", "Portugal", "Tier 1 (High)", "Excellent. Fully proficient in European Portuguese."},
    {"ro_RO", "Romanian", "Romania", "Tier 2 (Good)", "Good and Reliable. Consistent performance."},
    {"ru_RU", "Russian", "Russia", "Tier 2 (Good)", "Good and Reliable. High accuracy for a wide variety of texts."},
    {"sk_SK", "Slovak", "Slovakia", "Tier 3 (Functional)", "Functional. Good for straightforward text; review complex content."},
    {"sl_SI", "Slovenian", "Slovenia", "Tier 3 (Functional)", "Functional. Best for simple sentences and direct translations."},
    {"sr_RS", "Serbian", "Serbia", "Tier 3 (Functional)", "Functional. Understands Cyrillic/Latin scripts; best for simple text."},
    {"sv_SE", "Swedish", "Sweden", "Tier 2 (Good)", "Good and Reliable. Solid performance for general-purpose translation."},
    {"sw_KE", "Swahili", "Kenya", "Tier 3 (Functional)", "Functional. Primarily useful for basic translation and simple questions."},
    {"th_TH", "Thai", "Thailand", "Tier 2 (Good)", "Good and Reliable. Handles Thai script and nuances effectively."},
    {"tl_PH", "Tagalog", "Philippines", "Tier 3 (Functional)", "Functional. Also fil_PH. Good for gist; may sound machine-like."},
    {"tr_TR", "Turkish", "Turkey", "Tier 2 (Good)", "Good and Reliable. Strong performance for most translation tasks."},
    {"uk_UA", "Ukrainian", "Ukraine", "Tier 2 (Good)", "Good and Reliable. Quality is high and consistently improving."},
    {"ur_PK", "Urdu", "Pakistan", "Tier 3 (Functional)", "Functional. Capable of translating standard text; review recommended."},
    {"vi_VN", "Vietnamese", "Vietnam", "Tier 2 (Good)", "Good and Reliable. Suitable for a wide variety of contexts."},
    {"zh_CN", "Chinese", "China (Simplified)", "Tier 1 (High)", "Excellent. Expert-level translation for Simplified Chinese."},
    {"zh_TW", "Chinese", "Taiwan (Traditional)", "Tier 1 (High)", "Excellent. Expert-level translation for Traditional Chinese."}

};

/**
 * @brief Number of supported locales
 */
static const size_t num_supported_locales = 46;

/**
 * @brief Forward declaration for internal use
 */
static bool muse_extract_language_code_internal(const char *locale_code, char *buffer, size_t buffer_size);

bool muse_is_locale_supported(const char *locale_code) {
    if (!locale_code) {
        return false;
    }
    
    // First try exact match
    for (size_t i = 0; i < num_supported_locales; i++) {
        if (strcmp(supported_locales[i].code, locale_code) == 0) {
            return true;
        }
    }
    
    // If no exact match, try language-only match (e.g., "en" matches "en_US")
    char lang_code[8];
    if (muse_extract_language_code_internal(locale_code, lang_code, sizeof(lang_code))) {
        // Check if the input was already a language code
        if (strcmp(lang_code, locale_code) == 0) {
            // Input is language-only, check if any variant exists
            for (size_t i = 0; i < num_supported_locales; i++) {
                char supported_lang[8];
                if (muse_extract_language_code_internal(supported_locales[i].code, supported_lang, sizeof(supported_lang))) {
                    if (strcmp(supported_lang, lang_code) == 0) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

const char *muse_get_full_locale(const char *language_code) {
    if (!language_code) {
        return NULL;
    }
    
    // Convert hyphenated format to underscore format (es-mx -> es_MX)
    static char normalized_input[16];
    if (strchr(language_code, '-') != NULL) {
        strncpy(normalized_input, language_code, sizeof(normalized_input) - 1);
        normalized_input[sizeof(normalized_input) - 1] = '\0';
        
        // Convert to uppercase after hyphen and replace hyphen with underscore
        char *hyphen = strchr(normalized_input, '-');
        if (hyphen && hyphen[1] && hyphen[2]) {
            *hyphen = '_';
            hyphen[1] = toupper(hyphen[1]);
            hyphen[2] = toupper(hyphen[2]);
        }
        
        // If it's a full locale code, return it if supported
        if (muse_is_locale_supported(normalized_input)) {
            return normalized_input;
        }
    }
    
    // If it's already a full locale code, return it if supported
    if (strchr(language_code, '_') != NULL) {
        return muse_is_locale_supported(language_code) ? language_code : NULL;
    }
    
    // Smart defaults for ambiguous language codes
    if (strcmp(language_code, "zh") == 0) {
        return "zh_CN"; // Default to Simplified Chinese (most common)
    }
    if (strcmp(language_code, "pt") == 0) {
        return "pt_BR"; // Default to Brazilian Portuguese (most common)
    }
    if (strcmp(language_code, "es") == 0) {
        return "es_ES"; // Default to Spain Spanish (original)
    }
    if (strcmp(language_code, "en") == 0) {
        return "en_US"; // Default to US English (most common on web)
    }
    
    // Find first matching locale for this language (fallback)
    for (size_t i = 0; i < num_supported_locales; i++) {
        char lang_code[8];
        if (muse_extract_language_code_internal(supported_locales[i].code, lang_code, sizeof(lang_code))) {
            if (strcmp(lang_code, language_code) == 0) {
                return supported_locales[i].code;
            }
        }
    }
    
    return NULL;
}

const muse_locale_t *muse_get_supported_locales(size_t *count) {
    if (count) {
        *count = num_supported_locales;
    }
    return supported_locales;
}

const char *muse_get_locale_display_name(const char *locale_code) {
    if (!locale_code) {
        return NULL;
    }
    
    for (size_t i = 0; i < num_supported_locales; i++) {
        if (strcmp(supported_locales[i].code, locale_code) == 0) {
            static char display_name[256];
            snprintf(display_name, sizeof(display_name), "%s (%s)", 
                    supported_locales[i].language, supported_locales[i].country);
            return display_name;
        }
    }
    
    return NULL;
}

static bool muse_extract_language_code_internal(const char *locale_code, char *buffer, size_t buffer_size) {
    if (!locale_code || !buffer || buffer_size < 3) {
        return false;
    }
    
    // Check for empty string
    if (strlen(locale_code) == 0) {
        return false;
    }
    
    const char *underscore = strchr(locale_code, '_');
    if (underscore) {
        size_t lang_len = underscore - locale_code;
        if (lang_len == 0 || lang_len >= buffer_size) {
            return false;
        }
        strncpy(buffer, locale_code, lang_len);
        buffer[lang_len] = '\0';
    } else {
        // No underscore, assume it's already a language code
        if (strlen(locale_code) >= buffer_size) {
            return false;
        }
        strcpy(buffer, locale_code);
    }
    
    return true;
}

bool muse_extract_language_code(const char *locale_code, char *buffer, size_t buffer_size) {
    return muse_extract_language_code_internal(locale_code, buffer, buffer_size);
}

const char *muse_get_locale_tier(const char *locale_code) {
    if (!locale_code) {
        return NULL;
    }
    
    for (size_t i = 0; i < num_supported_locales; i++) {
        if (strcmp(supported_locales[i].code, locale_code) == 0) {
            return supported_locales[i].tier;
        }
    }
    
    return NULL;
}
