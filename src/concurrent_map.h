#pragma once
#include <type_traits>
#include <numeric>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>

template <typename Key, typename Value>
class ConcurrentMap 
{
    struct Access 
    {
        Access(Value& value, std::mutex&& mutex)
            : guard(mutex, std::adopt_lock)
            , ref_to_value(value) 
        {
        }
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    explicit ConcurrentMap(size_t bucket_count = std::thread::hardware_concurrency())
        : maps_(bucket_count) 
    {
    }

    Access operator[](const Key& key) 
    {
        auto& [map, mutex] = maps_[static_cast<size_t>(key) % maps_.size()];
        mutex.lock();
        return { map[key], std::move(mutex) };
    }

    void Erase(const Key& key) 
    {
        auto& [map, mutex] = maps_[static_cast<size_t>(key) % maps_.size()];
        std::lock_guard guard(mutex);
        map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() 
    {
        std::map<Key, Value> result;
        for (auto& [map, mutex] : maps_) 
        {
            std::lock_guard<std::mutex> guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<std::pair<std::map<Key, Value>, std::mutex>> maps_;
};