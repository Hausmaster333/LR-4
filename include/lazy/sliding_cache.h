#ifndef SLIDING_CACHE_H
#define SLIDING_CACHE_H

#include "core/dynamic_array.h"
#include <stdexcept>

// Поддерживает только движение вперёд: contains() проверяет, попадает ли логический индекс в текущее окно, а назад вернуться нельзя
template <class T>
class SlidingCache { // Буфер ограниченной емкости
    private:
        DynamicArray<T> buffer; // Физическое хранилище размера == capacity
        int capacity;
        int count; // Текущее число элементов в окне от 0 до capacity
        int first_physical_index; // Физический индекс в buffer, отвечающий за first_logical_index
        size_t first_logical_index; // Лог. индекс начального элемента в окне
        size_t last_logical_index; // Лог. индекс последнего элемента в окне
    public:
        explicit SlidingCache(int capacity); // Создаёт кэш фиксированной ёмкости

        bool is_empty() const; // Проверка на пустоту

        int get_count() const; // Текущее число элементов в окне
        int get_capacity() const; // Максимальная ёмкость окна (значение constructor-параметра)

        size_t get_first_index() const;
        size_t get_last_index() const;

        bool contains(size_t logical_index) const; // Проверка, что logical_index в окне

        const T& get(size_t logical_index) const;

        void push(const T& item); // Добавляет элемент в конец окна. Если окно заполнено — вытесняет самый старый (first_logical_index сдвигается вправо)
                                  // Новый элемент получает логический индекс = last_logical_index + 1 (или 0 если кэш был пуст)
        void clear(); // Сбрасывает кэш в пустое состояние (count = 0)
};

#include "sliding_cache.tpp"

#endif
