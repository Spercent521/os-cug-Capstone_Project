#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

constexpr key_t kMsgKey = 75;
constexpr long kRequestType = 1;
constexpr key_t kShmKey = 76;
constexpr std::size_t kShmSize = 512;

struct Message {
    long mtype;
    pid_t pid;
};

void demo_pipe() {
    std::cout << "\n[管道通信]\n";
    int fd[2];
    if (::pipe(fd) < 0) {
        std::perror("pipe");
        return;
    }

    pid_t pid = ::fork();
    if (pid < 0) {
        std::perror("fork");
        return;
    }
    if (pid == 0) {
        ::close(fd[0]);
        std::string message = "Child " + std::to_string(::getpid()) + " is sending a message to parent";
        ::write(fd[1], message.c_str(), message.size() + 1);
        ::close(fd[1]);
        _exit(0);
    }

    ::close(fd[1]);
    char buffer[256] = {};
    ::read(fd[0], buffer, sizeof(buffer));
    std::cout << "Parent received: " << buffer << "\n";
    ::close(fd[0]);
    ::waitpid(pid, nullptr, 0);
}

void server_process() {
    int msqid = ::msgget(kMsgKey, IPC_CREAT | 0666);
    if (msqid < 0) {
        std::perror("msgget server");
        _exit(1);
    }

    Message request{};
    if (::msgrcv(msqid, &request, sizeof(request.pid), kRequestType, 0) < 0) {
        std::perror("msgrcv");
        _exit(1);
    }

    std::cout << "serving for client " << request.pid << std::endl;
    Message reply{};
    reply.mtype = request.pid;
    reply.pid = ::getpid();
    if (::msgsnd(msqid, &reply, sizeof(reply.pid), 0) < 0) {
        std::perror("msgsnd reply");
        _exit(1);
    }
    _exit(0);
}

void client_process() {
    int msqid = ::msgget(kMsgKey, IPC_CREAT | 0666);
    if (msqid < 0) {
        std::perror("msgget client");
        _exit(1);
    }

    Message request{};
    request.mtype = kRequestType;
    request.pid = ::getpid();
    if (::msgsnd(msqid, &request, sizeof(request.pid), 0) < 0) {
        std::perror("msgsnd request");
        _exit(1);
    }

    Message reply{};
    if (::msgrcv(msqid, &reply, sizeof(reply.pid), ::getpid(), 0) < 0) {
        std::perror("msgrcv reply");
        _exit(1);
    }
    std::cout << "receive reply from " << reply.pid << std::endl;
    _exit(0);
}

void demo_message_queue() {
    std::cout << "\n[消息队列通信]\n";
    ::msgctl(::msgget(kMsgKey, IPC_CREAT | 0666), IPC_RMID, nullptr);

    pid_t server = ::fork();
    if (server < 0) {
        std::perror("fork server");
        return;
    }
    if (server == 0) {
        server_process();
    }

    ::usleep(100000);
    pid_t client = ::fork();
    if (client < 0) {
        std::perror("fork client");
        return;
    }
    if (client == 0) {
        client_process();
    }

    ::waitpid(server, nullptr, 0);
    ::waitpid(client, nullptr, 0);
    int msqid = ::msgget(kMsgKey, 0666);
    if (msqid >= 0) {
        ::msgctl(msqid, IPC_RMID, nullptr);
    }
}

void demo_shared_memory() {
    std::cout << "\n[共享内存通信]\n";
    int shmid = ::shmget(kShmKey, kShmSize, IPC_CREAT | 0666);
    if (shmid < 0) {
        std::perror("shmget");
        return;
    }

    void* addr = ::shmat(shmid, nullptr, 0);
    if (addr == reinterpret_cast<void*>(-1)) {
        std::perror("shmat parent");
        return;
    }

    char* shared = static_cast<char*>(addr);
    std::snprintf(shared, kShmSize, "Process A(%d) wrote: hello from shared memory", ::getpid());
    std::cout << shared << "\n";

    pid_t child = ::fork();
    if (child < 0) {
        std::perror("fork");
        return;
    }
    if (child == 0) {
        void* child_addr = ::shmat(shmid, nullptr, 0);
        if (child_addr == reinterpret_cast<void*>(-1)) {
            std::perror("shmat child");
            _exit(1);
        }
        char* child_shared = static_cast<char*>(child_addr);
        std::snprintf(child_shared, kShmSize, "Process B(%d) updated shared memory", ::getpid());
        std::cout << child_shared << std::endl;
        ::shmdt(child_addr);
        _exit(0);
    }

    ::waitpid(child, nullptr, 0);
    std::cout << "Process A sees: " << shared << "\n";
    ::shmdt(addr);
    ::shmctl(shmid, IPC_RMID, nullptr);
}

}  // namespace

int main() {
    while (true) {
        std::cout << "\n实验5：进程通信\n"
                  << "1. 管道\n"
                  << "2. 消息队列\n"
                  << "3. 共享内存\n"
                  << "0. 退出\n"
                  << "请选择: ";
        int choice = -1;
        if (!(std::cin >> choice)) {
            return 0;
        }
        if (choice == 1) {
            demo_pipe();
        } else if (choice == 2) {
            demo_message_queue();
        } else if (choice == 3) {
            demo_shared_memory();
        } else if (choice == 0) {
            return 0;
        } else {
            std::cout << "无效选项\n";
        }
    }
}
