#include <algorithm>
#include <deque>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Process {
    std::string name;
    int arrival;
    int burst;
    bool urgent;
    int remaining;
    int queue_level;
    int slice_used;
    int wait_in_queue;
    int completion = -1;
};

int main() {
    const std::vector<int> quantums = {4, 8, 16};
    const int aging_threshold = 32;
    std::vector<Process> processes = {
        {"A", 0, 12, false, 12, 0, 0, 0},
        {"B", 2, 8, false, 8, 0, 0, 0},
        {"C", 5, 32, false, 32, 0, 0, 0},
        {"D", 6, 3, true, 3, 0, 0, 0},
        {"E", 8, 21, false, 21, 0, 0, 0},
        {"F", 9, 34, false, 34, 0, 0, 0},
        {"G", 10, 7, true, 7, 0, 0, 0},
        {"H", 12, 22, false, 22, 0, 0, 0},
        {"I", 15, 10, false, 10, 0, 0, 0},
    };

    std::deque<int> queues[3];
    int completed = 0;
    int time = 0;
    int current = -1;

    std::cout << "实验6：多级反馈队列调度\n";
    std::cout << "时间片: Q1=4, Q2=8, Q3=16, 老化阈值=" << aging_threshold << "\n\n";

    auto enqueue = [&](int idx, int level) {
        processes[idx].queue_level = level;
        processes[idx].slice_used = 0;
        processes[idx].wait_in_queue = 0;
        queues[level].push_back(idx);
    };

    auto dispatch = [&]() {
        for (int level = 0; level < 3; ++level) {
            auto urgent_it = std::find_if(queues[level].begin(), queues[level].end(), [&](int idx) {
                return processes[idx].urgent;
            });
            if (urgent_it != queues[level].end()) {
                current = *urgent_it;
                queues[level].erase(urgent_it);
                std::cout << "t=" << time << " 调度紧急进程 " << processes[current].name << " 进入 Q"
                          << (level + 1) << "\n";
                return;
            }
        }
        for (int level = 0; level < 3; ++level) {
            if (!queues[level].empty()) {
                current = queues[level].front();
                queues[level].pop_front();
                std::cout << "t=" << time << " 调度进程 " << processes[current].name << " 进入 Q"
                          << (level + 1) << "\n";
                return;
            }
        }
        current = -1;
    };

    while (completed < static_cast<int>(processes.size())) {
        bool urgent_arrived = false;
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            if (processes[i].arrival == time) {
                enqueue(i, 0);
                std::cout << "t=" << time << " 进程 " << processes[i].name
                          << (processes[i].urgent ? " [紧急]" : "") << " 到达\n";
                if (processes[i].urgent) {
                    urgent_arrived = true;
                }
            }
        }

        if (urgent_arrived && current != -1) {
            std::cout << "t=" << time << " 紧急进程到达，抢占 " << processes[current].name << "\n";
            queues[processes[current].queue_level].push_back(current);
            current = -1;
        }

        if (current == -1) {
            dispatch();
        }

        if (current == -1) {
            ++time;
            continue;
        }

        for (int level = 0; level < 3; ++level) {
            for (int idx : queues[level]) {
                processes[idx].wait_in_queue += 1;
            }
        }

        for (int level = 1; level < 3; ++level) {
            int count = static_cast<int>(queues[level].size());
            for (int i = 0; i < count; ++i) {
                int idx = queues[level].front();
                queues[level].pop_front();
                if (processes[idx].wait_in_queue >= aging_threshold) {
                    processes[idx].wait_in_queue = 0;
                    processes[idx].slice_used = 0;
                    queues[level - 1].push_back(idx);
                    processes[idx].queue_level = level - 1;
                    std::cout << "t=" << time << " 进程 " << processes[idx].name << " 老化提升到 Q"
                              << level << "\n";
                } else {
                    queues[level].push_back(idx);
                }
            }
        }

        Process& running = processes[current];
        running.remaining -= 1;
        running.slice_used += 1;
        std::cout << "t=" << time << " 运行 " << running.name << ", 剩余 " << running.remaining << "\n";
        ++time;

        if (running.remaining == 0) {
            running.completion = time;
            std::cout << "t=" << time << " 进程 " << running.name << " 完成\n";
            current = -1;
            ++completed;
            continue;
        }

        int level = running.queue_level;
        if (running.slice_used >= quantums[level]) {
            int next_level = (level == 2) ? 2 : level + 1;
            enqueue(current, next_level);
            std::cout << "t=" << time << " 进程 " << running.name << " 时间片用完，转入 Q"
                      << (next_level + 1) << "\n";
            current = -1;
        }
    }

    std::cout << "\n结果统计\n";
    std::cout << std::left << std::setw(8) << "进程"
              << std::setw(10) << "到达"
              << std::setw(10) << "运行"
              << std::setw(10) << "完成"
              << "周转\n";

    double total_turnaround = 0.0;
    for (const auto& process : processes) {
        int turnaround = process.completion - process.arrival;
        total_turnaround += turnaround;
        std::cout << std::left << std::setw(8) << process.name
                  << std::setw(10) << process.arrival
                  << std::setw(10) << process.burst
                  << std::setw(10) << process.completion
                  << turnaround << "\n";
    }
    std::cout << "平均周转时间: " << std::fixed << std::setprecision(2)
              << total_turnaround / processes.size() << "\n";
    return 0;
}
