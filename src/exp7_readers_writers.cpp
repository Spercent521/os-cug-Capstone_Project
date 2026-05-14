#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Operation {
    int start_ms;
    int duration_ms;
};

struct ThreadPlan {
    std::string name;
    bool is_reader;
    std::vector<Operation> ops;
};

struct SharedState {
    sem_t mutex;
    sem_t reader_queue;
    sem_t writer_queue;
    int active_readers = 0;
    bool writer_active = false;
    int waiting_readers = 0;
    int waiting_writers = 0;
    int delayed_writer_count = 0;
    bool writer_priority = false;
    double total_reader_wait_ms = 0.0;
    double total_writer_wait_ms = 0.0;
    int reader_ops = 0;
    int writer_ops = 0;
    std::chrono::steady_clock::time_point start_time;
    pthread_mutex_t log_mutex;
};

struct ThreadContext {
    const ThreadPlan* plan;
    SharedState* state;
};

using Clock = std::chrono::steady_clock;

double elapsed_ms(const SharedState* state) {
    return std::chrono::duration<double, std::milli>(Clock::now() - state->start_time).count();
}

void log_line(SharedState* state, const std::string& text) {
    pthread_mutex_lock(&state->log_mutex);
    std::string line = text + "\n";
    ::write(STDOUT_FILENO, line.c_str(), line.size());
    pthread_mutex_unlock(&state->log_mutex);
}

void wake_all_readers(SharedState* state) {
    for (int i = 0; i < state->waiting_readers; ++i) {
        sem_post(&state->reader_queue);
    }
}

void reader_enter(SharedState* state, const std::string& name, double& wait_ms) {
    const auto wait_begin = Clock::now();
    sem_wait(&state->mutex);
    state->waiting_readers += 1;
    while (state->writer_active || (state->writer_priority && state->waiting_writers > 0)) {
        sem_post(&state->mutex);
        sem_wait(&state->reader_queue);
        sem_wait(&state->mutex);
    }
    if (state->waiting_writers > 0 && !state->writer_priority) {
        state->delayed_writer_count += 1;
        if (state->delayed_writer_count >= 3) {
            state->writer_priority = true;
        }
    }
    state->waiting_readers -= 1;
    state->active_readers += 1;
    sem_post(&state->mutex);

    wait_ms = std::chrono::duration<double, std::milli>(Clock::now() - wait_begin).count();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << "t=" << elapsed_ms(state) << "ms " << name
        << " starts reading";
    log_line(state, oss.str());
}

void reader_leave(SharedState* state, const std::string& name) {
    sem_wait(&state->mutex);
    state->active_readers -= 1;
    if (state->active_readers == 0 && state->waiting_writers > 0) {
        sem_post(&state->writer_queue);
    }
    sem_post(&state->mutex);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << "t=" << elapsed_ms(state) << "ms " << name
        << " ends reading";
    log_line(state, oss.str());
}

void writer_enter(SharedState* state, const std::string& name, double& wait_ms) {
    const auto wait_begin = Clock::now();
    sem_wait(&state->mutex);
    state->waiting_writers += 1;
    while (state->writer_active || state->active_readers > 0) {
        sem_post(&state->mutex);
        sem_wait(&state->writer_queue);
        sem_wait(&state->mutex);
    }
    state->waiting_writers -= 1;
    state->writer_active = true;
    state->delayed_writer_count = 0;
    state->writer_priority = false;
    sem_post(&state->mutex);

    wait_ms = std::chrono::duration<double, std::milli>(Clock::now() - wait_begin).count();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << "t=" << elapsed_ms(state) << "ms " << name
        << " starts writing";
    log_line(state, oss.str());
}

void writer_leave(SharedState* state, const std::string& name) {
    sem_wait(&state->mutex);
    state->writer_active = false;
    if (state->waiting_writers > 0 && state->writer_priority) {
        sem_post(&state->writer_queue);
    } else if (state->waiting_readers > 0) {
        wake_all_readers(state);
    } else if (state->waiting_writers > 0) {
        sem_post(&state->writer_queue);
    }
    sem_post(&state->mutex);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << "t=" << elapsed_ms(state) << "ms " << name
        << " ends writing";
    log_line(state, oss.str());
}

void* thread_main(void* arg) {
    auto* ctx = static_cast<ThreadContext*>(arg);
    const ThreadPlan& plan = *ctx->plan;
    SharedState* state = ctx->state;

    for (const auto& op : plan.ops) {
        while (elapsed_ms(state) < op.start_ms) {
            usleep(1000);
        }

        double wait_ms = 0.0;
        if (plan.is_reader) {
            reader_enter(state, plan.name, wait_ms);
            usleep(static_cast<useconds_t>(op.duration_ms * 1000));
            reader_leave(state, plan.name);

            sem_wait(&state->mutex);
            state->total_reader_wait_ms += wait_ms;
            state->reader_ops += 1;
            sem_post(&state->mutex);
        } else {
            writer_enter(state, plan.name, wait_ms);
            usleep(static_cast<useconds_t>(op.duration_ms * 1000));
            writer_leave(state, plan.name);

            sem_wait(&state->mutex);
            state->total_writer_wait_ms += wait_ms;
            state->writer_ops += 1;
            sem_post(&state->mutex);
        }
    }
    return nullptr;
}

int main() {
    SharedState state;
    sem_init(&state.mutex, 0, 1);
    sem_init(&state.reader_queue, 0, 0);
    sem_init(&state.writer_queue, 0, 0);
    state.start_time = Clock::now();
    pthread_mutex_init(&state.log_mutex, nullptr);

    const std::vector<ThreadPlan> plans = {
        {"Writer1", false, {{0, 100}, {400, 200}, {800, 200}, {1100, 200}}},
        {"Writer2", false, {{300, 100}, {700, 200}}},
        {"Reader1", true, {{100, 400}, {900, 300}}},
        {"Reader2", true, {{200, 100}, {600, 300}}},
        {"Reader3", true, {{500, 400}, {1000, 400}}},
    };

    std::vector<pthread_t> threads(plans.size());
    std::vector<ThreadContext> contexts(plans.size());

    std::cout << "实验7：信号量与读者-写者问题\n";
    std::cout << "策略：允许并发读，写者独占；当等待中的写者连续被3次新读者延迟后，后续调度转为写者优先。\n\n";

    for (std::size_t i = 0; i < plans.size(); ++i) {
        contexts[i] = {&plans[i], &state};
        pthread_create(&threads[i], nullptr, thread_main, &contexts[i]);
    }

    for (pthread_t thread : threads) {
        pthread_join(thread, nullptr);
    }

    std::cout << "\n统计结果\n";
    std::cout << "读者平均等待时间: " << std::fixed << std::setprecision(2)
              << (state.reader_ops == 0 ? 0.0 : state.total_reader_wait_ms / state.reader_ops) << " ms\n";
    std::cout << "写者平均等待时间: " << std::fixed << std::setprecision(2)
              << (state.writer_ops == 0 ? 0.0 : state.total_writer_wait_ms / state.writer_ops) << " ms\n";

    sem_destroy(&state.mutex);
    sem_destroy(&state.reader_queue);
    sem_destroy(&state.writer_queue);
    pthread_mutex_destroy(&state.log_mutex);
    return 0;
}
