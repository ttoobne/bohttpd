/**
 * @author ttoobne
 * @date 2020/6/2
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHOW_ERROR() (errno == 0 ? "NONE" : strerror(errno))

#define ERROR(M) \
    do {\
        fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, SHOW_ERROR());\
    } while (0)

#define ASSERT(A, M) if(!(A)) { ERROR(M "\n"); exit(1); }

#ifndef DEBUG
#define DBG(...) \
    do {\
        fprintf(stderr, " DBG(%s, %s(), %d): ", __FILE__,__FUNCTION__,__LINE__);\
        fprintf(stderr, __VA_ARGS__);\
    } while (0)
#else 
#define DBG(...)
#endif /* DEBUG */

#endif /* _DEBUG_H_ */
