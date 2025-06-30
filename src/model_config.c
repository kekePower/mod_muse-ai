#include "model_config.h"
#include "apr_file_io.h"
#include "apr_env.h"
#include "apr_strings.h"
#include "apr_json.h"
#include "httpd.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_config.h"
#include <unistd.h>

/* Global variables */
static muse_ai_models_config *g_models_config = NULL;
static apr_thread_t *g_monitor_thread = NULL;
static int g_monitor_thread_stop = 0;
static const char *g_config_file_path = NULL;
static apr_pool_t *g_config_pool = NULL;

/* Forward declarations */
static apr_status_t load_model_config(apr_pool_t *p, request_rec *r);
static void *APR_THREAD_FUNC model_config_monitor(apr_thread_t *thd, void *data);
static const char *resolve_env_vars(const char *value, apr_pool_t *p);

/* Load model configuration from JSON file */
static apr_status_t load_model_config(apr_pool_t *p, request_rec *r) {
    apr_file_t *file = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;
    apr_json_value_t *json_root = NULL;
    apr_json_value_t *json_models = NULL;
    apr_json_value_t *json_default_model = NULL;
    apr_pool_t *tmp_pool = NULL;
    apr_hash_t *new_models = NULL;
    const char *default_model_name = NULL;
    
    /* Create a temporary pool for parsing */
    rv = apr_pool_create(&tmp_pool, p);
    if (rv != APR_SUCCESS) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mod_muse_ai: Failed to create memory pool for model config parsing");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, "mod_muse_ai: Failed to create memory pool for model config parsing");
        }
        return rv;
    }
    
    /* Open the config file */
    rv = apr_file_open(&file, g_config_file_path, APR_READ, APR_OS_DEFAULT, tmp_pool);
    if (rv != APR_SUCCESS) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mod_muse_ai: Failed to open model config file: %s", g_config_file_path);
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, "mod_muse_ai: Failed to open model config file: %s", g_config_file_path);
        }
        apr_pool_destroy(tmp_pool);
        return rv;
    }
    
    /* Get file info to check modification time */
    rv = apr_file_info_get(&finfo, APR_FINFO_MTIME, file);
    if (rv != APR_SUCCESS) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mod_muse_ai: Failed to get file info for model config file");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, "mod_muse_ai: Failed to get file info for model config file");
        }
        apr_file_close(file);
        apr_pool_destroy(tmp_pool);
        return rv;
    }
    
    /* Check if the file has been modified since last load */
    if (g_models_config->last_mtime == finfo.mtime) {
        /* File hasn't changed, nothing to do */
        apr_file_close(file);
        apr_pool_destroy(tmp_pool);
        return APR_SUCCESS;
    }
    
    /* Parse the JSON file */
    rv = apr_json_parse_file(&json_root, file, tmp_pool);
    apr_file_close(file);
    if (rv != APR_SUCCESS) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mod_muse_ai: Failed to parse model config JSON file");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, "mod_muse_ai: Failed to parse model config JSON file");
        }
        apr_pool_destroy(tmp_pool);
        return rv;
    }
    
    /* Validate that the root is an object */
    if (json_root->type != APR_JSON_OBJECT) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, "mod_muse_ai: Model config JSON root is not an object");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EINVAL, NULL, "mod_muse_ai: Model config JSON root is not an object");
        }
        apr_pool_destroy(tmp_pool);
        return APR_EINVAL;
    }
    
    /* Get the models array */
    json_models = apr_json_object_get(json_root, "models", tmp_pool);
    if (!json_models || json_models->type != APR_JSON_ARRAY) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, "mod_muse_ai: Model config JSON does not contain a valid 'models' array");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EINVAL, NULL, "mod_muse_ai: Model config JSON does not contain a valid 'models' array");
        }
        apr_pool_destroy(tmp_pool);
        return APR_EINVAL;
    }
    
    /* Get the default model name */
    json_default_model = apr_json_object_get(json_root, "default_model", tmp_pool);
    if (!json_default_model || json_default_model->type != APR_JSON_STRING) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, "mod_muse_ai: Model config JSON does not contain a valid 'default_model' string");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EINVAL, NULL, "mod_muse_ai: Model config JSON does not contain a valid 'default_model' string");
        }
        apr_pool_destroy(tmp_pool);
        return APR_EINVAL;
    }
    
    /* Create a new hash table for the models */
    new_models = apr_hash_make(p);
    if (!new_models) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, r, "mod_muse_ai: Failed to create hash table for model configuration");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_ENOMEM, NULL, "mod_muse_ai: Failed to create hash table for model configuration");
        }
        apr_pool_destroy(tmp_pool);
        return APR_ENOMEM;
    }
    
    /* Process each model in the array */
    for (int i = 0; i < json_models->value.array->nelts; i++) {
        apr_json_value_t *json_model = APR_ARRAY_IDX(json_models->value.array, i, apr_json_value_t*);
        apr_json_value_t *json_name, *json_endpoint, *json_api_key, *json_model_name;
        muse_ai_model_config *model_config;
        
        /* Validate that the model is an object */
        if (json_model->type != APR_JSON_OBJECT) {
            if (r) {
                ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "mod_muse_ai: Skipping invalid model entry (not an object) at index %d", i);
            } else {
                ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL, "mod_muse_ai: Skipping invalid model entry (not an object) at index %d", i);
            }
            continue;
        }
        
        /* Get the required fields */
        json_name = apr_json_object_get(json_model, "name", tmp_pool);
        json_endpoint = apr_json_object_get(json_model, "endpoint", tmp_pool);
        json_api_key = apr_json_object_get(json_model, "api_key", tmp_pool);
        json_model_name = apr_json_object_get(json_model, "model", tmp_pool);
        
        /* Validate the required fields */
        if (!json_name || json_name->type != APR_JSON_STRING ||
            !json_endpoint || json_endpoint->type != APR_JSON_STRING ||
            !json_api_key || json_api_key->type != APR_JSON_STRING ||
            !json_model_name || json_model_name->type != APR_JSON_STRING) {
            if (r) {
                ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "mod_muse_ai: Skipping invalid model entry (missing or invalid required fields) at index %d", i);
            } else {
                ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL, "mod_muse_ai: Skipping invalid model entry (missing or invalid required fields) at index %d", i);
            }
            continue;
        }
        
        /* Create a new model config */
        model_config = apr_pcalloc(p, sizeof(muse_ai_model_config));
        if (!model_config) {
            if (r) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_ENOMEM, r, "mod_muse_ai: Failed to allocate memory for model configuration");
            } else {
                ap_log_error(APLOG_MARK, APLOG_ERR, APR_ENOMEM, NULL, "mod_muse_ai: Failed to allocate memory for model configuration");
            }
            apr_pool_destroy(tmp_pool);
            return APR_ENOMEM;
        }
        
        /* Set the model config fields */
        model_config->name = apr_pstrdup(p, json_name->value.string.p);
        model_config->endpoint = apr_pstrdup(p, json_endpoint->value.string.p);
        model_config->api_key = apr_pstrdup(p, json_api_key->value.string.p);
        model_config->model = apr_pstrdup(p, json_model_name->value.string.p);
        
        /* Add the model to the hash table */
        apr_hash_set(new_models, model_config->name, APR_HASH_KEY_STRING, model_config);
        
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, "mod_muse_ai: Loaded model configuration for '%s'", model_config->name);
        } else {
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, NULL, "mod_muse_ai: Loaded model configuration for '%s'", model_config->name);
        }
    }
    
    /* Get the default model name */
    default_model_name = apr_pstrdup(p, json_default_model->value.string.p);
    
    /* Validate that the default model exists */
    if (!apr_hash_get(new_models, default_model_name, APR_HASH_KEY_STRING)) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, "mod_muse_ai: Default model '%s' not found in configuration", default_model_name);
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, APR_EINVAL, NULL, "mod_muse_ai: Default model '%s' not found in configuration", default_model_name);
        }
        apr_pool_destroy(tmp_pool);
        return APR_EINVAL;
    }
    
    /* Lock the mutex for the update */
    rv = apr_thread_mutex_lock(g_models_config->mutex);
    if (rv != APR_SUCCESS) {
        if (r) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, "mod_muse_ai: Failed to lock mutex for model configuration update");
        } else {
            ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, "mod_muse_ai: Failed to lock mutex for model configuration update");
        }
        apr_pool_destroy(tmp_pool);
        return rv;
    }
    
    /* Update the configuration */
    g_models_config->models = new_models;
    g_models_config->default_model = default_model_name;
    g_models_config->last_mtime = finfo.mtime;
    
    /* Unlock the mutex */
    apr_thread_mutex_unlock(g_models_config->mutex);
    
    if (r) {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "mod_muse_ai: Successfully loaded model configuration with %d models", apr_hash_count(new_models));
    } else {
        ap_log_error(APLOG_MARK, APLOG_INFO, 0, NULL, "mod_muse_ai: Successfully loaded model configuration with %d models", apr_hash_count(new_models));
    }
    
    apr_pool_destroy(tmp_pool);
    return APR_SUCCESS;
}

