#pragma once

#ifdef WIN32
#include <process.h>
#include "../win_utils/utils.h"
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <set>

#include "tnet.h"

namespace tnet {

class Process {
public:
    Process();
    ~Process();

    void wait(size_t num, const ProcessCallback_t& callback);

    void stop();

    bool isMainProc() { return m_main == getpid(); }

    bool hasChild() { return m_children.size() > 0; }

private:
    pid_t create();
    void checkStop();

private:
    pid_t m_main;
    bool m_running;
    std::set<pid_t> m_children;

    int m_fd;
};

}
