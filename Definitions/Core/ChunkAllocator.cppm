module;

#include <algorithm>
#include <mutex>
#include <vector>

export module ChunkAllocator;
// Настройки кэша
// Сколько блоков хранить локально в каждом потоке (без блокировок)
constexpr size_t THREAD_CACHE_SIZE = 16;
// Сколько блоков перемещать между локальным и глобальным пулом за раз
constexpr size_t BATCH_MOVE_SIZE = 8;

export class ChunkAllocator {
private:
    std::vector<uint8_t*> globalPool; // Вектор быстрее стека
    std::mutex globalMutex;

    // Внутренняя структура для локального хранилища
    struct ThreadCache {
        std::vector<uint8_t*> blocks;

        // Деструктор очищает память, если поток умирает
        ~ThreadCache();
    };

public:
    static ChunkAllocator& Get();

    uint8_t* Allocate();

    void Free(uint8_t* ptr);

    // Очистка при выходе (опционально, ОС все равно почистит)
    ~ChunkAllocator();
};

