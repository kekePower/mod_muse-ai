#include "advanced_config.h"
#include <apr_strings.h>
#include <apr_time.h>
#include <http_log.h>
#include <math.h>

/* Global metrics instance */
static muse_ai_metrics_t *global_metrics = NULL;
static apr_thread_mutex_t *metrics_mutex = NULL;

/* Initialize metrics system */
int init_metrics_system(apr_pool_t *pool, server_rec *s)
{
    apr_status_t rv;
    
    /* Allocate global metrics structure */
    global_metrics = apr_pcalloc(pool, sizeof(muse_ai_metrics_t));
    if (!global_metrics) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Failed to allocate metrics structure");
        return -1;
    }
    
    /* Initialize metrics */
    global_metrics->total_requests = 0;
    global_metrics->successful_requests = 0;
    global_metrics->failed_requests = 0;
    global_metrics->cached_responses = 0;
    global_metrics->avg_response_time_ms = 0.0;
    global_metrics->min_response_time_ms = 0.0;
    global_metrics->max_response_time_ms = 0.0;
    global_metrics->pool_active_connections = 0;
    global_metrics->pool_idle_connections = 0;
    global_metrics->pool_total_created = 0;
    global_metrics->pool_total_reused = 0;
    global_metrics->ratelimit_blocked_requests = 0;
    global_metrics->healthy_backends = 0;
    global_metrics->total_backends = 0;
    global_metrics->last_updated = apr_time_now();
    
    /* Create mutex for thread safety */
    rv = apr_thread_mutex_create(&metrics_mutex, APR_THREAD_MUTEX_DEFAULT, pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, s, 
                    "[mod_muse_ai] Failed to create metrics mutex");
        return -1;
    }
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Metrics system initialized");
    
    return 0;
}

/* Get global metrics instance */
muse_ai_metrics_t *get_global_metrics(void)
{
    return global_metrics;
}

