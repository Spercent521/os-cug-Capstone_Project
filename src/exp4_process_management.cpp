#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

int pipe_fd[2] = {-1, -1};
pid_t child1_pid = -1;
pid_t child2_pid = -1;
volatile sig_atomic_t stop_requested = 0;

void safe_write(const std::string& text) {
    const char* data = text.c_str();
    std::size_t left = text.size();
    while (left > 0) {
        ssize_t written = ::write(STDOUT_FILENO, data, left);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        data += written;
        left -= static_cast<std::size_t>(written);
    }
}

void parent_sigint_handler(int) {
    stop_requested = 1;
}

void child1_exit_handler(int) {
    const char* msg = "Child Process 1 is killed by Parent!\n";
    ::write(STDOUT_FILENO, msg, std::strlen(msg));
    _exit(0);
}

void child2_exit_handler(int) {
    const char* msg = "Child Process 2 is killed by Parent!\n";
    ::write(STDOUT_FILENO, msg, std::strlen(msg));
    _exit(0);
}

void child1_main() {
    ::signal(SIGUSR1, child1_exit_handler);
    ::close(pipe_fd[0]);
    int count = 1;
    while (true) {
        char buffer[128];
        int len = std::snprintf(buffer, sizeof(buffer), "I send message %d times.\n", count++);
        if (::write(pipe_fd[1], buffer, static_cast<std::size_t>(len)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        ::sleep(1);
    }
    _exit(0);
}

void child2_main() {
    ::signal(SIGUSR1, child2_exit_handler);
    ::close(pipe_fd[1]);
    char buffer[128];
    while (true) {
        ssize_t n = ::read(pipe_fd[0], buffer, sizeof(buffer));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (n == 0) {
            break;
        }
        ssize_t offset = 0;
        while (offset < n) {
            ssize_t written = ::write(STDOUT_FILENO, buffer + offset, static_cast<std::size_t>(n - offset));
            if (written < 0) {
                if (errno == EINTR) {
                    continue;
                }
                _exit(1);
            }
            offset += written;
        }
    }
    _exit(0);
}

}  // namespace

int main() {
    if (::pipe(pipe_fd) < 0) {
        std::perror("pipe");
        return 1;
    }

    ::signal(SIGINT, parent_sigint_handler);

    child1_pid = ::fork();
    if (child1_pid < 0) {
        std::perror("fork child1");
        return 1;
    }
    if (child1_pid == 0) {
        child1_main();
    }

    child2_pid = ::fork();
    if (child2_pid < 0) {
        std::perror("fork child2");
        return 1;
    }
    if (child2_pid == 0) {
        child2_main();
    }

    ::close(pipe_fd[0]);
    ::close(pipe_fd[1]);

    while (!stop_requested) {
        ::pause();
    }

    ::kill(child1_pid, SIGUSR1);
    ::kill(child2_pid, SIGUSR1);

    int status = 0;
    ::waitpid(child1_pid, &status, 0);
    ::waitpid(child2_pid, &status, 0);
    safe_write("Parent Process is Killed!\n");
    return 0;
}
