#ifndef __YIELD_H__
#define __YIELD_H__

#include <stdbool.h>
#include "app_hal.h"

struct _yield_state_t {
    void *yield_ptr;
    bool yielded;
    uint32_t yield_ts;
};

#define _yield_label3(prefix, line) prefix ## line
#define _yield_label2(prefix, line) _yield_label3(prefix, line)
#define _yield_label _yield_label2(YIELD_ENTRY_, __LINE__)

#define YIELDABLE \
    static struct _yield_state_t _yield_state; \
    if (_yield_state.yielded) goto *_yield_state.yield_ptr; \

#define _YIELD(val) \
    do { \
        _yield_state.yield_ptr = &&_yield_label; \
        _yield_state.yielded = true; \
        return val; \
        _yield_label: \
        _yield_state.yielded = false; \
    } while (0)


#define YIELD_END \
    do { \
        _yield_state.yielded = false; \
        return true; \
    } while (0)

#define YIELD_WHILE(cond) \
    do { \
        _yield_state.yield_ts = GET_TIMESTAMP(); \
        while (cond) { _YIELD(false); } \
    } while (0)

#define YIELD_WHILE_WITH_TIMEOUT(cond, timeout) \
    do { \
        _yield_state.yield_ts = GET_TIMESTAMP(); \
        while ((cond) && (GET_TIMESTAMP() < _yield_state.yield_ts + timeout)) { _YIELD(false); } \
    } while (0)

#define YIELD_MS(ms) \
    do { \
        _yield_state.yield_ts = GET_TIMESTAMP(); \
        while (GET_TIMESTAMP() < _yield_state.yield_ts + ms) { _YIELD(false); } \
    } while (0)

#define YIELD_GET_MS() (GET_TIMESTAMP() - _yield_state.yield_ts)

#endif
