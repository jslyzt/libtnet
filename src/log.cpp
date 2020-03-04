#include "log.h"
#include "log_lvl.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include <string>
#include <algorithm>

namespace tnet {

#ifdef WIN32
    __declspec (thread) char errnoMsgBuf[1024];
#else
    __thread char errnoMsgBuf[1024];
#endif

const char* errorMsg(int code) {
#ifdef WIN32
    strerror_s(errnoMsgBuf, sizeof(errnoMsgBuf), code);
    return errnoMsgBuf;
#else
    return strerror_r(code, errnoMsgBuf, sizeof(errnoMsgBuf));
#endif
}

static const int MaxLogMsg = 1024;
static const char* LevelMsg[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#define WRITE_LOG(level)\
    if(m_level <= level)\
    {\
        char msg[MaxLogMsg];\
        va_list ap;\
        va_start(ap, fmt);\
        vsnprintf(msg, sizeof(msg), fmt, ap);\
        va_end(ap);\
        log(LevelMsg[level], file, function, line, msg);\
    }

static const char* DateTimeFormat = "%Y-%m-%d %H:%M:%S";

Log::Log() {
    m_fd = stdout;
    m_level = TRACE;
}

Log::Log(const char* fileName) {
    m_fd = fopen(fileName, "ab+");
    m_level = TRACE;
}

Log::~Log() {
    if (m_fd != stdout) {
        fclose(m_fd);
    }
}

void Log::redirect(const char* fileName) {
    if (m_fd != stdout) {
        fclose(m_fd);
        m_fd = fopen(fileName, "ab+");
    }
}

int Log::getLevel(const char* str) {
    std::string cstr(str);
    std::transform(cstr.begin(), cstr.end(), cstr.begin(), ::toupper);
    for (int lvl = TRACE; lvl <= FATAL; lvl++) {
        if (strcmp(cstr.c_str(), LevelMsg[lvl]) == 0) {
            return lvl;
        }
    }
    return TRACE;
}

void Log::trace(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(TRACE);
}

void Log::debug(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(DEBUG);
}

void Log::info(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(INFO);
}

void Log::warn(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(WARN);
}

void Log::error(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(ERROR);
}

void Log::fatal(const char* file, const char* function, int line, const char* fmt, ...) {
    WRITE_LOG(FATAL);
}

void Log::log(const char* level, const char* file, const char* function, int line, const char* msg) {
    char buf[64];

    time_t now = time(NULL);
    strftime(buf, sizeof(buf), DateTimeFormat, gmtime(&now));

    if (file != nullptr) {
        int pos = 0, fpos = 0;
        while (*(file + pos) != 0) {
            pos++;
            if (*(file + pos) == '\\' || *(file + pos) == '/') {
                fpos = pos;
            }
        }
        if (fpos >= 0 && fpos < pos) {
            file = file + fpos + 1;
        }
    }
    fprintf(m_fd, "%s %s [%d] %s:%d[%s] %s\n", buf, level, getpid(), file, line, function, msg);
}
}
