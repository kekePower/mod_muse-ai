#!/usr/bin/env python3
"""
Generate supported_locales.c from locale-identifiers.csv

This script reads the CSV file containing locale identifiers and generates
a C source file with static arrays and functions for locale validation.
"""

import csv
import sys
import os
from pathlib import Path

def read_locales_csv(csv_path):
    """Read locale data from CSV file"""
    locales = []
    try:
        with open(csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                # Skip invalid rows (missing required fields or malformed)
                code = row.get('Locale Code', '').strip()
                language = row.get('Language', '').strip()
                country = row.get('Country / Region', '').strip()
                tier = row.get('Proficiency Tier', '').strip()
                notes = row.get('Notes on Translation Performance', '').strip()
                
                # Skip rows with empty locale codes or invalid format
                if not code or code == 'Locale Code' or '_' not in code:
                    continue
                    
                if not language or not country:
                    continue
                    
                locales.append({
                    'code': code,
                    'language': language,
                    'country': country,
                    'tier': tier,
                    'notes': notes
                })
    except Exception as e:
        print(f"Error reading CSV file: {e}", file=sys.stderr)
        sys.exit(1)
    
    return locales

def generate_c_file(locales, output_path):
    """Generate the C implementation file"""
    
    # Sort locales by code for consistent output
    locales.sort(key=lambda x: x['code'])
    
    c_content = f'''/**
 * @file supported_locales.c
 * @brief Implementation of supported locales module
 * 
 * This file is auto-generated from docs/locale-identifiers.csv
 * Do not edit manually - run scripts/generate_locales.py to regenerate
 * 
 * Generated on: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
 * Total locales: {len(locales)}
 */

#include "supported_locales.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Static array of all supported locales
 */
static const muse_locale_t supported_locales[] = {{
'''

    # Generate the locale array
    for i, locale in enumerate(locales):
        comma = ',' if i < len(locales) - 1 else ''
        # Escape quotes in notes field
        notes_escaped = locale["notes"].replace('"', '\\"')
        c_content += f'    {{"{locale["code"]}", "{locale["language"]}", "{locale["country"]}", "{locale["tier"]}", "{notes_escaped}"}}{comma}\n'
    
    c_content += f'''
}};

/**
 * @brief Number of supported locales
 */
static const size_t num_supported_locales = {len(locales)};

/**
 * @brief Forward declaration for internal use
 */
static bool muse_extract_language_code_internal(const char *locale_code, char *buffer, size_t buffer_size);

bool muse_is_locale_supported(const char *locale_code) {{
    if (!locale_code) {{
        return false;
    }}
    
    // First try exact match
    for (size_t i = 0; i < num_supported_locales; i++) {{
        if (strcmp(supported_locales[i].code, locale_code) == 0) {{
            return true;
        }}
    }}
    
    // If no exact match, try language-only match (e.g., "en" matches "en_US")
    char lang_code[8];
    if (muse_extract_language_code_internal(locale_code, lang_code, sizeof(lang_code))) {{
        // Check if the input was already a language code
        if (strcmp(lang_code, locale_code) == 0) {{
            // Input is language-only, check if any variant exists
            for (size_t i = 0; i < num_supported_locales; i++) {{
                char supported_lang[8];
                if (muse_extract_language_code_internal(supported_locales[i].code, supported_lang, sizeof(supported_lang))) {{
                    if (strcmp(supported_lang, lang_code) == 0) {{
                        return true;
                    }}
                }}
            }}
        }}
    }}
    
    return false;
}}

const char *muse_get_full_locale(const char *language_code) {{
    if (!language_code) {{
        return NULL;
    }}
    
    // If it's already a full locale code, return it if supported
    if (strchr(language_code, '_') != NULL) {{
        return muse_is_locale_supported(language_code) ? language_code : NULL;
    }}
    
    // Find first matching locale for this language
    for (size_t i = 0; i < num_supported_locales; i++) {{
        char lang_code[8];
        if (muse_extract_language_code_internal(supported_locales[i].code, lang_code, sizeof(lang_code))) {{
            if (strcmp(lang_code, language_code) == 0) {{
                return supported_locales[i].code;
            }}
        }}
    }}
    
    return NULL;
}}

const muse_locale_t *muse_get_supported_locales(size_t *count) {{
    if (count) {{
        *count = num_supported_locales;
    }}
    return supported_locales;
}}

const char *muse_get_locale_display_name(const char *locale_code) {{
    if (!locale_code) {{
        return NULL;
    }}
    
    for (size_t i = 0; i < num_supported_locales; i++) {{
        if (strcmp(supported_locales[i].code, locale_code) == 0) {{
            static char display_name[256];
            snprintf(display_name, sizeof(display_name), "%s (%s)", 
                    supported_locales[i].language, supported_locales[i].country);
            return display_name;
        }}
    }}
    
    return NULL;
}}

static bool muse_extract_language_code_internal(const char *locale_code, char *buffer, size_t buffer_size) {{
    if (!locale_code || !buffer || buffer_size < 3) {{
        return false;
    }}
    
    // Check for empty string
    if (strlen(locale_code) == 0) {{
        return false;
    }}
    
    const char *underscore = strchr(locale_code, '_');
    if (underscore) {{
        size_t lang_len = underscore - locale_code;
        if (lang_len == 0 || lang_len >= buffer_size) {{
            return false;
        }}
        strncpy(buffer, locale_code, lang_len);
        buffer[lang_len] = '\\0';
    }} else {{
        // No underscore, assume it's already a language code
        if (strlen(locale_code) >= buffer_size) {{
            return false;
        }}
        strcpy(buffer, locale_code);
    }}
    
    return true;
}}

bool muse_extract_language_code(const char *locale_code, char *buffer, size_t buffer_size) {{
    return muse_extract_language_code_internal(locale_code, buffer, buffer_size);
}}

const char *muse_get_locale_tier(const char *locale_code) {{
    if (!locale_code) {{
        return NULL;
    }}
    
    for (size_t i = 0; i < num_supported_locales; i++) {{
        if (strcmp(supported_locales[i].code, locale_code) == 0) {{
            return supported_locales[i].tier;
        }}
    }}
    
    return NULL;
}}
'''
    
    try:
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(c_content)
        print(f"Generated {output_path} with {len(locales)} locales")
    except Exception as e:
        print(f"Error writing C file: {e}", file=sys.stderr)
        sys.exit(1)

def main():
    # Get script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    # Input and output paths
    csv_path = project_root / "docs" / "pruned-languages.csv"
    output_path = project_root / "src" / "supported_locales.c"
    
    if not csv_path.exists():
        print(f"CSV file not found: {csv_path}", file=sys.stderr)
        sys.exit(1)
    
    # Read locales and generate C file
    locales = read_locales_csv(csv_path)
    generate_c_file(locales, output_path)
    
    print(f"Successfully generated supported_locales.c with {len(locales)} locales")

if __name__ == "__main__":
    main()
