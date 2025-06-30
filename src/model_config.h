#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H

#include "apr_hash.h"
#include "apr_thread_mutex.h"
#include "httpd.h"

/**
 * Structure representing a single AI model configuration
 */
typedef struct {
    const char *name;          /* Model name (identifier) */
    const char *endpoint;      /* API endpoint URL (base URL only, e.g., /v1) */
    const char *api_key;       /* API key (may contain environment variable references) */
    const char *model;         /* Model identifier for the API */
} muse_ai_model_config;

/**
 * Structure representing the global model configuration
 */
typedef struct {
    apr_hash_t *models;           /* Hash table of model configurations */
    const char *default_model;     /* Name of the default model */
    apr_time_t last_mtime;        /* Last modification time of the config file */
    apr_thread_mutex_t *mutex;    /* Mutex for thread-safe access */
} muse_ai_models_config;

/**
 * Initialize the model configuration system
 * @param p Apache memory pool
 * @return APR_SUCCESS on success, error code on failure
 */
apr_status_t init_model_config(apr_pool_t *p);

/**
 * Clean up the model configuration system
 */
void cleanup_model_config(void);

/**
 * Get a model configuration by name
 * @param model_name The name of the model to retrieve
 * @param r The current request (for logging)
 * @return Pointer to the model configuration, or NULL if not found
 */
const muse_ai_model_config *get_model_config(const char *model_name, request_rec *r);

/**
 * Get the default model configuration
 * @param r The current request (for logging)
 * @return Pointer to the default model configuration
 */
const muse_ai_model_config *get_default_model_config(request_rec *r);

/**
 * Get the model configuration from the request environment
 * Uses the MuseAIModel environment variable to select the model
 * @param r The current request
 * @return Pointer to the selected model configuration, or default if not found
 */
const muse_ai_model_config *get_request_model_config(request_rec *r);

/**
 * Reload the model configuration if the file has changed
 * @param r The current request (for logging)
 * @return APR_SUCCESS on success, error code on failure
 */
apr_status_t check_and_reload_model_config(request_rec *r);

/**
 * Start the background thread for monitoring the model configuration file
 * @param p Apache memory pool
 * @return APR_SUCCESS on success, error code on failure
 */
apr_status_t start_model_config_monitor(apr_pool_t *p);

/**
 * Stop the background thread for monitoring the model configuration file
 */
void stop_model_config_monitor(void);

#endif /* MODEL_CONFIG_H */
