#pragma once
#include <algorithm>
#include <cassert>
#include <vector>

template <typename K, typename V> class HashMap {
    std::vector<std::pair<K, V>> items;
public:
    unsigned count() const { return items.size(); }
    bool contains(const K& key) const {
        return std::any_of(items.begin(), items.end(), [&](const auto& item) { return item.first == key; });
    }
    V& operator[](const K& key) {
        for(auto& item : items) if(item.first == key) return item.second;
        items.emplace_back(key, V{});
        return items.back().second;
    }
    const V& operator[](const K& key) const {
        for(const auto& item : items) if(item.first == key) return item.second;
        assert(false);
        static const V empty{};
        return empty;
    }
    const K& keyAt(unsigned index) const { return items.at(index).first; }
    V& valueAt(unsigned index) { return items.at(index).second; }
    const V& valueAt(unsigned index) const { return items.at(index).second; }
    void clear() { items.clear(); }
    void remove(const K& key) {
        items.erase(std::remove_if(items.begin(), items.end(),
            [&](const auto& item) { return item.first == key; }), items.end());
    }
};
