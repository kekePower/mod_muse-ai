/**
 * @file error_pages.c
 * @brief Error page generation implementation for mod_muse-ai
 */

#include "error_pages.h"
#include "http_protocol.h"
#include "http_log.h"
#include <apr_strings.h>

/**
 * Generate the common HTML header for error pages
 */
static const char *generate_error_page_header(apr_pool_t *pool, const char *title)
{
    return apr_psprintf(pool,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"utf-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "    <link rel=\"stylesheet\" href=\"/css/style.css\">\n"
        "    <title>%s - mod_muse-ai</title>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <header class=\"banner\">\n"
        "            <h1>%s</h1>\n"
        "        </header>\n"
        "        <nav>\n"
        "            <a href=\"/\">🏠 Home</a> | \n"
        "            <a href=\"/features\">✨ Features</a> | \n"
        "            <a href=\"/documentation\">📚 Documentation</a> | \n"
        "            <a href=\"/download\">⬇️ Download</a>\n"
        "        </nav>\n"
        "        <main>\n",
        title, title);
}

/**
 * Generate the common HTML footer for error pages
 */
static const char *generate_error_page_footer(apr_pool_t *pool)
{
    (void)pool; /* Unused parameter */
    return 
        "        </main>\n"
        "        <footer>\n"
        "            <p>&copy; 2025 mod_muse-ai. <a href=\"https://github.com/kekePower/mod_muse-ai\">Powered by mod_muse-ai</a></p>\n"
        "        </footer>\n"
        "    </div>\n"
        "</body>\n"
        "</html>";
}

