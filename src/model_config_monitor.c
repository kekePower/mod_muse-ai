/* Background thread for monitoring model configuration file changes */
#include "model_config.h"
#include "apr_thread_proc.h"
#include "apr_time.h"
#include "httpd.h"
#include "http_log.h"

extern apr_status_t load_model_config(apr_pool_t *p, request_rec *r);

/* Background thread function for monitoring the model configuration file */
void *APR_THREAD_FUNC model_config_monitor(apr_thread_t *thd, void *data) {
    apr_pool_t *pool;
    apr_status_t rv;
    int *stop_flag = (int *)data;
    
    /* Create a pool for this thread */
    rv = apr_pool_create(&pool, NULL);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to create memory pool for model config monitor thread");
        return NULL;
    }
    
    ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, 
                "mod_muse_ai: Model config monitor thread started");
    
    /* Monitor loop */
    while (!*stop_flag) {
        /* Check if the config file has changed */
        rv = load_model_config(pool, NULL);
        if (rv != APR_SUCCESS) {
            ap_log_error(APLOG_MARK, APLOG_WARNING, rv, NULL, 
                        "mod_muse_ai: Failed to reload model configuration");
        }
        
        /* Sleep for 5 seconds */
        apr_sleep(5 * 1000000);
    }
    
    ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, 
                "mod_muse_ai: Model config monitor thread stopped");
    
    /* Clean up */
    apr_pool_destroy(pool);
    
    return NULL;
}
