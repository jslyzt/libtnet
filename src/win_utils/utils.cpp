#include "utils.h"
#include <ctype.h>
#include <string.h>

void close(int) {
}

char* strcasestr(const char* s, const char* find) {
    char c, sc;
    size_t len;
    if ((c = *find++) != 0) {
        c = tolower((unsigned char)c);
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == 0) {
                    return nullptr;
                }
            } while ((char)tolower((unsigned char)sc) != c);
        } while (_strnicmp(s, find, len) != 0);
        s--;
    }
    return ((char*)s);
}
