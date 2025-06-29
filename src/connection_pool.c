#include "connection_pool.h"
#include <apr_strings.h>
#include <apr_network_io.h>
#include <http_log.h>

/* Global connection pool instance */
static connection_pool_t *global_pool = NULL;

/* Create a new connection pool */
connection_pool_t *create_connection_pool(apr_pool_t *pool, server_rec *s)
{
    connection_pool_t *conn_pool;
    apr_status_t rv;
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Creating connection pool with max %d connections", 
                MUSE_AI_POOL_MAX_CONNECTIONS);
    
    conn_pool = apr_pcalloc(pool, sizeof(connection_pool_t));
    if (!conn_pool) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, 
                    "[mod_muse_ai] Failed to allocate connection pool");
        return NULL;
    }
    
    /* Initialize pool structure */
    conn_pool->connections = NULL;
    conn_pool->active_count = 0;
    conn_pool->idle_count = 0;
    conn_pool->max_connections = MUSE_AI_POOL_MAX_CONNECTIONS;
    conn_pool->pool = pool;
    conn_pool->server = s;
    
    /* Create mutex for thread safety */
    rv = apr_thread_mutex_create(&conn_pool->mutex, APR_THREAD_MUTEX_DEFAULT, pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, s, 
                    "[mod_muse_ai] Failed to create connection pool mutex");
        return NULL;
    }
    
    /* Set global pool reference */
    global_pool = conn_pool;
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s, 
                "[mod_muse_ai] Connection pool created successfully");
    
    return conn_pool;
}

/* Find an existing connection or create a new one */
pooled_connection_t *get_pooled_connection(connection_pool_t *pool, 
                                          const char *hostname, 
                                          apr_port_t port)
{
    pooled_connection_t *conn = NULL;
    pooled_connection_t *prev = NULL;
    apr_time_t now = apr_time_now();
    apr_status_t rv;
    
    if (!pool || !hostname) {
        return NULL;
    }
    
    /* Lock the pool */
    apr_thread_mutex_lock(pool->mutex);
    
    /* Look for an existing idle connection to the same host:port */
    conn = pool->connections;
    while (conn) {
        if (conn->state == CONN_STATE_IDLE &&
            conn->port == port &&
            strcmp(conn->hostname, hostname) == 0) {
            
            /* Check if connection is still fresh */
            if ((now - conn->last_used) < (MUSE_AI_POOL_IDLE_TIMEOUT * APR_USEC_PER_SEC)) {
                /* Reuse this connection */
                conn->state = CONN_STATE_ACTIVE;
                conn->last_used = now;
                conn->request_count++;
                pool->idle_count--;
                pool->active_count++;
                
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
                           "[mod_muse_ai] Reusing pooled connection to %s:%d (requests: %d)",
                           hostname, port, conn->request_count);
                
                apr_thread_mutex_unlock(pool->mutex);
                return conn;
            } else {
                /* Connection is stale, remove it */
                ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
                           "[mod_muse_ai] Removing stale connection to %s:%d",
                           hostname, port);
                
                if (prev) {
                    prev->next = conn->next;
                } else {
                    pool->connections = conn->next;
                }
                
                if (conn->socket) {
                    apr_socket_close(conn->socket);
                }
                
                pool->idle_count--;
                conn = prev ? prev->next : pool->connections;
                continue;
            }
        }
        prev = conn;
        conn = conn->next;
    }
    
    /* No suitable connection found, create a new one if we have capacity */
    if ((pool->active_count + pool->idle_count) >= pool->max_connections) {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, pool->server,
                   "[mod_muse_ai] Connection pool exhausted (max: %d)", 
                   pool->max_connections);
        apr_thread_mutex_unlock(pool->mutex);
        return NULL;
    }
    
    /* Create new connection */
    conn = apr_pcalloc(pool->pool, sizeof(pooled_connection_t));
    if (!conn) {
        apr_thread_mutex_unlock(pool->mutex);
        return NULL;
    }
    
    /* Initialize connection structure */
    conn->hostname = apr_pstrdup(pool->pool, hostname);
    conn->port = port;
    conn->state = CONN_STATE_ACTIVE;
    conn->created = now;
    conn->last_used = now;
    conn->request_count = 1;
    conn->socket = NULL;
    
    /* Create socket */
    rv = apr_socket_create(&conn->socket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, pool->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, pool->server,
                   "[mod_muse_ai] Failed to create socket for %s:%d", hostname, port);
        apr_thread_mutex_unlock(pool->mutex);
        return NULL;
    }
    
    /* Set socket options for keep-alive and performance */
    apr_socket_opt_set(conn->socket, APR_SO_KEEPALIVE, 1);
    apr_socket_opt_set(conn->socket, APR_SO_REUSEADDR, 1);
    apr_socket_timeout_set(conn->socket, MUSE_AI_POOL_CONNECTION_TIMEOUT * APR_USEC_PER_SEC);
    
    /* Connect to the backend */
    apr_sockaddr_t *sa;
    rv = apr_sockaddr_info_get(&sa, hostname, APR_INET, port, 0, pool->pool);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, pool->server,
                   "[mod_muse_ai] Failed to resolve %s:%d", hostname, port);
        apr_socket_close(conn->socket);
        apr_thread_mutex_unlock(pool->mutex);
        return NULL;
    }
    
    rv = apr_socket_connect(conn->socket, sa);
    if (rv != APR_SUCCESS) {
        ap_log_error(APLOG_MARK, APLOG_ERR, rv, pool->server,
                   "[mod_muse_ai] Failed to connect to %s:%d", hostname, port);
        apr_socket_close(conn->socket);
        apr_thread_mutex_unlock(pool->mutex);
        return NULL;
    }
    
    /* Add to pool */
    conn->next = pool->connections;
    pool->connections = conn;
    pool->active_count++;
    
    ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
               "[mod_muse_ai] Created new pooled connection to %s:%d (pool: %d/%d)",
               hostname, port, pool->active_count + pool->idle_count, pool->max_connections);
    
    apr_thread_mutex_unlock(pool->mutex);
    return conn;
}

