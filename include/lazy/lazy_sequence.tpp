#ifndef LAZY_SEQUENCE_TPP
#define LAZY_SEQUENCE_TPP

// Подключаем парный .h для IDE/IntelliSense — иначе при открытии .tpp
// напрямую не виден шаблон LazySequence. Header guards защищают от
// бесконечного включения (ниже в lazy_sequence.h этот же .tpp подключается
// в конце своего объявления).
#include "lazy/lazy_sequence.h"

#include <stdexcept>
#include "core/ienumerator.h"

// =====================================================================
// Конструкторы (блок 1: «жадные», без генератора)
// =====================================================================

template <class T>
LazySequence<T>::LazySequence() : length(Cardinal::zero()), generator(nullptr) {}

template <class T>
LazySequence<T>::LazySequence(const T* items, int count)
    : length(Cardinal::zero()), generator(nullptr) {
    if (count < 0) throw std::out_of_range("Count cannot be < 0");

    for (int i = 0; i < count; ++i) {
        materialized.append(items[i]);
    }
    length = Cardinal::finite(static_cast<size_t>(count));
}

template <class T>
LazySequence<T>::LazySequence(const Sequence<T>* seq)
    : length(Cardinal::zero()), generator(nullptr) {
    if (seq == nullptr) throw std::invalid_argument("Cannot create from nullptr sequence");

    EnumeratorWrapper<T> iter(seq->get_enumerator());
    while (iter.move_next()) {
        materialized.append(iter.get_current());
    }
    length = Cardinal::finite(static_cast<size_t>(seq->get_count()));
}

// LazySequence(rule, initial) — заглушка, полная реализация в блоке 2.
template <class T>
LazySequence<T>::LazySequence(std::function<T(Sequence<T>*)> /*rule*/,
                              const Sequence<T>* /*initial*/)
    : length(Cardinal::zero()), generator(nullptr) {
    throw std::runtime_error("LazySequence(rule, initial): not implemented yet (block 2)");
}

// Внутренний конструктор: оборачиваем «недопривязанный» генератор.
template <class T>
LazySequence<T>::LazySequence(Generator<T>* prebuilt_gen, Cardinal length)
    : length(length), generator(prebuilt_gen) {
    if (prebuilt_gen != nullptr) {
        prebuilt_gen->set_owner(this);
    }
}

template <class T>
LazySequence<T>::~LazySequence() {
    delete generator;
}

// =====================================================================
// Приватный API «для генератора»
// =====================================================================

template <class T>
void LazySequence<T>::cache_push(const T& item) {
    materialized.append(item);
}

template <class T>
const T& LazySequence<T>::cache_at(size_t i) const {
    return materialized.get(static_cast<int>(i));
}

template <class T>
size_t LazySequence<T>::cache_size() const {
    return static_cast<size_t>(materialized.get_count());
}

template <class T>
void LazySequence<T>::set_length(Cardinal c) {
    length = c;
}

template <class T>
Cardinal LazySequence<T>::length_hint() const {
    return length;
}

// На блок 1: ничего не материализуем (генератора может не быть).
// Если запрошенного индекса нет в кэше — это ошибка.
// Полная реализация — в блоке 2.
template <class T>
void LazySequence<T>::materialize_up_to(size_t i) {
    if (i >= cache_size()) {
        throw std::out_of_range("materialize_up_to: lazy materialization not implemented yet (block 2)");
    }
}

// =====================================================================
// Декомпозиция
// =====================================================================

template <class T>
Cardinal LazySequence<T>::get_length() const {
    return length;
}

template <class T>
size_t LazySequence<T>::get_materialized_count() const {
    return cache_size();
}

template <class T>
T LazySequence<T>::get_first() {
    if (length == Cardinal::zero()) throw std::out_of_range("Lazy sequence is empty");

    return materialized.get_first();
}

template <class T>
T LazySequence<T>::get_last() {
    if (length == Cardinal::zero()) throw std::out_of_range("Lazy sequence is empty");

    // Блок 1: для жадных конструкторов всё уже в кэше → берём из materialized.
    // Блок 2: для конечных длин нужно материализовать всё до конца.
    return materialized.get_last();
}

template <class T>
T LazySequence<T>::get(int index) {
    if (index < 0) throw std::out_of_range("Lazy sequence index out of range");
    if (length.is_finite() && static_cast<size_t>(index) >= length.get_value()) {
        throw std::out_of_range("Lazy sequence index out of range");
    }

    // Блок 1: для жадных конструкторов кэш совпадает с длиной.
    // Блок 2: вызовем materialize_up_to(index).
    if (static_cast<size_t>(index) >= cache_size()) {
        throw std::out_of_range("Lazy sequence index out of range");
    }

    return materialized.get(index);
}

// =====================================================================
// Операции (блок 1: копирующая семантика, как было; блок 3: через
// ModifyingGenerator)
// =====================================================================

template <class T>
LazySequence<T>* LazySequence<T>::get_sub_sequence(int start, int end) {
    if (start < 0 || end < 0 || start > end || end >= materialized.get_count()) {
        throw std::out_of_range("Subsequence indexes out of range");
    }

    LazySequence<T>* result = new LazySequence<T>();
    EnumeratorWrapper<T> iter(materialized.get_enumerator());
    int curr_idx = 0;
    while (iter.move_next()) {
        if (curr_idx >= start && curr_idx <= end) {
            result->materialized.append(iter.get_current());
        }
        if (curr_idx == end) break;
        curr_idx++;
    }
    result->length = Cardinal::finite(static_cast<size_t>(result->materialized.get_count()));

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::append(const T& item) {
    LazySequence<T>* result = new LazySequence<T>();

    EnumeratorWrapper<T> iter(materialized.get_enumerator());
    while (iter.move_next()) {
        result->materialized.append(iter.get_current());
    }
    result->materialized.append(item);
    result->length = Cardinal::finite(static_cast<size_t>(result->materialized.get_count()));

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::prepend(const T& item) {
    LazySequence<T>* result = new LazySequence<T>();
    result->materialized.append(item);

    EnumeratorWrapper<T> iter(materialized.get_enumerator());
    while (iter.move_next()) {
        result->materialized.append(iter.get_current());
    }
    result->length = Cardinal::finite(static_cast<size_t>(result->materialized.get_count()));

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::insert_at(const T& item, int index) {
    if (index < 0 || index > materialized.get_count()) {
        throw std::out_of_range("Index out of range");
    }

    LazySequence<T>* result = new LazySequence<T>();
    EnumeratorWrapper<T> iter(materialized.get_enumerator());
    int curr_idx = 0;
    while (iter.move_next()) {
        if (curr_idx == index) {
            result->materialized.append(item);
        }
        result->materialized.append(iter.get_current());
        curr_idx++;
    }
    if (index == materialized.get_count()) {
        result->materialized.append(item);
    }
    result->length = Cardinal::finite(static_cast<size_t>(result->materialized.get_count()));

    return result;
}

#endif
