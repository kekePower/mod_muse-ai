#ifndef REQUEST_HANDLERS_H
#define REQUEST_HANDLERS_H

#include "httpd.h"
#include "http_config.h"

#include "advanced_config.h"

/* Initializes Phase 3 features. Called by the post_config hook. */
int init_phase3_features(apr_pool_t *pool, server_rec *s, struct advanced_muse_ai_config *cfg);

/* Phase 3 request handlers */
int enhanced_muse_ai_handler(request_rec *r);
int ai_file_handler(request_rec *r);
int metrics_handler(request_rec *r);
int health_check_handler(request_rec *r);

#endif /* REQUEST_HANDLERS_H */
