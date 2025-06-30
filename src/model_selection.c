#include "model_config.h"
#include "apr_env.h"
#include "apr_strings.h"
#include "httpd.h"
#include "http_log.h"
#include "http_protocol.h"

/* Resolve environment variables in a string */
static const char *resolve_env_vars(const char *value, apr_pool_t *p) {
    if (!value || !*value) {
        return value;
    }
    
    /* Check if the value contains an environment variable reference */
    if (strstr(value, "${") && strchr(value, '}')) {
        const char *start, *end;
        char *result = apr_pstrdup(p, "");
        const char *current = value;
        
        while ((start = strstr(current, "${"))) {
            /* Append the text before the variable reference */
            result = apr_pstrncat(p, result, current, start - current);
            
            /* Find the end of the variable reference */
            start += 2; /* Skip the ${ */
            end = strchr(start, '}');
            if (!end) {
                /* Malformed reference, just append the rest and stop */
                result = apr_pstrcat(p, result, start - 2, NULL);
                break;
            }
            
            /* Extract the variable name */
            char *var_name = apr_pstrndup(p, start, end - start);
            
            /* Get the environment variable value */
            char *env_value = NULL;
            apr_env_get(&env_value, var_name, p);
            
            /* Append the environment variable value or empty string if not found */
            result = apr_pstrcat(p, result, env_value ? env_value : "", NULL);
            
            /* Move past the closing brace */
            current = end + 1;
        }
        
        /* Append any remaining text */
        if (*current) {
            result = apr_pstrcat(p, result, current, NULL);
        }
        
        return result;
    }
    
    /* No environment variables to resolve */
    return value;
}

/* Get a model configuration by name */
const muse_ai_model_config *get_model_config(const char *model_name, request_rec *r) {
    extern muse_ai_models_config *g_models_config;
    const muse_ai_model_config *model_config = NULL;
    apr_status_t rv;
    
    if (!g_models_config) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, 
                    "mod_muse_ai: Model configuration system not initialized");
        return NULL;
    }
    
    /* Lock the mutex */
    rv = apr_thread_mutex_lock(g_models_config->mutex);
    if (rv != APR_SUCCESS) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, 
                    "mod_muse_ai: Failed to lock mutex for model configuration access");
        return NULL;
    }
    
    /* Get the model configuration */
    if (model_name && *model_name) {
        model_config = apr_hash_get(g_models_config->models, model_name, APR_HASH_KEY_STRING);
        
        /* If the requested model doesn't exist, fall back to the default */
        if (!model_config) {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, 
                        "mod_muse_ai: Model '%s' not found, falling back to default model '%s'", 
                        model_name, g_models_config->default_model);
            model_config = apr_hash_get(g_models_config->models, g_models_config->default_model, APR_HASH_KEY_STRING);
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                        "mod_muse_ai: Using model '%s'", model_name);
        }
    } else {
        /* No model name specified, use the default */
        model_config = apr_hash_get(g_models_config->models, g_models_config->default_model, APR_HASH_KEY_STRING);
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                    "mod_muse_ai: Using default model '%s'", g_models_config->default_model);
    }
    
    /* Unlock the mutex */
    apr_thread_mutex_unlock(g_models_config->mutex);
    
    /* Resolve environment variables in the API key */
    if (model_config) {
        /* Create a copy of the model config with resolved environment variables */
        muse_ai_model_config *resolved_config = apr_pcalloc(r->pool, sizeof(muse_ai_model_config));
        if (resolved_config) {
            resolved_config->name = model_config->name;
            resolved_config->endpoint = model_config->endpoint;
            resolved_config->model = model_config->model;
            
            /* Resolve environment variables in the API key */
            resolved_config->api_key = resolve_env_vars(model_config->api_key, r->pool);
            
            return resolved_config;
        }
    }
    
    return model_config;
}

/* Get the default model configuration */
const muse_ai_model_config *get_default_model_config(request_rec *r) {
    return get_model_config(NULL, r);
}

/* Get the model configuration from the request environment */
const muse_ai_model_config *get_request_model_config(request_rec *r) {
    const char *model_name = NULL;
    
    /* Check for the MuseAIModel environment variable */
    model_name = apr_table_get(r->subprocess_env, "MuseAIModel");
    
    /* Get the model configuration */
    return get_model_config(model_name, r);
}

/* Check and reload the model configuration if the file has changed */
apr_status_t check_and_reload_model_config(request_rec *r) {
    extern apr_status_t load_model_config(apr_pool_t *p, request_rec *r);
    extern muse_ai_models_config *g_models_config;
    
    if (!g_models_config) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_EINVAL, r, 
                    "mod_muse_ai: Model configuration system not initialized");
        return APR_EINVAL;
    }
    
    return load_model_config(r->pool, r);
}
