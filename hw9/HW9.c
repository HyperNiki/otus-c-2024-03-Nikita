#include "logger.h"

int main() {
    init_logger("app.log");
    
    log_debug("This is a debug message");
    log_info("This is an info message");
    log_warning("This is a warning message");
    log_error("This is an error message");
    
    return 0;
}
