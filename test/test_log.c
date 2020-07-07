/**
 * @author ttoobne
 * @date 2020/7/3
 */

#include "log.h"

int main() {
    log_trace("this is trace log");
    log_debug("this is debug log");
    log_info("this is info log");
    log_warn("this is warning log");
    log_error("this is error log");
    log_fatal("this is fatal log");

    return 0;
}