#ifndef ADVANCED_STREAMING_H
#define ADVANCED_STREAMING_H

#include <httpd.h>
#include <http_config.h>
#include <apr_pools.h>
#include <apr_buckets.h>

/* Advanced streaming configuration */
#define MUSE_AI_STREAM_BUFFER_SIZE 8192
#define MUSE_AI_STREAM_CHUNK_SIZE 1024
#define MUSE_AI_STREAM_SANITIZE_PATTERNS 10

/* Streaming states */
typedef enum {
    STREAM_STATE_INIT,
    STREAM_STATE_HEADERS_SENT,
    STREAM_STATE_CONTENT,
    STREAM_STATE_SANITIZING,
    STREAM_STATE_COMPLETE,
    STREAM_STATE_ERROR
} stream_state_t;

/* Content sanitization patterns */
typedef struct sanitize_pattern {
    char *pattern;
    char *replacement;
    int is_regex;
    int cross_chunk;  /* Pattern can span multiple chunks */
} sanitize_pattern_t;

/* Advanced streaming context */
typedef struct advanced_stream_context {
    request_rec *r;
    stream_state_t state;
    
    /* Buffer management */
    char *buffer;
    size_t buffer_size;
    size_t buffer_used;
    size_t chunk_size;
    
    /* Cross-chunk pattern handling */
    char *overlap_buffer;
    size_t overlap_size;
    size_t overlap_used;
    
    /* Sanitization */
    sanitize_pattern_t *patterns;
    int pattern_count;
    int sanitization_enabled;
    
    /* Performance tracking */
    apr_time_t start_time;
    size_t bytes_processed;
    size_t chunks_sent;
    
    /* State tracking */
    int html_detected;
    int in_code_block;
    int markdown_cleaned;
    
    /* Reasoning model support */
    int thinking_mode;
    int thinking_detected;
    
} advanced_stream_context_t;

/* Streaming statistics */
typedef struct stream_stats {
    long total_streams;
    long successful_streams;
    long failed_streams;
    double avg_stream_duration_ms;
    size_t total_bytes_streamed;
    size_t avg_bytes_per_stream;
    long sanitization_operations;
    long cross_chunk_patterns_handled;
} stream_stats_t;

/* Function declarations */
advanced_stream_context_t *create_advanced_stream_context(request_rec *r, 
                                                         size_t buffer_size, 
                                                         size_t chunk_size);

int init_stream_sanitization(advanced_stream_context_t *ctx);
int add_sanitization_pattern(advanced_stream_context_t *ctx, 
                           const char *pattern, 
                           const char *replacement,
                           int is_regex,
                           int cross_chunk);

int stream_process_chunk(advanced_stream_context_t *ctx, 
                        const char *data, 
                        size_t data_len);

int stream_send_chunk(advanced_stream_context_t *ctx, 
                     const char *data, 
                     size_t data_len);

int stream_finalize(advanced_stream_context_t *ctx);

/* Sanitization functions */
char *sanitize_streaming_content(advanced_stream_context_t *ctx, 
                               const char *input, 
                               size_t input_len,
                               size_t *output_len);

int detect_html_content(const char *data, size_t len);
int detect_markdown_fences(const char *data, size_t len);
int detect_thinking_tags(const char *data, size_t len);

/* Cross-chunk pattern handling */
int handle_cross_chunk_patterns(advanced_stream_context_t *ctx, 
                              const char *new_data, 
                              size_t new_len);

/* Performance and monitoring */
stream_stats_t *get_stream_stats(void);
void update_stream_stats(advanced_stream_context_t *ctx, int success);
void reset_stream_stats(void);

/* Utility functions */
int is_reasoning_model(const char *model_name);
void set_reasoning_model_patterns(apr_array_header_t *patterns);

#endif /* ADVANCED_STREAMING_H */