int generate_language_error_page(request_rec *r, muse_language_selection_t *lang_selection)
{
    const char *requested_lang = lang_selection->language_code ? lang_selection->language_code : "unknown";
    const char *source = lang_selection->source ? lang_selection->source : "unknown";
    
    /* Set appropriate HTTP status */
    r->status = HTTP_NOT_FOUND;
    
    /* Set content type */
    ap_set_content_type(r, "text/html; charset=utf-8");
    
    /* Generate the error page */
    const char *header = generate_error_page_header(r->pool, "Language Not Supported");
    const char *footer = generate_error_page_footer(r->pool);
    
    const char *content = apr_psprintf(r->pool,
        "            <div class=\"alert\">\n"
        "                <h2>🌍 Requested Language Not Available</h2>\n"
        "                <p><strong>Language requested:</strong> <code>%s</code> (detected from %s)</p>\n"
        "                <p>Unfortunately, this language is not currently supported by our AI translation system.</p>\n"
        "            </div>\n"
        "            \n"
        "            <section>\n"
        "                <h3>🔧 What You Can Do</h3>\n"
        "                <ul>\n"
        "                    <li><strong>View in English:</strong> <a href=\"/\">Visit the English version</a></li>\n"
        "                    <li><strong>Try a supported language:</strong> Choose from our <a href=\"#supported-languages\">46 supported languages</a> below</li>\n"
        "                    <li><strong>Request support:</strong> <a href=\"https://github.com/kekePower/mod_muse-ai/issues\">Open an issue on GitHub</a> to request this language</li>\n"
        "                </ul>\n"
        "            </section>\n"
        "            \n"
        "            <section id=\"supported-languages\">\n"
        "                <h3>🌐 Supported Languages</h3>\n"
        "                <p>Our AI translation system currently supports these 46 languages with quality tiers:</p>\n"
        "                \n"
        "                <h4>Tier 1 (High Quality)</h4>\n"
        "                <p>Excellent translations with nuanced understanding:</p>\n"
        "                <ul>\n"
        "                    <li><a href=\"/zh-cn/\">🇨🇳 Chinese (Simplified)</a> - zh-cn</li>\n"
        "                    <li><a href=\"/zh-tw/\">🇹🇼 Chinese (Traditional)</a> - zh-tw</li>\n"
        "                    <li><a href=\"/en-us/\">🇺🇸 English (US)</a> - en-us</li>\n"
        "                    <li><a href=\"/en-gb/\">🇬🇧 English (UK)</a> - en-gb</li>\n"
        "                    <li><a href=\"/es-es/\">🇪🇸 Spanish (Spain)</a> - es-es</li>\n"
        "                    <li><a href=\"/es-mx/\">🇲🇽 Spanish (Mexico)</a> - es-mx</li>\n"
        "                    <li><a href=\"/fr/\">🇫🇷 French</a> - fr</li>\n"
        "                    <li><a href=\"/de/\">🇩🇪 German</a> - de</li>\n"
        "                    <li><a href=\"/it/\">🇮🇹 Italian</a> - it</li>\n"
        "                    <li><a href=\"/ja/\">🇯🇵 Japanese</a> - ja</li>\n"
        "                    <li><a href=\"/pt-br/\">🇧🇷 Portuguese (Brazil)</a> - pt-br</li>\n"
        "                    <li><a href=\"/pt-pt/\">🇵🇹 Portuguese (Portugal)</a> - pt-pt</li>\n"
        "                </ul>\n"
        "                \n"
        "                <h4>Tier 2 (Good Quality)</h4>\n"
        "                <p>Reliable translations suitable for most content:</p>\n"
        "                <ul>\n"
        "                    <li><a href=\"/ar/\">🇸🇦 Arabic</a> - ar</li>\n"
        "                    <li><a href=\"/bn/\">🇧🇩 Bengali</a> - bn</li>\n"
        "                    <li><a href=\"/cs/\">🇨🇿 Czech</a> - cs</li>\n"
        "                    <li><a href=\"/da/\">🇩🇰 Danish</a> - da</li>\n"
        "                    <li><a href=\"/el/\">🇬🇷 Greek</a> - el</li>\n"
        "                    <li><a href=\"/he/\">🇮🇱 Hebrew</a> - he</li>\n"
        "                    <li><a href=\"/hi/\">🇮🇳 Hindi</a> - hi</li>\n"
        "                    <li><a href=\"/hu/\">🇭🇺 Hungarian</a> - hu</li>\n"
        "                    <li><a href=\"/id/\">🇮🇩 Indonesian</a> - id</li>\n"
        "                    <li><a href=\"/ko/\">🇰🇷 Korean</a> - ko</li>\n"
        "                    <li><a href=\"/nb/\">🇳🇴 Norwegian</a> - nb</li>\n"
        "                    <li><a href=\"/nl/\">🇳🇱 Dutch</a> - nl</li>\n"
        "                    <li><a href=\"/pl/\">🇵🇱 Polish</a> - pl</li>\n"
        "                    <li><a href=\"/ro/\">🇷🇴 Romanian</a> - ro</li>\n"
        "                    <li><a href=\"/ru/\">🇷🇺 Russian</a> - ru</li>\n"
        "                    <li><a href=\"/sv/\">🇸🇪 Swedish</a> - sv</li>\n"
        "                    <li><a href=\"/th/\">🇹🇭 Thai</a> - th</li>\n"
        "                    <li><a href=\"/tr/\">🇹🇷 Turkish</a> - tr</li>\n"
        "                    <li><a href=\"/uk/\">🇺🇦 Ukrainian</a> - uk</li>\n"
        "                    <li><a href=\"/vi/\">🇻🇳 Vietnamese</a> - vi</li>\n"
        "                    <li>And more...</li>\n"
        "                </ul>\n"
        "                \n"
        "                <p><strong>Quick access:</strong> Use short codes like <code>/zh/</code>, <code>/es/</code>, <code>/pt/</code> for smart defaults, or full codes like <code>/zh-tw/</code>, <code>/es-mx/</code>, <code>/pt-pt/</code> for specific variants.</p>\n"
        "            </section>\n",
        requested_lang, source);
    
    /* Combine header, content, and footer */
    const char *full_page = apr_pstrcat(r->pool, header, content, footer, NULL);
    
    /* Send the error page */
    ap_rputs(full_page, r);
    
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                 "[mod_muse_ai] Generated language error page for unsupported language: %s (source: %s)",
                 requested_lang, source);
    
    return HTTP_NOT_FOUND;
}

