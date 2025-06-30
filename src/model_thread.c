#include "model_config.h"
#include "apr_thread_proc.h"
#include "httpd.h"
#include "http_log.h"

extern void *APR_THREAD_FUNC model_config_monitor(apr_thread_t *thd, void *data);
extern muse_ai_models_config *g_models_config;
extern apr_thread_t *g_monitor_thread;
extern int g_monitor_thread_stop;
extern apr_pool_t *g_config_pool;

/* Start the background thread for monitoring the model configuration file */
apr_status_t start_model_config_monitor(apr_pool_t *p) {
    apr_status_t rv;
    apr_threadattr_t *thread_attr;
    
    /* Create thread attributes */
    rv = apr_threadattr_create(&thread_attr, p);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to create thread attributes for model config monitor");
        return rv;
    }
    
    /* Set thread as detached */
    rv = apr_threadattr_detach_set(thread_attr, 1);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to set thread attributes for model config monitor");
        return rv;
    }
    
    /* Reset the stop flag */
    g_monitor_thread_stop = 0;
    
    /* Create the thread */
    rv = apr_thread_create(&g_monitor_thread, thread_attr, model_config_monitor, &g_monitor_thread_stop, p);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to create model config monitor thread");
        return rv;
    }
    
    ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, 
                "mod_muse_ai: Started model config monitor thread");
    
    return APR_SUCCESS;
}

/* Stop the background thread for monitoring the model configuration file */
void stop_model_config_monitor(void) {
    if (g_monitor_thread) {
        /* Set the stop flag */
        g_monitor_thread_stop = 1;
        
        /* Wait for the thread to exit (it checks the flag periodically) */
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, 
                    "mod_muse_ai: Stopping model config monitor thread");
        
        /* We don't join the thread since it's detached */
        g_monitor_thread = NULL;
    }
}

/* Clean up the model configuration system */
void cleanup_model_config(void) {
    /* Stop the monitor thread */
    stop_model_config_monitor();
    
    /* Clean up the configuration */
    if (g_models_config && g_models_config->mutex) {
        apr_thread_mutex_destroy(g_models_config->mutex);
        g_models_config->mutex = NULL;
    }
    
    /* Destroy the pool */
    if (g_config_pool) {
        apr_pool_destroy(g_config_pool);
        g_config_pool = NULL;
    }
    
    g_models_config = NULL;
}
