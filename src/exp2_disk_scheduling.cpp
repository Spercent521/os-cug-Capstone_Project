#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

struct SequenceResult {
    std::string name;
    std::vector<int> order;
    int total_movement;
};

SequenceResult fcfs(const std::vector<int>& requests, int start) {
    int head = start;
    int movement = 0;
    for (int track : requests) {
        movement += std::abs(track - head);
        head = track;
    }
    return {"FCFS", requests, movement};
}

SequenceResult sstf(const std::vector<int>& requests, int start) {
    std::vector<int> remaining = requests;
    std::vector<int> order;
    int head = start;
    int movement = 0;
    while (!remaining.empty()) {
        auto chosen = remaining.begin();
        for (auto it = remaining.begin(); it != remaining.end(); ++it) {
            int dist_it = std::abs(*it - head);
            int dist_chosen = std::abs(*chosen - head);
            if (dist_it < dist_chosen) {
                chosen = it;
            }
        }
        movement += std::abs(*chosen - head);
        head = *chosen;
        order.push_back(*chosen);
        remaining.erase(chosen);
    }
    return {"SSTF", order, movement};
}

SequenceResult scan(const std::vector<int>& requests, int start) {
    std::vector<int> up;
    std::vector<int> down;
    for (int track : requests) {
        if (track >= start) {
            up.push_back(track);
        } else {
            down.push_back(track);
        }
    }
    std::sort(up.begin(), up.end());
    std::sort(down.begin(), down.end(), std::greater<int>());

    std::vector<int> order;
    order.insert(order.end(), up.begin(), up.end());
    order.insert(order.end(), down.begin(), down.end());

    int head = start;
    int movement = 0;
    for (int track : order) {
        movement += std::abs(track - head);
        head = track;
    }
    return {"SCAN", order, movement};
}

void print_result(const SequenceResult& result, int start) {
    std::cout << "=== " << result.name << " ===\n";
    std::cout << "访问顺序: " << start;
    for (int track : result.order) {
        std::cout << " -> " << track;
    }
    std::cout << "\n";
    std::cout << "总移动道数: " << result.total_movement << "\n";
    std::cout << "平均移动道数: " << std::fixed << std::setprecision(2)
              << static_cast<double>(result.total_movement) / result.order.size() << "\n\n";
}

int main() {
    const std::vector<int> requests = {30, 50, 100, 180, 20, 90, 150, 70, 80, 10, 160, 120, 40, 110};
    const int start = 90;

    std::cout << "实验2：磁盘调度\n\n";
    print_result(fcfs(requests, start), start);
    print_result(sstf(requests, start), start);
    print_result(scan(requests, start), start);
    return 0;
}
