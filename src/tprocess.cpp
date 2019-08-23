#include "tprocess.h"

#include <vector>
#include <sys/types.h>
#ifndef WIN32
#include <sys/wait.h>
#include <unistd.h>
#include <sys/signalfd.h>
#else
#include <windows.h>
#include <signal.h>
#endif

#include "signaler.h"
#include "log.h"

using namespace std;

namespace tnet {

Process::Process()
    : m_running(false) {
    m_main = getpid();
#ifndef WIN32
    vector<int> signums{ SIGTERM };
    m_fd = Signaler::createSignalFd(signums);
#endif
}

Process::~Process() {
}

pid_t Process::create() {
#ifndef WIN32
    pid_t pid = fork();
    if (pid < 0) {
        LOG_ERROR("fork error %s", errorMsg(errno));
        return 0;
    } else if (pid == 0) {
        //child
        m_children.clear();
        close(m_fd);
        return getpid();
    } else {
        //parent
        m_children.insert(pid);
    }
#endif
    return 0;
}

void Process::wait(size_t num, const ProcessCallback_t& callback) {
    m_running = true;
#ifndef WIN32
    for (size_t i = 0; i < num; ++i) {
        auto pid = create();
        if (pid != 0) {
            callback();
            return;
        }
    }

    while (!m_children.empty()) {
        int status = 0;
        pid_t pid;
        if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            m_children.erase(pid);

            if (!m_running) {
                continue;
            }

            LOG_INFO("child was dead, restart it");

            if (create() != 0) {
                callback();
                return;
            }
        } else {
            checkStop();
            sleep(1);
            continue;
        }
    }
#endif
    return;
}

void Process::stop() {
    LOG_INFO("stop child process");
#ifndef WIN32
    m_running = false;
    for_each_all(m_children, std::bind(::kill, _1, SIGTERM));
#endif
}

void Process::checkStop() {
#ifndef WIN32
    struct signalfd_siginfo fdsi;
    int s = read(m_fd, &fdsi, sizeof(fdsi));
    if (s != sizeof(fdsi)) {
        return;
    }
    int signum = fdsi.ssi_signo;
    switch (signum) {
        case SIGTERM:
            stop();
            break;
        default:
            LOG_INFO("signum %d", signum);
            break;
    }
#endif
}
}
