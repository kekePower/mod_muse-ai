/**
 * @file error_pages.h
 * @brief Error page generation for mod_muse-ai
 * 
 * This module handles generation of user-friendly error pages for various
 * error conditions including unsupported languages and missing files.
 */

#ifndef ERROR_PAGES_H
#define ERROR_PAGES_H

#include "httpd.h"
#include "http_protocol.h"
#include "language_selection.h"

/**
 * Generate a helpful error page for unsupported languages
 * 
 * @param r The request record
 * @param lang_selection Language selection result with error details
 * @return HTTP status code (typically HTTP_NOT_FOUND)
 */
int generate_language_error_page(request_rec *r, muse_language_selection_t *lang_selection);

/**
 * Generate a 404 error page for missing .ai files
 * 
 * @param r The request record
 * @param requested_path The path that was requested but not found
 * @return HTTP status code (HTTP_NOT_FOUND)
 */
int generate_file_not_found_page(request_rec *r, const char *requested_path);

/**
 * Generate a general error page with custom message
 * 
 * @param r The request record
 * @param status HTTP status code to return
 * @param title Error page title
 * @param message Error message to display
 * @return HTTP status code (same as status parameter)
 */
int generate_custom_error_page(request_rec *r, int status, const char *title, const char *message);

#endif /* ERROR_PAGES_H */
