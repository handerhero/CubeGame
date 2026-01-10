//
// Created by aspadien on 26.11.2025.
//

#ifndef CUBEREBUILD_HASH_MAP_MODIFICATION_HPP
#define CUBEREBUILD_HASH_MAP_MODIFICATION_HPP

#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>

// Шаблонный класс для потокобезопасной карты
template<typename Key, typename Value, typename Hasher, typename Equal>
class ThreadSafeMap {
private:
    std::unordered_map<Key, Value, Hasher, Equal> _map;
    mutable std::shared_mutex _mutex; // mutable позволяет блокировать в const методах

public:
    // 1. Безопасное добавление / обновление
    void insert(const Key& key, const Value& value) {
        std::unique_lock<std::shared_mutex> lock(_mutex); // Блокируем на запись
        _map[key] = value;
    }

    // 2. Безопасное удаление
    void erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(_mutex); // Блокируем на запись
        _map.erase(key);
    }

    // 3. Безопасное получение элемента (Поиск)
    // Возвращает копию shared_ptr, что ГАРАНТИРУЕТ, что чанк не удалится, пока вы им пользуетесь
    Value tryGet(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(_mutex); // Блокируем на чтение (быстро)
        auto it = _map.find(key);
        if (it != _map.end()) {
            return it->second;
        }
        return nullptr; // Или пустой shared_ptr
    }

    // 4. Проверка наличия (быстрая)
    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _map.find(key) != _map.end();
    }

    // 5. Безопасная итерация (для chunkFinder)
    // Мы не можем просто вернуть итератор, так как карта может измениться.
    // Поэтому мы позволяем выполнить функцию для каждого элемента под защитой.
    void forEach(const std::function<void(const Key&, const Value&)>& func) {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        for (const auto& pair : _map) {
            func(pair.first, pair.second);
        }
    }

    // Альтернатива для chunkFinder: Получить снимок всех ключей (чтобы быстро освободить лок)
    std::vector<Key> getKeys() const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        std::vector<Key> keys;
        keys.reserve(_map.size());
        for (const auto& pair : _map) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    // Возвращает вектор пар (Позиция, Чанк).
    // shared_ptr копируется быстро, сам чанк не дублируется.
    std::vector<std::pair<Key, Value>> getSnapshot() const {
        std::shared_lock<std::shared_mutex> lock(_mutex); // Блокируем чтение
        std::vector<std::pair<Key, Value>> result;
        result.reserve(_map.size());
        // Копируем пары в вектор
        for (const auto& pair : _map) {
            result.push_back(pair);
        }
        return result;
    }
};

template<typename Key, typename Hasher, typename Equal>
class ThreadSafeSet {
private:
    std::unordered_set<Key, Hasher, Equal> _set;
    mutable std::shared_mutex _mutex;

public:
    void insert(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _set.insert(key);
    }

    void erase(const Key& key) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _set.erase(key);
    }

    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _set.find(key) != _set.end();
    }

    // Для совместимости с кодом, который проверяет .count()
    size_t count(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _set.count(key);
    }
};

#endif //CUBEREBUILD_HASH_MAP_MODIFICATION_HPP