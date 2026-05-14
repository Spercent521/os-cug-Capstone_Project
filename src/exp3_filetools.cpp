#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

namespace {

void write_all(int fd, const std::string& text) {
    const char* data = text.c_str();
    std::size_t left = text.size();
    while (left > 0) {
        ssize_t written = ::write(fd, data, left);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        data += written;
        left -= static_cast<std::size_t>(written);
    }
}

void print_error(const std::string& prefix) {
    std::string msg = prefix + ": " + std::strerror(errno) + "\n";
    write_all(STDERR_FILENO, msg);
}

std::string read_line(const std::string& prompt) {
    write_all(STDOUT_FILENO, prompt);
    std::string line;
    char ch = 0;
    while (true) {
        ssize_t n = ::read(STDIN_FILENO, &ch, 1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return {};
        }
        if (n == 0 || ch == '\n') {
            break;
        }
        line.push_back(ch);
    }
    return line;
}

int parse_int(const std::string& text, int base = 10) {
    int value = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return -1;
        }
        int digit = ch - '0';
        if (digit >= base) {
            return -1;
        }
        value = value * base + digit;
    }
    return value;
}

void create_file() {
    std::string path = read_line("输入文件路径: ");
    int fd = ::open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd < 0) {
        print_error("创建文件失败");
        return;
    }
    ::close(fd);
    write_all(STDOUT_FILENO, "创建成功\n");
}

void write_file() {
    std::string path = read_line("输入文件路径: ");
    std::string id = read_line("记录ID: ");
    std::string name = read_line("姓名: ");
    std::string score = read_line("分数: ");
    std::string record = id + "|" + name + "|" + score + "\n";

    int fd = ::open(path.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd < 0) {
        print_error("打开文件失败");
        return;
    }
    write_all(fd, record);
    ::close(fd);
    write_all(STDOUT_FILENO, "写入成功\n");
}

void read_file() {
    std::string path = read_line("输入文件路径: ");
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        print_error("打开文件失败");
        return;
    }

    write_all(STDOUT_FILENO, "文件内容如下:\n");
    char buffer[256];
    while (true) {
        ssize_t n = ::read(fd, buffer, sizeof(buffer));
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            print_error("读取文件失败");
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
                print_error("输出失败");
                ::close(fd);
                return;
            }
            offset += written;
        }
    }
    ::close(fd);
}

void chmod_file() {
    std::string path = read_line("输入文件路径: ");
    std::string mode_text = read_line("输入八进制权限(如 644): ");
    int mode_value = parse_int(mode_text, 8);
    if (mode_value < 0) {
        write_all(STDERR_FILENO, "权限格式错误\n");
        return;
    }
    if (::chmod(path.c_str(), static_cast<mode_t>(mode_value)) < 0) {
        print_error("修改权限失败");
        return;
    }
    write_all(STDOUT_FILENO, "修改权限成功\n");
}

void show_permission() {
    std::string path = read_line("输入文件路径: ");
    pid_t pid = ::fork();
    if (pid < 0) {
        print_error("fork失败");
        return;
    }
    if (pid == 0) {
        char* args[4];
        args[0] = const_cast<char*>("ls");
        args[1] = const_cast<char*>("-l");
        args[2] = const_cast<char*>(path.c_str());
        args[3] = nullptr;
        ::execv("/bin/ls", args);
        _exit(1);
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
}

}  // namespace

int main() {
    while (true) {
        write_all(STDOUT_FILENO,
                  "\nfiletools 功能菜单\n"
                  "1. 创建新文件\n"
                  "2. 写文件\n"
                  "3. 读文件\n"
                  "4. 修改文件权限\n"
                  "5. 查看当前文件权限\n"
                  "0. 退出\n");
        std::string choice = read_line("输入功能号: ");
        if (choice == "1") {
            create_file();
        } else if (choice == "2") {
            write_file();
        } else if (choice == "3") {
            read_file();
        } else if (choice == "4") {
            chmod_file();
        } else if (choice == "5") {
            show_permission();
        } else if (choice == "0") {
            break;
        } else {
            write_all(STDERR_FILENO, "无效选项\n");
        }
    }
    return 0;
}
