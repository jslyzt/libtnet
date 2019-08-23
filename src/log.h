#pragma once

#include <stdio.h>
#include <errno.h>

#include "nocopyable.h"

namespace tnet {
class Log : public nocopyable {
public:
    enum Level {
        TRACE = 0,
        DEBUG,
        INFO,
        WARN,
        Error,
        FATAL,
    };

    Log();
    Log(const char* fileName);
    ~Log();

    static Log& rootLog() {
        static Log log;
        return log;
    }

    void redirect(const char* fileName);

    void setLevel(Level level) { m_level = level; }
    Level getLevel() { return m_level; }

    void trace(const char* file, const char* function, int line, const char* fmt, ...);
    void debug(const char* file, const char* function, int line, const char* fmt, ...);
    void info(const char* file, const char* function, int line, const char* fmt, ...);
    void warn(const char* file, const char* function, int line, const char* fmt, ...);
    void error(const char* file, const char* function, int line, const char* fmt, ...);
    void fatal(const char* file, const char* function, int line, const char* fmt, ...);

private:
    void log(const char* level, const char* file, const char* function, int line, const char* msg);

private:
    FILE* m_fd;
    Level m_level;
};

#ifdef WIN32
#define LOG_TRACE(fmt, ...) Log::rootLog().trace("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...) Log::rootLog().debug("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) Log::rootLog().info("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) Log::rootLog().warn("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) Log::rootLog().error("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#define LOG_FATAL(fmt, ...) Log::rootLog().fatal("", __FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#else
#define LOG_TRACE(fmt, args...) Log::rootLog().trace("", __FUNCTION__, __LINE__, fmt, ##args)
#define LOG_DEBUG(fmt, args...) Log::rootLog().debug("", __FUNCTION__, __LINE__, fmt, ##args)
#define LOG_INFO(fmt, args...) Log::rootLog().info("", __FUNCTION__, __LINE__, fmt, ##args)
#define LOG_WARN(fmt, args...) Log::rootLog().warn("", __FUNCTION__, __LINE__, fmt, ##args)
#define LOG_ERROR(fmt, args...) Log::rootLog().error("", __FUNCTION__, __LINE__, fmt, ##args)
#define LOG_FATAL(fmt, args...) Log::rootLog().fatal("", __FUNCTION__, __LINE__, fmt, ##args)
#endif


const char* errorMsg(int code);
}

