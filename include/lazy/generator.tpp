#ifndef GENERATOR_TPP
#define GENERATOR_TPP

// Подключаем парный .h и lazy_sequence.h для IDE/IntelliSense — иначе при
// открытии .tpp напрямую не видны шаблоны Generator и LazySequence.
// Header guards защищают от бесконечного включения.
#include "lazy/generator.h"
#include "lazy/lazy_sequence.h"

#include <stdexcept>
#include "core/ienumerator.h"

// =====================================================================
// Базовый Generator<T>
// =====================================================================

template <class T>
Generator<T>::Generator(LazySequence<T>* owner) : owner(owner), position(0) {}

// --- Прокси к приватному API хозяина ---
// (См. комментарий в generator.h: технически необходимы, потому что
// friendship не наследуется, и подклассы не могут лезть напрямую.)

template <class T>
void Generator<T>::cache_push(const T& item) {
    owner->cache_push(item);
}

template <class T>
const T& Generator<T>::cache_at(size_t i) const {
    return owner->cache_at(i);
}

template <class T>
size_t Generator<T>::cache_size() const {
    return owner->cache_size();
}

template <class T>
void Generator<T>::set_length(Cardinal c) {
    owner->set_length(c);
}

template <class T>
Cardinal Generator<T>::length_hint() const {
    return owner->length_hint();
}

// --- Дефолтные реализации операций (заглушки на блок 1) ---
// В блоке 3 будут переписаны на ModifyingGenerator.

template <class T>
Generator<T>* Generator<T>::append(const T&) const {
    throw std::runtime_error("Generator::append(item): not implemented yet (block 3)");
}

template <class T>
Generator<T>* Generator<T>::append(const Sequence<T>*) const {
    throw std::runtime_error("Generator::append(seq): not implemented yet (block 3)");
}

template <class T>
Generator<T>* Generator<T>::insert(const T&, size_t) const {
    throw std::runtime_error("Generator::insert(item, idx): not implemented yet (block 3)");
}

template <class T>
Generator<T>* Generator<T>::insert(const Sequence<T>*, size_t) const {
    throw std::runtime_error("Generator::insert(seq, idx): not implemented yet (block 3)");
}

template <class T>
Generator<T>* Generator<T>::remove(size_t) const {
    throw std::runtime_error("Generator::remove(idx): not implemented yet (block 3)");
}

template <class T>
Generator<T>* Generator<T>::remove(size_t, size_t) const {
    throw std::runtime_error("Generator::remove(idx, count): not implemented yet (block 3)");
}

// =====================================================================
// SourceGenerator<T> — обёртка над готовой Sequence<T>*.
// Sequence не имеет get(int), поэтому используем enumerator,
// сохраняя его как поле и продвигая при каждом get_next().
// =====================================================================

// Чтобы не таскать iter в публичном объявлении, используем простой паттерн:
// храним IEnumerator<T>* как mutable-поле через приведение/обёртку.
// Объявление SourceGenerator уже зафиксировано в generator.h без поля iter,
// поэтому держим его в отдельной map (вариант) — слишком громоздко.
// Проще: в этом блоке делаем SourceGenerator стабом, а полную реализацию
// — в блоке 2, когда сразу же добавим поле iter в .h.

template <class T>
SourceGenerator<T>::SourceGenerator(LazySequence<T>* owner, const Sequence<T>* source)
    : Generator<T>(owner), source(source) {}

template <class T>
bool SourceGenerator<T>::has_next() const {
    return this->position < static_cast<size_t>(source->get_count());
}

template <class T>
T SourceGenerator<T>::get_next() {
    if (!has_next()) {
        throw std::out_of_range("SourceGenerator: no more elements");
    }

    // На блок 1: пересоздаём enumerator каждый раз и идём до position.
    // O(n) на каждый вызов — заведомо плохо, но это временно.
    // В блоке 2 заменим на хранимый IEnumerator<T>* в поле класса.
    EnumeratorWrapper<T> iter(source->get_enumerator());
    for (size_t i = 0; i <= this->position; ++i) {
        iter.move_next();
    }
    T value = iter.get_current();
    this->cache_push(value);
    this->position++;
    return value;
}

template <class T>
Option<T> SourceGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();
    return Option<T>::Some(get_next());
}

#endif
