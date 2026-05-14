# os-cug-Capstone_Project

`os-cug-Capstone_Project` 是一个基于 C++17 与 POSIX API 的操作系统课程设计示例项目，覆盖题目文档中的实验 1 至实验 7，不包含附录内容。项目以独立可执行程序的形式组织，便于分别编译、运行和验收。

## Features

- 实验1：支持 FCFS、SJF、HRRN 三种作业调度算法，并输出调度顺序、周转时间和带权周转时间。
- 实验2：支持 FCFS、SSTF、SCAN 三种磁盘调度算法，并统计平均移动道数。
- 实验3：基于 Linux 文件系统调用实现 `filetools`，包含文件创建、读写、权限修改与权限查看功能。
- 实验4：基于 `pipe`、`fork`、`signal`、`kill`、`waitpid` 实现父子进程协作与终止控制。
- 实验5：提供统一入口演示管道、消息队列、共享内存三种进程通信方式。
- 实验6：实现带紧急进程抢占与老化提升机制的多级反馈队列调度模拟器。
- 实验7：使用信号量实现公平读者-写者同步，并统计读者、写者平均等待时间。

## Project Layout

```text
.
├── Makefile
├── README.md
├── src
│   ├── exp1_job_scheduling.cpp
│   ├── exp2_disk_scheduling.cpp
│   ├── exp3_filetools.cpp
│   ├── exp4_process_management.cpp
│   ├── exp5_ipc.cpp
│   ├── exp6_mlfq.cpp
│   └── exp7_readers_writers.cpp
└── bin
```

## Build

推荐在 Ubuntu 22.04+/24.04+ 环境中构建。

```bash
make
```

构建完成后，所有可执行文件将输出到 `bin/` 目录。

## Run

```bash
./bin/exp1_job_scheduling
./bin/exp2_disk_scheduling
./bin/exp3_filetools
./bin/exp4_process_management
./bin/exp5_ipc
./bin/exp6_mlfq
./bin/exp7_readers_writers
```

### Experiment 4

实验4为持续运行程序，父进程会等待 `SIGINT` 后通知两个子进程退出。可使用以下方式进行验证：

```bash
./bin/exp4_process_management &
pid=$!
sleep 4
kill -INT "$pid"
wait "$pid"
```

### Experiment 5

实验5为交互式菜单程序，启动后可手动选择：

- `1`：管道通信
- `2`：消息队列通信
- `3`：共享内存通信
- `0`：退出

## Notes

- 实验3按照题目要求使用系统调用进行文件操作，不依赖 C 标准库文件接口。
- 实验7在 macOS 上编译时会出现 `sem_init` 相关弃用警告，这是平台头文件行为；在 Ubuntu 环境下可正常编译与运行。
- 仓库默认聚焦课程设计实现本身，未包含课程报告模板填写内容。
