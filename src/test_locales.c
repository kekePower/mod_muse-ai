/**
 * @file test_locales.c
 * @brief Test program for supported_locales module
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "supported_locales.h"

void test_basic_validation() {
    printf("Testing basic locale validation...\n");
    
    // Test exact matches
    assert(muse_is_locale_supported("en_US") == true);
    assert(muse_is_locale_supported("es_ES") == true);
    assert(muse_is_locale_supported("fr_FR") == true);
    assert(muse_is_locale_supported("de_DE") == true);
    
    // Test language-only codes
    assert(muse_is_locale_supported("en") == true);
    assert(muse_is_locale_supported("es") == true);
    assert(muse_is_locale_supported("fr") == true);
    
    // Test invalid codes
    assert(muse_is_locale_supported("invalid_CODE") == false);
    assert(muse_is_locale_supported("xx") == false);
    assert(muse_is_locale_supported("") == false);
    assert(muse_is_locale_supported(NULL) == false);
    
    printf("✓ Basic validation tests passed\n");
}

void test_full_locale_retrieval() {
    printf("Testing full locale retrieval...\n");
    
    const char *full_en = muse_get_full_locale("en");
    assert(full_en != NULL);
    assert(strstr(full_en, "en_") == full_en);  // Should start with "en_"
    
    const char *full_es = muse_get_full_locale("es");
    assert(full_es != NULL);
    assert(strstr(full_es, "es_") == full_es);  // Should start with "es_"
    
    // Test with full locale codes
    const char *full_us = muse_get_full_locale("en_US");
    assert(full_us != NULL);
    assert(strcmp(full_us, "en_US") == 0);
    
    // Test invalid
    assert(muse_get_full_locale("invalid") == NULL);
    assert(muse_get_full_locale(NULL) == NULL);
    
    printf("✓ Full locale retrieval tests passed\n");
}

void test_language_extraction() {
    printf("Testing language code extraction...\n");
    
    char buffer[8];
    
    // Test full locale codes
    assert(muse_extract_language_code("en_US", buffer, sizeof(buffer)) == true);
    assert(strcmp(buffer, "en") == 0);
    
    assert(muse_extract_language_code("es_ES", buffer, sizeof(buffer)) == true);
    assert(strcmp(buffer, "es") == 0);
    
    assert(muse_extract_language_code("zh_CN", buffer, sizeof(buffer)) == true);
    assert(strcmp(buffer, "zh") == 0);
    
    // Test language-only codes
    assert(muse_extract_language_code("en", buffer, sizeof(buffer)) == true);
    assert(strcmp(buffer, "en") == 0);
    
    // Test edge cases
    assert(muse_extract_language_code("", buffer, sizeof(buffer)) == false);
    assert(muse_extract_language_code(NULL, buffer, sizeof(buffer)) == false);
    assert(muse_extract_language_code("en_US", NULL, sizeof(buffer)) == false);
    assert(muse_extract_language_code("en_US", buffer, 2) == false);  // Buffer too small
    
    printf("✓ Language extraction tests passed\n");
}

void test_display_names() {
    printf("Testing display names...\n");
    
    const char *display = muse_get_locale_display_name("en_US");
    assert(display != NULL);
    assert(strstr(display, "English") != NULL);
    assert(strstr(display, "United States") != NULL);
    
    display = muse_get_locale_display_name("es_ES");
    assert(display != NULL);
    assert(strstr(display, "Spanish") != NULL);
    assert(strstr(display, "Spain") != NULL);
    
    // Test invalid
    assert(muse_get_locale_display_name("invalid_CODE") == NULL);
    assert(muse_get_locale_display_name(NULL) == NULL);
    
    printf("✓ Display name tests passed\n");
}

void test_supported_locales_list() {
    printf("Testing supported locales list...\n");
    
    size_t count = 0;
    const muse_locale_t *locales = muse_get_supported_locales(&count);
    
    assert(locales != NULL);
    assert(count > 40);  // We should have over 40 locales
    
    // Check first few entries are valid
    for (size_t i = 0; i < 5 && i < count; i++) {
        assert(locales[i].code != NULL);
        assert(locales[i].language != NULL);
        assert(locales[i].country != NULL);
        assert(locales[i].tier != NULL);
        assert(locales[i].notes != NULL);
        assert(strlen(locales[i].code) > 0);
        assert(strlen(locales[i].language) > 0);
        assert(strlen(locales[i].country) > 0);
        assert(strlen(locales[i].tier) > 0);
        // Notes can be empty, so don't assert on length
    }
    
    printf("✓ Supported locales list tests passed (found %zu locales)\n", count);
}

int main() {
    printf("Running supported_locales module tests...\n\n");
    
    test_basic_validation();
    test_full_locale_retrieval();
    test_language_extraction();
    test_display_names();
    test_supported_locales_list();
    
    printf("\n🎉 All tests passed! The supported_locales module is working correctly.\n");
    return 0;
}
