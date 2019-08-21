#pragma once

#include <sys/types.h>
#include <stdarg.h>

typedef char* sds;

struct sdshdr {
    int len;
    int free;
    char buf[];
};

sds sdsnewlen(const void* init, size_t initlen);
sds sdsnew(const char* init);
sds sdsempty(void);
size_t sdslen(const sds s);
sds sdsdup(const sds s);
void sdsfree(sds s);
size_t sdsavail(const sds s);
sds sdsgrowzero(sds s, size_t len);
sds sdscatlen(sds s, const void* t, size_t len);
sds sdscat(sds s, const char* t);
sds sdscpylen(sds s, char* t, size_t len);
sds sdscpy(sds s, char* t);

sds sdscatvprintf(sds s, const char* fmt, va_list ap);
#ifdef __GNUC__
sds sdscatprintf(sds s, const char* fmt, ...)
__attribute__((format(printf, 2, 3)));
#else
sds sdscatprintf(sds s, const char* fmt, ...);
#endif

sds sdstrim(sds s, const char* cset);
sds sdsrange(sds s, int start, int end);
void sdsupdatelen(sds s);
int sdscmp(sds s1, sds s2);
sds* sdssplitlen(char* s, int len, char* sep, int seplen, int* count);
void sdsfreesplitres(sds* tokens, int count);
void sdstolower(sds s);
void sdstoupper(sds s);
sds sdsfromlonglong(long long value);
sds sdscatrepr(sds s, char* p, size_t len);
sds* sdssplitargs(char* line, int* argc);
