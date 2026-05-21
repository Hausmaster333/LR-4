#ifndef SLIDING_CACHE_H
#define SLIDING_CACHE_H

#include "core/dynamic_array.h"
#include <stdexcept>

template <class T>
class SlidingCache {
    private:
        DynamicArray<T> buffer;
        int capacity;
        int count;
        int first_physical_index; // Физический индекс в buffer, отвечающий за first_logical_index
        size_t first_logical_index; // Лог. индекс начального элемента в окне
        size_t last_logical_index; // Лог. индекс последнего элемента в окне
    public:
        SlidingCache(int capacity);

        bool is_empty() const;

        const T& get(size_t logical_index) const;

        int get_count() const; // Кол-во элементов в кэше
        int get_capacity() const; // Емкость кэша

        size_t get_first_index() const;
        size_t get_last_index() const;

        bool contains(size_t logical_index) const;

        void push(const T& item);

        void clear();
};

#include "sliding_cache.tpp"

#endif
