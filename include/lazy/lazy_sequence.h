#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include <functional>
#include <cstddef>
#include "core/sequence.h"
#include "core/option.h"
#include "lazy/cardinal.h"

// Forward declaration: для поля Generator<T>* generator достаточно.
// Полное определение подключается в конце файла (после объявления LazySequence),
// чтобы в generator.tpp был доступен полный тип LazySequence.
template <class T> class Generator;

template <class T>
class LazySequence {
    template <class U>
    friend class Generator;
private:
    MutableArraySequence<T> materialized; // Элементы, находящиеся в памяти
    Cardinal length;                      // Сколько элементов в последовательности вообще
    Generator<T>* generator;              // Один на хозяина, без публичного геттера

    // Приватный API «для генератора»: вся работа с внутренностями хозяина
    // идёт строго через эти методы (Generator — friend).
    void cache_push(const T& item);
    const T& cache_at(size_t i) const;
    size_t cache_size() const;
    void set_length(Cardinal c);
    Cardinal length_hint() const;

    // Гарантирует, что в кэше материализовано минимум (i+1) элементов.
    // Дёргает generator->get_next() сколько нужно. Используется в get(i).
    void materialize_up_to(size_t i);

    // Внутренний конструктор: оборачивает уже построенный «недопривязанный»
    // генератор (owner == nullptr) в новый LazySequence заданной длины.
    // Используется в реализации append/insert_at/concat/map/where/zip,
    // где результат gen->append(...) и т.п. нужно сделать хозяином чего-то.
    LazySequence(Generator<T>* prebuilt_gen, Cardinal length);
public:
    // Создание объекта (по ТЗ)
    LazySequence();
    LazySequence(const T* items, int count);
    LazySequence(const Sequence<T>* seq);
    LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial);

    LazySequence(const LazySequence& other) = delete;
    LazySequence& operator=(const LazySequence& other) = delete;

    ~LazySequence();

    // Декомпозиция
    Cardinal get_length() const;
    size_t get_materialized_count() const; // Сколько элементов реально материализовано

    T get_first();
    T get_last();                          // Для бесконечной — не завершается
    T get(int index);

    LazySequence<T>* get_sub_sequence(int start, int end);

    // Операции (по ТЗ): возвращают новые LazySequence без полной материализации
    LazySequence<T>* append(const T& item);
    LazySequence<T>* prepend(const T& item);
    LazySequence<T>* insert_at(const T& item, int index);

    LazySequence<T>* concat(LazySequence<T>* other);

    template <class U>
    LazySequence<U>* map(std::function<U(const T&)> f);

    LazySequence<T>* where(std::function<bool(const T&)> pred);

    template <class U, class R>
    LazySequence<R>* zip(LazySequence<U>* other, std::function<R(const T&, const U&)> combiner);

    // Материализует всё; на бесконечной — не завершается
    T reduce(std::function<T(const T&, const T&)> f, const T& initial);
};

// Полный тип Generator нужен для определений в lazy_sequence.tpp.
#include "lazy/generator.h"
#include "lazy_sequence.tpp"

#endif