int generate_file_not_found_page(request_rec *r, const char *requested_path)
{
    /* Set appropriate HTTP status */
    r->status = HTTP_NOT_FOUND;
    
    /* Set content type */
    ap_set_content_type(r, "text/html; charset=utf-8");
    
    /* Generate the error page */
    const char *header = generate_error_page_header(r->pool, "Page Not Found");
    const char *footer = generate_error_page_footer(r->pool);
    
    const char *content = apr_psprintf(r->pool,
        "            <div class=\"alert\">\n"
        "                <h2>📄 Page Not Found</h2>\n"
        "                <p><strong>Requested path:</strong> <code>%s</code></p>\n"
        "                <p>The AI-powered page you're looking for doesn't exist or has been moved.</p>\n"
        "            </div>\n"
        "            \n"
        "            <section>\n"
        "                <h3>🔧 What You Can Do</h3>\n"
        "                <ul>\n"
        "                    <li><strong>Go home:</strong> <a href=\"/\">Visit the homepage</a></li>\n"
        "                    <li><strong>Check the URL:</strong> Make sure you typed the address correctly</li>\n"
        "                    <li><strong>Browse features:</strong> <a href=\"/features\">Explore what mod_muse-ai can do</a></li>\n"
        "                    <li><strong>Read documentation:</strong> <a href=\"/documentation\">Learn how to create .ai files</a></li>\n"
        "                </ul>\n"
        "            </section>\n"
        "            \n"
        "            <section>\n"
        "                <h3>🤖 About AI-Powered Pages</h3>\n"
        "                <p>mod_muse-ai generates content dynamically from <code>.ai</code> files. If you're looking for a specific page:</p>\n"
        "                <ul>\n"
        "                    <li>Check if there's a corresponding <code>.ai</code> file in the document root</li>\n"
        "                    <li>Verify the file has the correct permissions</li>\n"
        "                    <li>Ensure the AI backend service is running</li>\n"
        "                </ul>\n"
        "                <p><strong>Example:</strong> <code>/about</code> requires <code>/var/www/html/about.ai</code></p>\n"
        "            </section>\n",
        requested_path ? requested_path : "unknown");
    
    /* Combine header, content, and footer */
    const char *full_page = apr_pstrcat(r->pool, header, content, footer, NULL);
    
    /* Send the error page */
    ap_rputs(full_page, r);
    
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                 "[mod_muse_ai] Generated 404 error page for missing path: %s",
                 requested_path ? requested_path : "unknown");
    
    return HTTP_NOT_FOUND;
}

int generate_custom_error_page(request_rec *r, int status, const char *title, const char *message)
{
    /* Set appropriate HTTP status */
    r->status = status;
    
    /* Set content type */
    ap_set_content_type(r, "text/html; charset=utf-8");
    
    /* Generate the error page */
    const char *header = generate_error_page_header(r->pool, title);
    const char *footer = generate_error_page_footer(r->pool);
    
    const char *content = apr_psprintf(r->pool,
        "            <div class=\"alert\">\n"
        "                <h2>⚠️ %s</h2>\n"
        "                <p>%s</p>\n"
        "            </div>\n"
        "            \n"
        "            <section>\n"
        "                <h3>🔧 What You Can Do</h3>\n"
        "                <ul>\n"
        "                    <li><strong>Go home:</strong> <a href=\"/\">Visit the homepage</a></li>\n"
        "                    <li><strong>Try again:</strong> Refresh the page or try your request again</li>\n"
        "                    <li><strong>Get help:</strong> <a href=\"https://github.com/kekePower/mod_muse-ai/issues\">Report this issue on GitHub</a></li>\n"
        "                </ul>\n"
        "            </section>\n",
        title, message);
    
    /* Combine header, content, and footer */
    const char *full_page = apr_pstrcat(r->pool, header, content, footer, NULL);
    
    /* Send the error page */
    ap_rputs(full_page, r);
    
    ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                 "[mod_muse_ai] Generated custom error page: %s (status: %d)",
                 title, status);
    
    return status;
}