/* Return a connection to the pool */
int return_pooled_connection(connection_pool_t *pool, pooled_connection_t *conn)
{
    if (!pool || !conn) {
        return -1;
    }
    
    apr_thread_mutex_lock(pool->mutex);
    
    if (conn->state == CONN_STATE_ACTIVE) {
        conn->state = CONN_STATE_IDLE;
        conn->last_used = apr_time_now();
        pool->active_count--;
        pool->idle_count++;
        
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
                   "[mod_muse_ai] Returned connection to %s:%d to pool (idle: %d, active: %d)",
                   conn->hostname, conn->port, pool->idle_count, pool->active_count);
    }
    
    apr_thread_mutex_unlock(pool->mutex);
    return 0;
}

/* Clean up expired connections */
void cleanup_connection_pool(connection_pool_t *pool)
{
    pooled_connection_t *conn, *prev = NULL, *next;
    apr_time_t now = apr_time_now();
    int cleaned = 0;
    
    if (!pool) {
        return;
    }
    
    apr_thread_mutex_lock(pool->mutex);
    
    conn = pool->connections;
    while (conn) {
        next = conn->next;
        
        /* Clean up idle connections that have expired */
        if (conn->state == CONN_STATE_IDLE &&
            (now - conn->last_used) > (MUSE_AI_POOL_IDLE_TIMEOUT * APR_USEC_PER_SEC)) {
            
            /* Remove from list */
            if (prev) {
                prev->next = next;
            } else {
                pool->connections = next;
            }
            
            /* Close socket and clean up */
            if (conn->socket) {
                apr_socket_close(conn->socket);
            }
            
            pool->idle_count--;
            cleaned++;
            
            ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
                       "[mod_muse_ai] Cleaned up expired connection to %s:%d",
                       conn->hostname, conn->port);
        } else {
            prev = conn;
        }
        
        conn = next;
    }
    
    if (cleaned > 0) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, 0, pool->server,
                   "[mod_muse_ai] Connection pool cleanup: removed %d expired connections",
                   cleaned);
    }
    
    apr_thread_mutex_unlock(pool->mutex);
}

/* Log connection pool statistics */
void log_pool_stats(connection_pool_t *pool, server_rec *s)
{
    if (!pool) {
        return;
    }
    
    apr_thread_mutex_lock(pool->mutex);
    
    ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, s,
               "[mod_muse_ai] Connection Pool Stats: Active=%d, Idle=%d, Max=%d",
               pool->active_count, pool->idle_count, pool->max_connections);
    
    apr_thread_mutex_unlock(pool->mutex);
}

/* Get global connection pool instance */
connection_pool_t *get_global_connection_pool(void)
{
    return global_pool;
}
