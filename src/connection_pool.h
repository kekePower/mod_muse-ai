#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <httpd.h>
#include <http_config.h>
#include <apr_pools.h>
#include <apr_network_io.h>
#include <apr_thread_mutex.h>
#include <apr_time.h>

/* Connection pool configuration */
#define MUSE_AI_POOL_MAX_CONNECTIONS 10
#define MUSE_AI_POOL_CONNECTION_TIMEOUT 300  /* 5 minutes */
#define MUSE_AI_POOL_IDLE_TIMEOUT 60        /* 1 minute */

/* Connection states */
typedef enum {
    CONN_STATE_IDLE,
    CONN_STATE_ACTIVE,
    CONN_STATE_CLOSED,
    CONN_STATE_ERROR
} connection_state_t;

/* Pooled connection structure */
typedef struct pooled_connection {
    apr_socket_t *socket;
    char *hostname;
    apr_port_t port;
    connection_state_t state;
    apr_time_t last_used;
    apr_time_t created;
    int request_count;
    struct pooled_connection *next;
} pooled_connection_t;

/* Connection pool structure */
typedef struct connection_pool {
    pooled_connection_t *connections;
    int active_count;
    int idle_count;
    int max_connections;
    apr_thread_mutex_t *mutex;
    apr_pool_t *pool;
    server_rec *server;
} connection_pool_t;

/* Function declarations */
connection_pool_t *create_connection_pool(apr_pool_t *pool, server_rec *s);
pooled_connection_t *get_pooled_connection(connection_pool_t *pool, 
                                          const char *hostname, 
                                          apr_port_t port);
int return_pooled_connection(connection_pool_t *pool, 
                           pooled_connection_t *conn);
void cleanup_connection_pool(connection_pool_t *pool);
void log_pool_stats(connection_pool_t *pool, server_rec *s);

#endif /* CONNECTION_POOL_H */
