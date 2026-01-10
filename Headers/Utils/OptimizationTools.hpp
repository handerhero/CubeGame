//
// Created by aspadien on 10.12.2025.
//

#ifndef CUBEREBUILD_OPTIMIZATIONTOOLS_HPP
#define CUBEREBUILD_OPTIMIZATIONTOOLS_HPP

// --- ФАЙЛ: OptimizationTools.hpp ---
#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <thread>

class VramAllocator {
    struct Block {
        uint32_t offset;
        uint32_t size;
    };

    // Вектор вместо map. Это уберет _Rb_tree_increment из профайлера.
    std::vector<Block> freeBlocks;
    uint32_t totalCapacity;
    uint32_t usedMemory = 0;

public:
    explicit VramAllocator(uint32_t size) : totalCapacity(size) {
        freeBlocks.reserve(1024); // Пре-аллокация, чтобы избежать ресайзов
        freeBlocks.push_back({0, size});
    }

    // Аллокация (Best Fit или First Fit)
    uint32_t allocate(uint32_t size) {
        if (size == 0) return 0;

        // Выравнивание размера до 4 байт (опционально, но полезно для GPU)
        // size = (size + 3) & ~3;

        // Линейный проход по памяти (очень быстр благодаря CPU кэшу)
        for (size_t i = 0; i < freeBlocks.size(); ++i) {
            if (freeBlocks[i].size >= size) {
                uint32_t offset = freeBlocks[i].offset;

                if (freeBlocks[i].size == size) {
                    // Идеальное совпадение - удаляем блок swap-ом (быстро)
                    // Но нам нужно сохранять порядок? Нет, для списка свободных не обязательно,
                    // но для слияния лучше держать порядок при вставке.
                    // erase(iter) в векторе может быть медленным, если блоков 10000+.
                    // Но для вокселей обычно freeBlocks < 1000.
                    freeBlocks.erase(freeBlocks.begin() + i);
                } else {
                    // Отрезаем кусок
                    freeBlocks[i].offset += size;
                    freeBlocks[i].size -= size;
                }

                usedMemory += size;
                return offset;
            }
        }

        // Память кончилась
        return 0xFFFFFFFF; // UINT32_MAX
    }

    void free(uint32_t offset, uint32_t size) {
        if (size == 0) return;
        // size = (size + 3) & ~3;
        usedMemory -= size;

        // Находим место для вставки, чтобы список оставался отсортированным по offset
        // Это позволяет делать слияние (дефрагментацию) за O(1)
        auto it = std::lower_bound(freeBlocks.begin(), freeBlocks.end(), offset,
            [](const Block& b, uint32_t val) { return b.offset < val; });

        // Вставляем блок
        it = freeBlocks.insert(it, {offset, size});

        // 1. Попытка слияния со СЛЕДУЮЩИМ блоком
        // it указывает на только что вставленный блок
        auto next = it + 1;
        if (next != freeBlocks.end() && (it->offset + it->size == next->offset)) {
            it->size += next->size;
            freeBlocks.erase(next);
            // После erase итераторы могут инвалидироваться, но мы работаем с индексами или пересчитываем
        }

        // 2. Попытка слияния с ПРЕДЫДУЩИМ блоком
        if (it != freeBlocks.begin()) {
            auto prev = it - 1;
            if (prev->offset + prev->size == it->offset) {
                prev->size += it->size;
                freeBlocks.erase(it);
            }
        }
    }

    // Дебаг инфо
    float getUsage() const { return (float)usedMemory / totalCapacity; }
    size_t getFreeBlockCount() const { return freeBlocks.size(); }
};

// ==========================================
// 2. Priority Thread Pool
// ==========================================
struct Task {
    int priority; // Меньше число = выше важность (дистанция)
    std::function<void()> func;
    bool operator<(const Task& other) const { return priority > other.priority; }
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

    void enqueue(int priority, std::function<void()> f) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.push({priority, std::move(f)});
        }
        condition.notify_one();
    }
};

#endif //CUBEREBUILD_OPTIMIZATIONTOOLS_HPP