#ifndef SLIDING_CACHE_TPP
#define SLIDING_CACHE_TPP

#include "lazy/sliding_cache.h"

template <class T>
SlidingCache<T>::SlidingCache(int capacity) : buffer(capacity), capacity(capacity), count(0), first_physical_index(0), first_logical_index(0), last_logical_index(0) {
    if (capacity < 1) throw std::invalid_argument("Capacity must be >= 1");
}

template <class T>
bool SlidingCache<T>::is_empty() const {
    return count == 0;
}

template <class T>
int SlidingCache<T>::get_count() const {
    return count;
}

template <class T>
int SlidingCache<T>::get_capacity() const {
    return capacity;
}

template <class T>
size_t SlidingCache<T>::get_first_index() const {
    if (count == 0) throw std::logic_error("Cache is empty");

    return first_logical_index;
}

template <class T>
size_t SlidingCache<T>::get_last_index() const {
    if (count == 0) throw std::logic_error("Cache is empty");

    return last_logical_index;
}

template <class T>
bool SlidingCache<T>::contains(size_t logical_index) const {
    if (count == 0) return false;

    return logical_index >= first_logical_index && logical_index <= last_logical_index;
}

template <class T>
const T& SlidingCache<T>::get(size_t logical_index) const {
    if (!contains(logical_index)) throw std::out_of_range("Index not in window");

    size_t offset = logical_index - first_logical_index;
    int physical_index = (first_physical_index + static_cast<int>(offset)) % capacity;

    return buffer.get(physical_index);
}

template <class T>
void SlidingCache<T>::push(const T& item) {
    if (count == 0) {
        buffer.set(0, item);
        first_physical_index = 0;
        first_logical_index = 0;
        last_logical_index = 0;
        count = 1;
        return;
    }

    if (count < capacity) {
        int new_physical_index = (first_physical_index + count) % capacity;
        buffer.set(new_physical_index, item);
        last_logical_index++;
        count++;
    } else {
        buffer.set(first_physical_index, item);
        first_physical_index = (first_physical_index + 1) % capacity;
        first_logical_index++;
        last_logical_index++;
    }
}

template <class T>
void SlidingCache<T>::clear() {
    first_physical_index = 0;
    first_logical_index = 0;
    last_logical_index = 0;
    count = 0;
}

#endif
