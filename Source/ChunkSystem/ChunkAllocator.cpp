module;

#include <algorithm>
#include <mutex>
#include <vector>
#include "Core/Constants.hpp"

module ChunkAllocator;

        // Деструктор очищает память, если поток умирает
       ChunkAllocator::ThreadCache::~ThreadCache() {
            for (uint8_t* ptr : blocks) {
                delete[] ptr;
            }
        }


    static ChunkAllocator& Get() {
        static ChunkAllocator instance;
        return instance;
    }

    uint8_t* ChunkAllocator::Allocate() {
        // thread_local - у каждого потока своя копия этой переменной.
        // Доступ к ней НЕ ТРЕБУЕТ мьютексов.
        static thread_local ThreadCache tls;

        // 1. Пытаемся взять из локального кэша (БЫСТРО, 0 блокировок)
        if (!tls.blocks.empty()) {
            uint8_t* ptr = tls.blocks.back();
            tls.blocks.pop_back();
            return ptr;
        }

        // 2. Локальный кэш пуст. Идем в глобальный пул (МЕДЛЕННО, блокировка)
        // Но мы берем сразу пачку (BATCH), чтобы не приходить сюда часто.
        {
            std::lock_guard<std::mutex> lock(globalMutex);
            if (!globalPool.empty()) {
                size_t count = std::min(globalPool.size(), BATCH_MOVE_SIZE);
                for (size_t i = 0; i < count; ++i) {
                    tls.blocks.push_back(globalPool.back());
                    globalPool.pop_back();
                }
            }
        }

        // 3. Снова проверяем локальный кэш после пополнения
        if (!tls.blocks.empty()) {
            uint8_t* ptr = tls.blocks.back();
            tls.blocks.pop_back();
            return ptr;
        }

        // 4. Если и в глобальном пусто — выделяем новую память
        return new uint8_t[CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z];
    }

    void ChunkAllocator::Free(uint8_t* ptr) {
        if (!ptr) return;

        static thread_local ThreadCache tls;

        // 1. Кладем в локальный кэш (БЫСТРО, 0 блокировок)
        if (tls.blocks.size() < THREAD_CACHE_SIZE) {
            tls.blocks.push_back(ptr);
            return;
        }

        // 2. Локальный кэш переполнен. Сбрасываем часть в глобальный пул.
        {
            std::lock_guard<std::mutex> lock(globalMutex);

            // Переносим пачку блоков из локального в глобальный
            for (size_t i = 0; i < BATCH_MOVE_SIZE; ++i) {
                globalPool.push_back(tls.blocks.back());
                tls.blocks.pop_back();
            }

            // Теперь место освободилось, кладем текущий указатель
            tls.blocks.push_back(ptr);
        }
    }

    // Очистка при выходе (опционально, ОС все равно почистит)
    ChunkAllocator::~ChunkAllocator() {
        std::lock_guard<std::mutex> lock(globalMutex);
        for (uint8_t* ptr : globalPool) {
            delete[] ptr;
        }
        globalPool.clear();
    }


 ChunkAllocator& ChunkAllocator::Get() {
           static ChunkAllocator instance;
           return instance;
       }

