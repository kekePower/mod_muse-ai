#ifndef ADVANCED_CONFIG_H
#define ADVANCED_CONFIG_H

#include <httpd.h>
#include <http_config.h>
#include <apr_pools.h>
#include <apr_tables.h>

/* Advanced configuration structure extending the basic muse_ai_config */
typedef struct advanced_muse_ai_config {
    /* Basic configuration (inherited) */
    char *endpoint;
    char *api_key;
    char *model;
    int timeout;
    int debug;
    int streaming;
    
    /* Phase 3: Advanced Features */
    
    /* Connection Pooling */
    int pool_max_connections;
    int pool_connection_timeout;
    int pool_idle_timeout;
    int pool_enable_keepalive;
    
    /* Caching */
    int cache_enable;
    int cache_ttl_seconds;
    int cache_max_entries;
    char *cache_key_prefix;
    
    /* Rate Limiting */
    int ratelimit_enable;
    int ratelimit_requests_per_minute;
    int ratelimit_burst_size;
    char *ratelimit_whitelist_ips;
    
    /* Performance Monitoring */
    int metrics_enable;
    char *metrics_endpoint;
    int metrics_include_request_details;
    
    /* Reasoning Models Support */
    apr_table_t *reasoning_model_patterns;
    int reasoning_disable_thinking;
    
    /* Advanced Streaming */
    int streaming_buffer_size;
    int streaming_chunk_size;
    int streaming_sanitization_enable;
    
    /* Security */
    int security_validate_content_type;
    int security_max_request_size;
    char *security_allowed_origins;
    
    /* Load Balancing */
    apr_array_header_t *backend_endpoints;
    char *load_balance_method; /* "round_robin", "least_connections", "random" */
    int health_check_interval;
    
    /* Timeouts and Retries */
    int connect_timeout;
    int read_timeout;
    int write_timeout;
    int max_retries;
    int retry_delay_ms;

    /* Prompts Directory Configuration */
    char *prompts_dir; /* Path to the prompts directory */
    int prompts_minify; /* Flag to use minified layout */
    int phase3_initialized; /* Flag to check if phase 3 features are initialized */
    
} advanced_muse_ai_config;

/* Backend endpoint structure for load balancing */
typedef struct backend_endpoint {
    char *url;
    char *api_key;
    int weight;
    int active_connections;
    int total_requests;
    int failed_requests;
    apr_time_t last_health_check;
    int healthy;
} backend_endpoint_t;

/* Metrics structure */
typedef struct muse_ai_metrics {
    /* Request counters */
    long total_requests;
    long successful_requests;
    long failed_requests;
    long cached_responses;
    
    /* Timing metrics */
    double avg_response_time_ms;
    double min_response_time_ms;
    double max_response_time_ms;
    
    /* Connection pool metrics */
    int pool_active_connections;
    int pool_idle_connections;
    int pool_total_created;
    int pool_total_reused;
    
    /* Rate limiting metrics */
    long ratelimit_blocked_requests;
    
    /* Backend health */
    int healthy_backends;
    int total_backends;
    
    apr_time_t last_updated;
} muse_ai_metrics_t;

/* Function declarations */
void *create_advanced_muse_ai_config(apr_pool_t *pool, server_rec *s);
void *merge_advanced_muse_ai_config(apr_pool_t *p, void *base_conf, void *new_conf);
extern const command_rec muse_ai_advanced_cmds[];
const char *set_pool_max_connections(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_cache_enable(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_cache_ttl(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_ratelimit_enable(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_ratelimit_rpm(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_metrics_enable(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_reasoning_model_pattern(cmd_parms *cmd, void *cfg, const char *pattern);
const char *set_backend_endpoint(cmd_parms *cmd, void *cfg, const char *endpoint);
const char *set_load_balance_method(cmd_parms *cmd, void *cfg, const char *method);
const char *set_streaming_buffer_size(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_security_max_request_size(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_prompts_dir(cmd_parms *cmd, void *cfg, const char *arg);
const char *set_muse_ai_prompts_minify(cmd_parms *cmd, void *cfg, const char *arg);

/* Metrics functions */
muse_ai_metrics_t *get_global_metrics(void);
void update_request_metrics(double response_time_ms, int success);
void update_cache_metrics(int cache_hit);
void update_pool_metrics(int active, int idle, int created, int reused);
void update_ratelimit_metrics(int blocked);
void reset_metrics(void);

/* Configuration validation */
int validate_advanced_config(advanced_muse_ai_config *cfg, server_rec *s);

#endif /* ADVANCED_CONFIG_H */
