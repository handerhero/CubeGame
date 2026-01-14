module;

// --- Глобальный фрагмент (заголовки) ---
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <functional>
#include <array>

export module HashMapMod;

// Константа по умолчанию
export constexpr size_t DEFAULT_SHARD_COUNT = 64;

// --------------------------------------------------------
// ShardedMap
// --------------------------------------------------------
export template<typename Key, typename Value, typename Hasher = std::hash<Key>, typename Equal = std::equal_to<Key>, size_t NumShards = DEFAULT_SHARD_COUNT>
class ShardedMap {
private:
    // Внутренняя структура шарда
    struct alignas(64) Shard {
        std::unordered_map<Key, Value, Hasher, Equal> _map;
        mutable std::shared_mutex _mutex;
    };

    std::array<Shard, NumShards> _shards;
    Hasher _hasher;

    // Приватные хелперы (реализованы внутри для простоты шаблона)
    Shard& getShard(const Key& key) {
        size_t hash = _hasher(key);
        return _shards[hash % NumShards];
    }

    const Shard& getShard(const Key& key) const {
        size_t hash = _hasher(key);
        return _shards[hash % NumShards];
    }

public:
    // 1. Безопасное добавление / обновление
    void insert(const Key& key, const Value& value) {
        Shard& shard = getShard(key);
        std::unique_lock<std::shared_mutex> lock(shard._mutex);
        shard._map[key] = value;
    }

    // 2. Безопасное удаление
    void erase(const Key& key) {
        Shard& shard = getShard(key);
        std::unique_lock<std::shared_mutex> lock(shard._mutex);
        shard._map.erase(key);
    }

    // 3. Безопасное получение элемента
    // Value должен быть легко копируемым (shared_ptr или указатель)
    // Если элемента нет - возвращает Value() (обычно nullptr для указателей)
    Value tryGet(const Key& key) const {
        const Shard& shard = getShard(key);
        std::shared_lock<std::shared_mutex> lock(shard._mutex);
        auto it = shard._map.find(key);
        if (it != shard._map.end()) {
            return it->second;
        }
        return Value{}; // Возвращаем дефолтное значение (nullptr)
    }

    // 4. Проверка наличия
    bool contains(const Key& key) const {
        const Shard& shard = getShard(key);
        std::shared_lock<std::shared_mutex> lock(shard._mutex);
        return shard._map.find(key) != shard._map.end();
    }

    // 5. Безопасная итерация
    void forEach(const std::function<void(const Key&, const Value&)>& func) {
        for (auto& shard : _shards) {
            std::shared_lock<std::shared_mutex> lock(shard._mutex);
            for (const auto& pair : shard._map) {
                func(pair.first, pair.second);
            }
        }
    }

    // Получение списка всех ключей
    std::vector<Key> getKeys() const {
        std::vector<Key> keys;
        for (const auto& shard : _shards) {
            std::shared_lock<std::shared_mutex> lock(shard._mutex);
            keys.reserve(keys.size() + shard._map.size());
            for (const auto& pair : shard._map) {
                keys.push_back(pair.first);
            }
        }
        return keys;
    }

    // Снимок всей карты
    std::vector<std::pair<Key, Value>> getSnapshot() const {
        std::vector<std::pair<Key, Value>> result;
        for (const auto& shard : _shards) {
            std::shared_lock<std::shared_mutex> lock(shard._mutex);
            result.reserve(result.size() + shard._map.size());
            for (const auto& pair : shard._map) {
                result.push_back(pair);
            }
        }
        return result;
    }

    void clear() {
        for (auto& shard : _shards) {
            std::unique_lock<std::shared_mutex> lock(shard._mutex);
            shard._map.clear();
        }
    }
};

// --------------------------------------------------------
// ShardedSet
// --------------------------------------------------------
export template<typename Key, typename Hasher = std::hash<Key>, typename Equal = std::equal_to<Key>, size_t NumShards = DEFAULT_SHARD_COUNT>
class ShardedSet {
private:
    struct alignas(64) Shard {
        std::unordered_set<Key, Hasher, Equal> _set;
        mutable std::shared_mutex _mutex;
    };

    std::array<Shard, NumShards> _shards;
    Hasher _hasher;

    Shard& getShard(const Key& key) {
        return _shards[_hasher(key) % NumShards];
    }

    const Shard& getShard(const Key& key) const {
        return _shards[_hasher(key) % NumShards];
    }

public:
    void insert(const Key& key) {
        Shard& shard = getShard(key);
        std::unique_lock<std::shared_mutex> lock(shard._mutex);
        shard._set.insert(key);
    }

    void erase(const Key& key) {
        Shard& shard = getShard(key);
        std::unique_lock<std::shared_mutex> lock(shard._mutex);
        shard._set.erase(key);
    }

    bool contains(const Key& key) const {
        const Shard& shard = getShard(key);
        std::shared_lock<std::shared_mutex> lock(shard._mutex);
        return shard._set.find(key) != shard._set.end();
    }

    size_t count(const Key& key) const {
        return contains(key) ? 1 : 0;
    }

    // Метод очистки для Set тоже пригодится
    void clear() {
        for (auto& shard : _shards) {
            std::unique_lock<std::shared_mutex> lock(shard._mutex);
            shard._set.clear();
        }
    }
};