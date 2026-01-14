// --- Глобальный фрагмент (для старых заголовков) ---
module;

#include <vector>
#include <cstdint>
#include <memory>        // std::shared_ptr
#include <unordered_map> // std::unordered_map
#include <glm/vec3.hpp>  // glm types

// Ваши хеш-мапы (если они заголовочные и не модули)
import HashMapMod;

// --- Объявление модуля ---
export module Chunk;

// --- Определение типов и классов ---


// Определяем хешеры, чтобы они были видны пользователям модуля
export struct GoodVec3Hasher {
    std::size_t operator()(const glm::ivec3& v) const noexcept {
        std::size_t seed = std::hash<int>{}(v.x);
        seed ^= std::hash<int>{}(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

export struct FastIVec3Equal {
    bool operator()(const glm::ivec3& a, const glm::ivec3& b) const noexcept {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

// Экспортируем алиасы типов
export using ChunkMap = ShardedMap<glm::ivec3, std::shared_ptr<class Chunk>, GoodVec3Hasher, FastIVec3Equal>;
export using ChunkSet = ShardedSet<glm::ivec3, GoodVec3Hasher, FastIVec3Equal>;

// Сам класс Chunk
export class Chunk {
public:
    glm::ivec3 worldPosition;
    uint8_t* blocks = nullptr;
    bool needsMeshUpdate = false;

    // void* лучше, чем зависимость от GL заголовков в модуле, если можно избежать
    void* renderInfo = nullptr;
    size_t renderListIndex = -1;

    // Mutex не копируется, поэтому аккуратнее с конструкторами копирования
    // std::mutex mutex; // <mutex> нужно подключить в module; наверху

    explicit Chunk(glm::ivec3 pos);
    ~Chunk();

    [[nodiscard]] uint8_t get(int x, int y, int z) const;
};

// Экспорт функций
export glm::ivec3 getChunkIndex(const glm::vec3 worldPos);
export uint8_t getBlock(const glm::vec3 worldPos, const ChunkMap& chunks);
export bool isSolidBlock(const glm::vec3 pos, ChunkMap& chunks);






