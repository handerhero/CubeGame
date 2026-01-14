module;
#include <vector>
#include <cstdint>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
export module VramAllocator;

export class VRamAllocator {
    struct Block {
        uint32_t offset;
        uint32_t size;
    };

    // Вектор вместо map. Это уберет _Rb_tree_increment из профайлера.
    std::vector<Block> freeBlocks;
    uint32_t totalCapacity;
    uint32_t usedMemory = 0;

public:
    explicit VRamAllocator(uint32_t size);

    // Аллокация (Best Fit или First Fit)
    uint32_t allocate(uint32_t size);

    void free(uint32_t offset, uint32_t size);

    // Дебаг инфо
    [[nodiscard]] float getUsage() const;
    [[nodiscard]] size_t getFreeBlockCount() const;
};

// ==========================================
// 2. Priority Thread Pool
// ==========================================
export struct Task {
    int priority; // Меньше число = выше важность (дистанция)
    std::function<void()> func;
    bool operator<(const Task& other) const { return priority > other.priority; }
};

export class PriorityThreadPool {
    std::vector<std::thread> workers;
    std::priority_queue<Task> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};

public:
    PriorityThreadPool(size_t threads);

    ~PriorityThreadPool();

    void enqueue(int priority, std::function<void()> f);
};
