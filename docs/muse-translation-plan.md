# Muse-AI Translation System Implementation Plan

## Overview
This document provides a comprehensive, step-by-step plan for implementing robust, user-friendly, and maintainable AI-powered translation support in the mod_muse-ai Apache module. The design prioritizes user control, quality, and seamless integration with Apache and MuseWeb paradigms.

---

## Table of Contents
1. Requirements & Principles
2. Supported Locales Module
3. URL and Language Selection Logic
4. Fallback & Default Language Handling
5. Integration with Request Handlers
6. Updating Supported Languages
7. Documentation & User Communication
8. Testing & Validation
9. Future Enhancements

---

## 1. Requirements & Principles
- **User Control:** Never force language based on location or browser settings alone. User choice always takes precedence.
- **Supported Languages:** Only offer translations into languages that LLMs can handle with high confidence, as defined in a maintainable internal list.
- **Fallback:** If translation is not possible, always serve the file in its original language (never default to English unless the file is written in English).
- **Maintainability:** Locale list must be easy to update as LLM capabilities evolve.
- **Transparency:** Clearly document and communicate language selection logic to users.

---

## 2. Supported Locales Module

### 2.1. Create Internal Locale Module
- Create `src/supported_locales.c` and `src/supported_locales.h`.
- Implement:
  - `int is_supported_locale(const char* lang_code);`
  - `const char** get_supported_locales(size_t* count);`
- Store supported locales as a static array of strings, initially generated from `locale-identifiers.csv`.

### 2.2. Automate Locale List Updates (Optional)
- Provide a script (e.g., `tools/gen_locales.py`) to convert the CSV into a C array.
- Document the update process in HOWTO.md.

---

## 3. URL and Language Selection Logic

### 3.1. Primary: Virtual Subdirectory URLs
- Accept URLs like `/es/page`, `/no/about`, `/en/contact`.
- Parse the first path segment as a language code.
- Validate against supported locales.

### 3.2. Secondary: Query Parameter
- Accept `?lang=xx_XX` as a fallback for power users or legacy links.

### 3.3. Tertiary: Cookie-Based Preference (Optional)
- If a user selects a language, set a `muse_ai_lang` cookie.
- Use this preference if no language is specified in the URL or query.

### 3.4. Accept-Language Header (Last Resort)
- If no explicit language is set and no cookie is present, check the browser's `Accept-Language` header.
- Only select a language if it matches the supported list.

---

## 4. Fallback & Default Language Handling

### 4.1. Always Serve Original Language as Fallback
- If the requested translation is not supported or fails, serve the `.ai` file in its original language.
- Never force a default to English unless the file is written in English.

### 4.2. Detecting File Language
- Optionally, add a metadata line to `.ai` files (e.g., `# lang: de_DE`).
- If not present, infer from filename, directory, or site default.
- Document the convention for file language metadata.

---

## 5. Integration with Request Handlers

### 5.1. Update Handler Logic
- On each request:
  1. Parse language from URL, query, or cookie.
  2. Validate against supported locales.
  3. If translation is requested and supported, generate translation prompt.
  4. If not supported, serve original file.

### 5.2. Translation Prompt Construction
- When translation is needed, construct a prompt instructing the LLM to translate the content to the target language.
- Ensure the prompt includes both the target language and original language (if known).

---

## 6. Updating Supported Languages

### 6.1. Manual Update
- Edit `supported_locales.c` to add/remove languages.
- Recompile the module.

### 6.2. Automated Update (Optional)
- Run the provided script to regenerate the C array from the CSV.
- Review and commit changes.

---

## 7. Documentation & User Communication

### 7.1. HOWTO.md
- Add a section explaining:
  - How language selection works (URL, query, cookie, header).
  - How to update the supported locales list.
  - How to mark a file's original language.

### 7.2. User-Facing Communication
- If a requested translation is not available, clearly inform the user that the original language is being served.

---

## 8. Testing & Validation

### 8.1. Unit Tests
- Test locale validation logic.
- Test handler logic for all language selection cases.

### 8.2. Integration Tests
- Simulate requests with various URLs, queries, cookies, and headers.
- Verify correct language is served or translated.

### 8.3. Manual Testing
- Test with real `.ai` files in different source languages.
- Verify fallback and error cases.

---

## 9. Future Enhancements
- Allow dynamic reloading of supported locales without recompiling.
- Add UI for language selection (dropdown, flags, etc.).
- Store user language preference in session or profile.
- Support per-page or per-section default languages.
- Add admin endpoint to display current supported locales.

---

## Appendix: Example Supported Locales Array
```c
static const char* supported_locales[] = {
    "en_US", "en_GB", "no_NO", "es_ES", "de_DE", // ...
};
```

---

**End of Plan**
