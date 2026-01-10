//
// Created by aspadien on 10.12.2025.
//

#ifndef CUBEREBUILD_PRIORITYTHREADPOOL_HPP
#define CUBEREBUILD_PRIORITYTHREADPOOL_HPP


#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

// Задача с приоритетом. Чем МЕНЬШЕ число priority, тем БЫСТРЕЕ выполнится.
struct Task {
    int priority; // Дистанция до игрока (в чанках ^ 2)
    std::function<void()> func;

    // Оператор для priority_queue (обратный порядок, чтобы min было наверху)
    bool operator<(const Task& other) const {
        return priority > other.priority;
    }
};

class PriorityThreadPool {
    std::vector<std::thread> workers;
    std::priority_queue<Task> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};

public:
    PriorityThreadPool(size_t threads) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while(true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if(stop && tasks.empty()) return;

                        task = std::move(tasks.top());
                        tasks.pop();
                    }
                    // Выполняем задачу вне лока
                    task.func();
                }
            });
        }
    }

    ~PriorityThreadPool() {
        stop = true;
        condition.notify_all();
        for(auto& worker : workers) if(worker.joinable()) worker.join();
    }

    // Добавить задачу. priority = дистанция до игрока (чем меньше, тем важнее)
    void enqueue(int priority, std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.push({priority, std::move(f)});
        }
        condition.notify_one();
    }

    // Очистить очередь (например, при телепортации)
    void clear() {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks = std::priority_queue<Task>();
    }
};

#endif //CUBEREBUILD_PRIORITYTHREADPOOL_HPP