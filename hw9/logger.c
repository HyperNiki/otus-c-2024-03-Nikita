#include "logger.h"
#include <pthread.h>
#include <execinfo.h>

static FILE *log_fp = NULL;
static pthread_mutex_t log_mutex;

void init_logger(const char *log_file) {
    pthread_mutex_init(&log_mutex, NULL);
    log_fp = fopen(log_file, "a");
    if (!log_fp) {
        fprintf(stderr, "Failed to open log file %s\n", log_file);
        exit(EXIT_FAILURE);
    }
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    const char *level_strings[] = { "DEBUG", "INFO", "WARNING", "ERROR" };
    va_list args;
    
    pthread_mutex_lock(&log_mutex);
    
    if (log_fp) {
        fprintf(log_fp, "[%s] %s:%d: ", level_strings[level], file, line);
        va_start(args, fmt);
        vfprintf(log_fp, fmt, args);
        va_end(args);
        fprintf(log_fp, "\n");
        
        if (level == LOG_LEVEL_ERROR) {
            void *buffer[100];
            int nptrs = backtrace(buffer, 100);
            char **bt_symbols = backtrace_symbols(buffer, nptrs);
            if (bt_symbols) {
                for (int i = 0; i < nptrs; i++) {
                    fprintf(log_fp, "%s\n", bt_symbols[i]);
                }
                free(bt_symbols);
            }
        }
        
        fflush(log_fp);
    }
    
    pthread_mutex_unlock(&log_mutex);
}