/* Update request metrics */
void update_request_metrics(double response_time_ms, int success)
{
    if (!global_metrics || !metrics_mutex) {
        return;
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    global_metrics->total_requests++;
    global_metrics->last_updated = apr_time_now();
    
    if (success) {
        global_metrics->successful_requests++;
    } else {
        global_metrics->failed_requests++;
    }
    
    /* Update response time statistics */
    if (response_time_ms > 0) {
        if (global_metrics->min_response_time_ms == 0.0 || 
            response_time_ms < global_metrics->min_response_time_ms) {
            global_metrics->min_response_time_ms = response_time_ms;
        }
        
        if (response_time_ms > global_metrics->max_response_time_ms) {
            global_metrics->max_response_time_ms = response_time_ms;
        }
        
        /* Calculate running average */
        if (global_metrics->successful_requests > 1) {
            global_metrics->avg_response_time_ms = 
                (global_metrics->avg_response_time_ms * (global_metrics->successful_requests - 1) + 
                 response_time_ms) / global_metrics->successful_requests;
        } else {
            global_metrics->avg_response_time_ms = response_time_ms;
        }
    }
    
    apr_thread_mutex_unlock(metrics_mutex);
}

/* Update cache metrics */
void update_cache_metrics(int cache_hit)
{
    if (!global_metrics || !metrics_mutex) {
        return;
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    if (cache_hit) {
        global_metrics->cached_responses++;
    }
    
    global_metrics->last_updated = apr_time_now();
    
    apr_thread_mutex_unlock(metrics_mutex);
}

/* Update connection pool metrics */
void update_pool_metrics(int active, int idle, int created, int reused)
{
    if (!global_metrics || !metrics_mutex) {
        return;
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    global_metrics->pool_active_connections = active;
    global_metrics->pool_idle_connections = idle;
    
    if (created > 0) {
        global_metrics->pool_total_created += created;
    }
    
    if (reused > 0) {
        global_metrics->pool_total_reused += reused;
    }
    
    global_metrics->last_updated = apr_time_now();
    
    apr_thread_mutex_unlock(metrics_mutex);
}

/* Update rate limiting metrics */
void update_ratelimit_metrics(int blocked)
{
    if (!global_metrics || !metrics_mutex) {
        return;
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    if (blocked > 0) {
        global_metrics->ratelimit_blocked_requests += blocked;
    }
    
    global_metrics->last_updated = apr_time_now();
    
    apr_thread_mutex_unlock(metrics_mutex);
}

/* Reset all metrics */
void reset_metrics(void)
{
    if (!global_metrics || !metrics_mutex) {
        return;
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    global_metrics->total_requests = 0;
    global_metrics->successful_requests = 0;
    global_metrics->failed_requests = 0;
    global_metrics->cached_responses = 0;
    global_metrics->avg_response_time_ms = 0.0;
    global_metrics->min_response_time_ms = 0.0;
    global_metrics->max_response_time_ms = 0.0;
    global_metrics->pool_total_created = 0;
    global_metrics->pool_total_reused = 0;
    global_metrics->ratelimit_blocked_requests = 0;
    global_metrics->last_updated = apr_time_now();
    
    apr_thread_mutex_unlock(metrics_mutex);
}

/* Generate Prometheus-style metrics output */
char *generate_prometheus_metrics(apr_pool_t *pool)
{
    char *metrics_output;
    muse_ai_metrics_t *metrics = get_global_metrics();
    
    if (!metrics) {
        return apr_pstrdup(pool, "# Metrics not available\n");
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    metrics_output = apr_psprintf(pool,
        "# HELP mod_muse_ai_requests_total Total number of requests processed\n"
        "# TYPE mod_muse_ai_requests_total counter\n"
        "mod_muse_ai_requests_total %ld\n"
        "\n"
        "# HELP mod_muse_ai_requests_successful_total Total number of successful requests\n"
        "# TYPE mod_muse_ai_requests_successful_total counter\n"
        "mod_muse_ai_requests_successful_total %ld\n"
        "\n"
        "# HELP mod_muse_ai_requests_failed_total Total number of failed requests\n"
        "# TYPE mod_muse_ai_requests_failed_total counter\n"
        "mod_muse_ai_requests_failed_total %ld\n"
        "\n"
        "# HELP mod_muse_ai_cache_hits_total Total number of cache hits\n"
        "# TYPE mod_muse_ai_cache_hits_total counter\n"
        "mod_muse_ai_cache_hits_total %ld\n"
        "\n"
        "# HELP mod_muse_ai_response_time_seconds Response time statistics\n"
        "# TYPE mod_muse_ai_response_time_seconds gauge\n"
        "mod_muse_ai_response_time_seconds{quantile=\"avg\"} %.3f\n"
        "mod_muse_ai_response_time_seconds{quantile=\"min\"} %.3f\n"
        "mod_muse_ai_response_time_seconds{quantile=\"max\"} %.3f\n"
        "\n"
        "# HELP mod_muse_ai_pool_connections Current connection pool status\n"
        "# TYPE mod_muse_ai_pool_connections gauge\n"
        "mod_muse_ai_pool_connections{state=\"active\"} %d\n"
        "mod_muse_ai_pool_connections{state=\"idle\"} %d\n"
        "\n"
        "# HELP mod_muse_ai_pool_connections_total Total connection pool operations\n"
        "# TYPE mod_muse_ai_pool_connections_total counter\n"
        "mod_muse_ai_pool_connections_total{operation=\"created\"} %d\n"
        "mod_muse_ai_pool_connections_total{operation=\"reused\"} %d\n"
        "\n"
        "# HELP mod_muse_ai_ratelimit_blocked_total Total number of rate-limited requests\n"
        "# TYPE mod_muse_ai_ratelimit_blocked_total counter\n"
        "mod_muse_ai_ratelimit_blocked_total %ld\n"
        "\n"
        "# HELP mod_muse_ai_backends_healthy Number of healthy backend endpoints\n"
        "# TYPE mod_muse_ai_backends_healthy gauge\n"
        "mod_muse_ai_backends_healthy %d\n"
        "\n"
        "# HELP mod_muse_ai_backends_total Total number of configured backend endpoints\n"
        "# TYPE mod_muse_ai_backends_total gauge\n"
        "mod_muse_ai_backends_total %d\n",
        
        metrics->total_requests,
        metrics->successful_requests,
        metrics->failed_requests,
        metrics->cached_responses,
        metrics->avg_response_time_ms / 1000.0,
        metrics->min_response_time_ms / 1000.0,
        metrics->max_response_time_ms / 1000.0,
        metrics->pool_active_connections,
        metrics->pool_idle_connections,
        metrics->pool_total_created,
        metrics->pool_total_reused,
        metrics->ratelimit_blocked_requests,
        metrics->healthy_backends,
        metrics->total_backends
    );
    
    apr_thread_mutex_unlock(metrics_mutex);
    
    return metrics_output;
}

/* Generate JSON metrics output */
char *generate_json_metrics(apr_pool_t *pool)
{
    char *metrics_output;
    muse_ai_metrics_t *metrics = get_global_metrics();
    
    if (!metrics) {
        return apr_pstrdup(pool, "{\"error\": \"Metrics not available\"}");
    }
    
    apr_thread_mutex_lock(metrics_mutex);
    
    metrics_output = apr_psprintf(pool,
        "{\n"
        "  \"requests\": {\n"
        "    \"total\": %ld,\n"
        "    \"successful\": %ld,\n"
        "    \"failed\": %ld,\n"
        "    \"success_rate\": %.2f\n"
        "  },\n"
        "  \"cache\": {\n"
        "    \"hits\": %ld,\n"
        "    \"hit_rate\": %.2f\n"
        "  },\n"
        "  \"response_time_ms\": {\n"
        "    \"avg\": %.2f,\n"
        "    \"min\": %.2f,\n"
        "    \"max\": %.2f\n"
        "  },\n"
        "  \"connection_pool\": {\n"
        "    \"active\": %d,\n"
        "    \"idle\": %d,\n"
        "    \"total_created\": %d,\n"
        "    \"total_reused\": %d,\n"
        "    \"reuse_rate\": %.2f\n"
        "  },\n"
        "  \"rate_limiting\": {\n"
        "    \"blocked_requests\": %ld\n"
        "  },\n"
        "  \"backends\": {\n"
        "    \"healthy\": %d,\n"
        "    \"total\": %d,\n"
        "    \"health_rate\": %.2f\n"
        "  },\n"
        "  \"last_updated\": %lld\n"
        "}",
        
        metrics->total_requests,
        metrics->successful_requests,
        metrics->failed_requests,
        metrics->total_requests > 0 ? (double)metrics->successful_requests / metrics->total_requests * 100.0 : 0.0,
        
        metrics->cached_responses,
        metrics->total_requests > 0 ? (double)metrics->cached_responses / metrics->total_requests * 100.0 : 0.0,
        
        metrics->avg_response_time_ms,
        metrics->min_response_time_ms,
        metrics->max_response_time_ms,
        
        metrics->pool_active_connections,
        metrics->pool_idle_connections,
        metrics->pool_total_created,
        metrics->pool_total_reused,
        (metrics->pool_total_created + metrics->pool_total_reused) > 0 ? 
            (double)metrics->pool_total_reused / (metrics->pool_total_created + metrics->pool_total_reused) * 100.0 : 0.0,
        
        metrics->ratelimit_blocked_requests,
        
        metrics->healthy_backends,
        metrics->total_backends,
        metrics->total_backends > 0 ? (double)metrics->healthy_backends / metrics->total_backends * 100.0 : 0.0,
        
        (long long)metrics->last_updated
    );
    
    apr_thread_mutex_unlock(metrics_mutex);
    
    return metrics_output;
}
