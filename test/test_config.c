/**
 * @author ttoobne
 * @date 2020/7/4
 */

#include "config.h"

#include <stdio.h>

int main() {
    config_t* config;

    config = parse_configuration("bohttpd.conf");

    printf("threadpool: %d\n", config->threadpool);
    printf("taskqueue: %d\n", config->taskqueue);
    printf("root: %s\n", config->root);
    printf("defile: %s\n", config->defile);
    printf("timeout: %d\n", config->timeout);
    printf("port: %d\n", config->port);
    return 0;
}