/* Initialize the model configuration system */
apr_status_t init_model_config(apr_pool_t *p) {
    apr_status_t rv;
    
    /* Create a subpool for configuration */
    rv = apr_pool_create(&g_config_pool, p);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to create memory pool for model configuration");
        return rv;
    }
    
    /* Set the config file path */
    g_config_file_path = apr_pstrdup(g_config_pool, "models.json");
    
    /* Create the models configuration structure */
    g_models_config = apr_pcalloc(g_config_pool, sizeof(muse_ai_models_config));
    if (!g_models_config) {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_ENOMEM, NULL, 
                    "mod_muse_ai: Failed to allocate memory for model configuration");
        return APR_ENOMEM;
    }
    
    /* Create the mutex */
    rv = apr_thread_mutex_create(&g_models_config->mutex, APR_THREAD_MUTEX_DEFAULT, g_config_pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, NULL, 
                    "mod_muse_ai: Failed to create mutex for model configuration");
        return rv;
    }
    
    /* Create the models hash table */
    g_models_config->models = apr_hash_make(g_config_pool);
    if (!g_models_config->models) {
        ap_log_error(APLOG_MARK, APLOG_ERR, APR_ENOMEM, NULL, 
                    "mod_muse_ai: Failed to create hash table for model configuration");
        return APR_ENOMEM;
    }
    
    /* Load the initial configuration */
    rv = load_model_config(g_config_pool, NULL);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_WARNING, rv, NULL, 
                    "mod_muse_ai: Failed to load initial model configuration, will retry later");
        /* Continue anyway, we'll retry later */
    }
    
    return APR_SUCCESS;
}
