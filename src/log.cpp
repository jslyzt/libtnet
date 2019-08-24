#include "log.h"

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
        log(LevelMsg[int(level)], file, function, line, msg);\
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

Log::Level Log::getLevel(const char* str) {
    std::string cstr(str);
    std::transform(cstr.begin(), cstr.end(), cstr.begin(), ::toupper);
    for (int lvl = TRACE; lvl <= FATAL; lvl++) {
        if (strcmp(cstr.c_str(), LevelMsg[lvl]) == 0) {
            return (Log::Level)lvl;
        }
    }
    return Log::TRACE;
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
    WRITE_LOG(Error);
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
        while (*file != 0) {
            pos++;
            if (*file == '\\' || *file == '/') {
                fpos = pos;
            }
        }
        if (fpos >= 0 && fpos < pos) {
            file = file + fpos;
        }
    }
    fprintf(m_fd, "%s %s [%d] %s %s:%d %s\n", buf, level, getpid(), function, file, line, msg);
}
}
