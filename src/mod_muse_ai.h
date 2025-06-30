#ifndef MOD_MUSE_AI_H
#define MOD_MUSE_AI_H

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "ap_config.h"
#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_file_io.h"
#include "apr_uri.h"
#include "language_selection.h"

/* Module configuration structure */
typedef struct {
    char *endpoint;     /* MuseWeb endpoint URL */
    int timeout;        /* Request timeout in seconds */
    int debug;          /* Debug flag */
    char *model;        /* AI model to use */
    char *api_key;      /* API key for authentication */
    int streaming;      /* Enable streaming responses */
    int max_tokens;     /* Maximum tokens for AI response generation */
} muse_ai_config;

/* Default configuration values */
#define DEFAULT_ENDPOINT "http://127.0.0.1:11434/v1"
#define MUSE_AI_DEFAULT_TIMEOUT 300
#define DEFAULT_DEBUG 0
#define DEFAULT_MODEL "default"
#define DEFAULT_STREAMING 1  /* Enable streaming by default */

/* Streaming state structure */
typedef struct {
    int streaming_started;      /* Have we started streaming to client? */
    int last_sent_length;      /* How much have we sent so far? */
    char *pending_buffer;      /* Accumulated content buffer */
    int html_complete;         /* Have we seen </html>? */
    apr_time_t buffer_start_time; /* When did we start buffering? */
} streaming_state_t;

/* Function declarations */

/* Configuration functions */
void *create_muse_ai_config(apr_pool_t *pool, server_rec *s);
const char *set_muse_ai_endpoint(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_timeout(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_debug(cmd_parms *cmd, void *cfg, int flag);
const char *set_muse_ai_model(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_api_key(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_streaming(cmd_parms *cmd, void *cfg, int flag);



/* HTTP client functions */
int make_backend_request(request_rec *r, muse_ai_config *cfg, 
                        const char *backend_url, const char *json_payload,
                        char **response_body, const muse_language_selection_t *lang_selection);

/* Streaming functions */
streaming_state_t *create_streaming_state(apr_pool_t *pool);
void reset_streaming_state(streaming_state_t *state);
char *process_streaming_content(request_rec *r, streaming_state_t *state, 
                               const char *new_content, 
                               const muse_language_selection_t *lang_selection);
int find_html_start(const char *content);
int find_html_end(const char *content);

/* Sanitization functions */
char *cleanup_code_fences(apr_pool_t *pool, const char *content);
char *sanitize_response(apr_pool_t *pool, const char *content, const muse_language_selection_t *lang_selection);
char *extract_html_content(apr_pool_t *pool, const char *content);

/* Utility functions */
char *read_file_contents(apr_pool_t *pool, const char *file_path);
char *url_decode(apr_pool_t *pool, const char *encoded);
char *escape_json_string(apr_pool_t *pool, const char *str);

/* Module declaration */
extern module AP_MODULE_DECLARE_DATA muse_ai_module;

#endif /* MOD_MUSE_AI_H */
