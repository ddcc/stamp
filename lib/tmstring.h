#ifndef TMSTRING_H
#define TMSTRING_H 1

#include <string.h>

#include "tm.h"

TM_PURE
static inline void *
Pmemcpy(void *dst, const void *src, size_t num) {
    return memcpy(dst, src, num);
}

TM_PURE
static inline int
Pstrncmp(const char *s1, const char *s2, size_t num) {
    return strncmp(s1, s2, num);
}

#endif /* TMSTRING_H */
