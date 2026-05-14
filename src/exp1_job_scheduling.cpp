#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct Job {
    std::string name;
    int arrival;
    int service;
};

struct ActiveJob {
    int index;
    int start;
    int finish;
};

struct Result {
    std::string algorithm;
    std::vector<int> start_order;
    std::vector<int> start_time;
    std::vector<int> finish_time;
    double avg_turnaround;
    double avg_weighted_turnaround;
};

using Selector = int (*)(const std::vector<Job>&, const std::vector<int>&, int);

int pick_fcfs(const std::vector<Job>& jobs, const std::vector<int>& ready, int) {
    return *std::min_element(ready.begin(), ready.end(), [&](int lhs, int rhs) {
        if (jobs[lhs].arrival != jobs[rhs].arrival) {
            return jobs[lhs].arrival < jobs[rhs].arrival;
        }
        return jobs[lhs].name < jobs[rhs].name;
    });
}

int pick_sjf(const std::vector<Job>& jobs, const std::vector<int>& ready, int) {
    return *std::min_element(ready.begin(), ready.end(), [&](int lhs, int rhs) {
        if (jobs[lhs].service != jobs[rhs].service) {
            return jobs[lhs].service < jobs[rhs].service;
        }
        if (jobs[lhs].arrival != jobs[rhs].arrival) {
            return jobs[lhs].arrival < jobs[rhs].arrival;
        }
        return jobs[lhs].name < jobs[rhs].name;
    });
}

int pick_hrrn(const std::vector<Job>& jobs, const std::vector<int>& ready, int now) {
    return *std::max_element(ready.begin(), ready.end(), [&](int lhs, int rhs) {
        double wait_l = static_cast<double>(now - jobs[lhs].arrival);
        double wait_r = static_cast<double>(now - jobs[rhs].arrival);
        double ratio_l = wait_l / jobs[lhs].service + 1.0;
        double ratio_r = wait_r / jobs[rhs].service + 1.0;
        if (ratio_l != ratio_r) {
            return ratio_l < ratio_r;
        }
        if (jobs[lhs].arrival != jobs[rhs].arrival) {
            return jobs[lhs].arrival > jobs[rhs].arrival;
        }
        return jobs[lhs].name > jobs[rhs].name;
    });
}

Result simulate(const std::string& algorithm, const std::vector<Job>& jobs, Selector selector) {
    const int n = static_cast<int>(jobs.size());
    std::vector<int> start_time(n, -1);
    std::vector<int> finish_time(n, -1);
    std::vector<int> pending;
    pending.reserve(n);
    for (int i = 0; i < n; ++i) {
        pending.push_back(i);
    }

    std::vector<int> ready;
    std::vector<ActiveJob> running;
    std::vector<int> start_order;

    int now = 0;
    auto add_arrivals = [&](int current_time) {
        std::vector<int> rest;
        for (int idx : pending) {
            if (jobs[idx].arrival <= current_time) {
                ready.push_back(idx);
            } else {
                rest.push_back(idx);
            }
        }
        pending.swap(rest);
    };

    while (!pending.empty() || !ready.empty() || !running.empty()) {
        add_arrivals(now);

        while (running.size() < 2 && !ready.empty()) {
            int idx = selector(jobs, ready, now);
            ready.erase(std::find(ready.begin(), ready.end(), idx));
            start_time[idx] = now;
            finish_time[idx] = now + jobs[idx].service;
            start_order.push_back(idx);
            running.push_back({idx, now, finish_time[idx]});
        }

        if (running.empty()) {
            if (!pending.empty()) {
                now = jobs[*std::min_element(pending.begin(), pending.end(), [&](int lhs, int rhs) {
                    if (jobs[lhs].arrival != jobs[rhs].arrival) {
                        return jobs[lhs].arrival < jobs[rhs].arrival;
                    }
                    return jobs[lhs].name < jobs[rhs].name;
                })]
                          .arrival;
            }
            continue;
        }

        int next_finish = running.front().finish;
        for (const auto& item : running) {
            next_finish = std::min(next_finish, item.finish);
        }
        now = next_finish;

        std::vector<ActiveJob> still_running;
        for (const auto& item : running) {
            if (item.finish == now) {
                finish_time[item.index] = item.finish;
            } else {
                still_running.push_back(item);
            }
        }
        running.swap(still_running);
    }

    double total_turnaround = 0.0;
    double total_weighted_turnaround = 0.0;
    for (int i = 0; i < n; ++i) {
        double turnaround = finish_time[i] - jobs[i].arrival;
        total_turnaround += turnaround;
        total_weighted_turnaround += turnaround / jobs[i].service;
    }

    return {algorithm,
            start_order,
            start_time,
            finish_time,
            total_turnaround / n,
            total_weighted_turnaround / n};
}

void print_result(const std::vector<Job>& jobs, const Result& result) {
    std::cout << "=== " << result.algorithm << " ===\n";
    std::cout << "调度顺序: ";
    for (std::size_t i = 0; i < result.start_order.size(); ++i) {
        if (i != 0) {
            std::cout << " -> ";
        }
        std::cout << jobs[result.start_order[i]].name;
    }
    std::cout << "\n";

    std::cout << std::left << std::setw(8) << "作业"
              << std::setw(8) << "到达"
              << std::setw(8) << "运行"
              << std::setw(8) << "开始"
              << std::setw(8) << "完成"
              << std::setw(12) << "周转"
              << "带权周转\n";

    for (std::size_t i = 0; i < jobs.size(); ++i) {
        double turnaround = result.finish_time[i] - jobs[i].arrival;
        double weighted = turnaround / jobs[i].service;
        std::cout << std::left << std::setw(8) << jobs[i].name
                  << std::setw(8) << jobs[i].arrival
                  << std::setw(8) << jobs[i].service
                  << std::setw(8) << result.start_time[i]
                  << std::setw(8) << result.finish_time[i]
                  << std::setw(12) << turnaround
                  << std::fixed << std::setprecision(2) << weighted << "\n";
    }

    std::cout << "平均周转时间: " << std::fixed << std::setprecision(2) << result.avg_turnaround
              << "\n";
    std::cout << "平均带权周转时间: " << std::fixed << std::setprecision(2)
              << result.avg_weighted_turnaround << "\n\n";
}

int main() {
    const std::vector<Job> jobs = {
        {"A", 0, 7},   {"B", 2, 10}, {"C", 5, 20}, {"D", 7, 30}, {"E", 12, 40},
        {"F", 15, 8},  {"G", 4, 8},  {"H", 6, 20}, {"I", 8, 10}, {"J", 10, 12},
    };

    std::cout << "实验1：作业调度（系统可同时运行两道作业）\n\n";
    print_result(jobs, simulate("FCFS", jobs, pick_fcfs));
    print_result(jobs, simulate("SJF", jobs, pick_sjf));
    print_result(jobs, simulate("HRRN", jobs, pick_hrrn));
    return 0;
}
