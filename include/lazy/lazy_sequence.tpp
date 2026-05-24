#ifndef LAZY_SEQUENCE_TPP
#define LAZY_SEQUENCE_TPP

#include "lazy/lazy_sequence.h"
#include "core/ienumerator.h"
#include <stdexcept>
#include <climits>

template <class T>
LazySequence<T>::LazySequence(int cache_capacity)
    : generator(nullptr), gen_pos(0), length(Ordinal::zero()), cache(cache_capacity) {}

template <class T>
LazySequence<T>::LazySequence(const T* items, int count, int cache_capacity)
    : generator(nullptr), gen_pos(0), length(Ordinal::zero()), cache(cache_capacity) {
    if (count < 0) throw std::out_of_range("Count cannot be < 0");

    if (count > 0) {
        MutableArraySequence<T>* source = new MutableArraySequence<T>();
        for (int index = 0; index < count; index++) {
            source->append(items[index]);
        }
        generator = SourceGenerator<T>::own(source);
    }

    length = Ordinal::finite(static_cast<size_t>(count));
}

template <class T>
LazySequence<T>::LazySequence(const Sequence<T>* source, int cache_capacity)
    : generator(nullptr), gen_pos(0), length(Ordinal::zero()), cache(cache_capacity) {
    if (source == nullptr) throw std::invalid_argument("Cannot create from nullptr sequence");

    int source_count = source->get_count();
    if (source_count > 0) {
        generator = new SourceGenerator<T>(source);
    }

    length = Ordinal::finite(static_cast<size_t>(source_count));
}

template <class T>
LazySequence<T>::LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial, int cache_capacity)
    : generator(nullptr), gen_pos(0), length(Ordinal::infinity()), cache(cache_capacity) {
    generator = new RecurrenceGenerator<T>(rule, initial);
}

template <class T>
LazySequence<T>::LazySequence(Generator<T>* generator, Ordinal length, int cache_capacity)
    : generator(generator), gen_pos(generator != nullptr ? generator->position() : 0), length(length), cache(cache_capacity) {}

template <class T>
void LazySequence<T>::sys_append(const T& item) {
    if (length.is_infinite()) throw std::logic_error("sys_append on infinite LazySequence has no meaning");

    MutableArraySequence<T>* buf = new MutableArraySequence<T>();
    buf->append(item);
    Generator<T>* single = SourceGenerator<T>::own(buf);

    if (generator == nullptr) {
        generator = single;
    } else {
        generator = new ConcatGenerator<T>(generator, length, single);
    }

    gen_pos = 0;
    cache.clear();
    length = length + Ordinal::finite(1);
}

template <class T>
Sequence<T>* LazySequence<T>::CreateEmpty() const {
    return new LazySequence<T>(cache.get_capacity());
}

template <class T>
void LazySequence<T>::materialize_up_to(size_t target_index) const {
    if (Ordinal::finite(target_index) >= length) throw std::out_of_range("Index >= length");

    if (cache.contains(target_index)) return;

    if (!cache.is_empty() && target_index < cache.get_first_index()) throw std::out_of_range("Index already evicted from cache");

    if (generator == nullptr) throw std::out_of_range("No generator");

    while (gen_pos <= target_index) {
        if (!generator->has_next()) {
            length = Ordinal::finite(gen_pos);
            throw std::out_of_range("Ran out of elements");
        }

        T value = generator->get_next();
        cache.push(value);
        gen_pos++;
    }
}

template <class T>
const T& LazySequence<T>::get_first() const {
    if (length == Ordinal::zero()) throw std::out_of_range("Lazy sequence is empty");

    materialize_up_to(0);

    return cache.get(0);
}

template <class T>
Option<T> LazySequence<T>::try_get_first() const {
    if (length == Ordinal::zero()) return Option<T>::None();

    try {
        materialize_up_to(0);
        return Option<T>::Some(cache.get(0));
    } catch (...) {
        return Option<T>::None();
    }
}

template <class T>
int LazySequence<T>::get_count() const {
    if (length.is_infinite()) throw std::logic_error("get_count on infinite LazySequence");

    size_t value = length.get_value();
    if (value > static_cast<size_t>(INT_MAX)) throw std::overflow_error("get_count exceeds INT_MAX");

    return static_cast<int>(value);
}

template <class T>
IEnumerator<T>* LazySequence<T>::get_enumerator() const {
    return new Enumerator(const_cast<LazySequence<T>*>(this));
}

template <class T>
const T& LazySequence<T>::get_last() const {
    if (length.is_infinite()) throw std::logic_error("get_last on infinite LazySequence");
    if (length == Ordinal::zero()) throw std::out_of_range("Lazy sequence is empty");

    size_t last_index = length.get_value() - 1;
    materialize_up_to(last_index);

    return cache.get(last_index);
}

template <class T>
Option<T> LazySequence<T>::try_get_last() const {
    if (length.is_infinite()) return Option<T>::None();
    if (length == Ordinal::zero()) return Option<T>::None();

    try {
        size_t last_index = length.get_value() - 1;
        materialize_up_to(last_index);
        return Option<T>::Some(cache.get(last_index));
    } catch (...) {
        return Option<T>::None();
    }
}

// ==========================

template <class T>
Sequence<T>* LazySequence<T>::get_sub_sequence(int, int) const {
    throw std::logic_error("get_sub_sequence(int,int) const not supported on LazySequence, use non-const LazySequence::get_sub_sequence");
}

template <class T>
Sequence<T>* LazySequence<T>::concat(const Sequence<T>*) const {
    throw std::logic_error("concat(const Sequence*) not supported on LazySequence, use concat(LazySequence*)");
}

template <class T>
Sequence<T>* LazySequence<T>::map(T (*)(const T&)) const {
    throw std::logic_error("map(T(*)(const T&)) not supported on LazySequence, use template map<U>(std::function<U(const T&)>)");
}

template <class T>
Sequence<T>* LazySequence<T>::where(bool (*)(const T&)) const {
    throw std::logic_error("where(bool(*)(const T&)) not supported on LazySequence, use where(std::function<bool(const T&)>)");
}

template <class T>
T LazySequence<T>::reduce(T (*)(const T&, const T&), const T&) const {
    throw std::logic_error("reduce(T(*)(const T&, const T&), T) not supported on LazySequence, use reduce(std::function<...>, T)");
}

template <class T>
Sequence<T>* LazySequence<T>::slice(int, int, const Sequence<T>*) const {
    throw std::logic_error("slice() not supported on LazySequence");
}

template <class T>
Ordinal LazySequence<T>::get_length() const {
    return length; 
}

template <class T>
int LazySequence<T>::get_materialized_count() const {
    return cache.get_count();
}

template <class T>
int LazySequence<T>::get_cache_capacity() const {
    return cache.get_capacity();
}

// ==========================

template <class T>
T LazySequence<T>::get(int index) {
    if (index < 0) throw std::out_of_range("Lazy sequence index out of range");
    if (Ordinal::finite(static_cast<size_t>(index)) >= length) throw std::out_of_range("Lazy sequence index out of range");

    size_t logical_index = static_cast<size_t>(index);

    if (cache.is_empty() || logical_index >= cache.get_first_index()) {
        materialize_up_to(logical_index);

        return cache.get(logical_index);
    }

    auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);
    if (indexable != nullptr) {
        return indexable->get_at(Ordinal(0, logical_index));
    }

    throw std::out_of_range("Index already evicted from cache and generator is not ordinal-indexable");
}

template <class T>
T LazySequence<T>::get(Ordinal idx) {
    if (!(idx < length)) throw std::out_of_range("Lazy sequence ordinal index beyond length");

    if (idx.get_omega_count() == 0) {
        return get(static_cast<int>(idx.get_finite_part()));
    }

    auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);
    if (indexable == nullptr) throw std::logic_error("Ordinal index requires an ordinable generator (WhereGenerator is non-indexable)");

    return indexable->get_at(idx);
}

// ==========================

template <class T>
LazySequence<T>* LazySequence<T>::take(int n) {
    if (n < 0) throw std::out_of_range("n must be >= 0");

    Ordinal n_ord = Ordinal::finite(static_cast<size_t>(n));
    if (length < n_ord) throw std::out_of_range("Not enough elements");

    if (!cache.is_empty() && cache.get_first_index() > 0) {
        auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);

        if (indexable == nullptr) throw std::out_of_range("Cache window has shifted past index 0 and generator is not ordinal-indexable");
    }

    MutableArraySequence<T>* buffer = new MutableArraySequence<T>();
    for (int index = 0; index < n; index++) {
        buffer->append(get(index));
    }

    Generator<T>* new_generator = (n > 0) ? SourceGenerator<T>::own(buffer) : nullptr;

    if (n == 0) delete buffer;

    return new LazySequence<T>(new_generator, n_ord, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::take(Ordinal limit) { // Ленивая обрезка по длине
    if (limit.get_omega_count() == 0) {
        return take(static_cast<int>(limit.get_finite_part()));
    }

    if (generator == nullptr) throw std::logic_error("Generator is empty");
    if (!(limit <= length)) throw std::out_of_range("Limit exceeds sequence length");

    return new LazySequence<T>(generator->clone(), limit, cache.get_capacity());
}

// ==========================

template <class T>
LazySequence<T>* LazySequence<T>::get_sub_sequence(int start, int end) {
    if (start < 0 || end < start) throw std::out_of_range("Subsequence indexes out of range");
    if (Ordinal::finite(static_cast<size_t>(end)) >= length) throw std::out_of_range("Subsequence end >= length");

    MutableArraySequence<T>* buffer = new MutableArraySequence<T>();
    for (int index = start; index <= end; index++) {
        buffer->append(get(index));
    }

    Generator<T>* new_generator = SourceGenerator<T>::own(buffer);

    return new LazySequence<T>(new_generator, Ordinal::finite(static_cast<size_t>(end - start + 1)), cache.get_capacity());
}

// ==========================

template <class T>
LazySequence<T>* LazySequence<T>::append(const T& item) {
    MutableArraySequence<T>* buf = new MutableArraySequence<T>();
    buf->append(item);
    Generator<T>* single = SourceGenerator<T>::own(buf);

    Generator<T>* new_gen;
    if (generator == nullptr) {
        new_gen = single;
    } else {
        new_gen = new ConcatGenerator<T>(generator->clone(), length, single);
    }
    Ordinal new_length = length + Ordinal::finite(1);

    return new LazySequence<T>(new_gen, new_length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::prepend(const T& item) {
    if (generator == nullptr) {
        T single[1] = { item };
        return new LazySequence<T>(single, 1, cache.get_capacity());
    }

    Generator<T>* new_gen = new PrependGenerator<T>(item, generator->clone());
    Ordinal new_length = Ordinal::finite(1) + length;

    return new LazySequence<T>(new_gen, new_length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::insert_at(const T& item, int index) {
    if (index < 0) throw std::out_of_range("Index out of range");

    Ordinal idx_ord = Ordinal::finite(static_cast<size_t>(index));

    if (idx_ord > length) throw std::out_of_range("Index out of range");
    if (idx_ord == length) return append(item);

    if (generator == nullptr) throw std::logic_error("Unreachable (empty, non-zero idx)");

    size_t target_index = static_cast<size_t>(index);
    Generator<T>* base_clone = generator->clone();
    Generator<T>* new_gen = new InsertAtGenerator<T>(target_index, item, base_clone);
    Ordinal new_length = Ordinal::finite(1) + length;

    return new LazySequence<T>(new_gen, new_length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::insert_at(LazySequence<T>* other, int index) {
    if (other == nullptr) throw std::invalid_argument("Other is nullptr");
    if (index < 0) throw std::out_of_range("Index < 0");
    if (Ordinal::finite(static_cast<size_t>(index)) > length) throw std::out_of_range("Index past end");

    size_t target_index = static_cast<size_t>(index);

    if (other->generator == nullptr) {
        Generator<T>* this_clone = (generator != nullptr) ? generator->clone() : nullptr;

        return new LazySequence<T>(this_clone, length, cache.get_capacity());
    }

    if (generator == nullptr) {
        if (target_index != 0) throw std::logic_error("Empty base, non-zero index");

        Generator<T>* other_clone = other->generator->clone();
        return new LazySequence<T>(other_clone, other->length, cache.get_capacity());
    }

    // Длина = target_idx + other.length + (this.length - target_idx).
    Generator<T>* this_base_clone = generator->clone();
    Generator<T>* other_clone = other->generator->clone();
    Generator<T>* new_gen = new InsertAtGenerator<T>(target_index, other_clone, other->length, this_base_clone);

    Ordinal target_index_ord = Ordinal::finite(target_index);
    Ordinal remainder = length - target_index_ord;
    Ordinal new_length = target_index_ord + other->length + remainder;

    return new LazySequence<T>(new_gen, new_length, cache.get_capacity());
}

// ==========================

template <class T>
LazySequence<T>* LazySequence<T>::concat(LazySequence<T>* other) {
    if (other == nullptr) throw std::invalid_argument("concat: other is nullptr");

    if (other->generator == nullptr) {
        Generator<T>* this_clone = (generator != nullptr) ? generator->clone() : nullptr;
        return new LazySequence<T>(this_clone, length, cache.get_capacity());
    }

    if (generator == nullptr) {
        Generator<T>* other_clone = other->generator->clone();
        return new LazySequence<T>(other_clone, other->length, cache.get_capacity());
    }

    Generator<T>* new_gen = new ConcatGenerator<T>(generator->clone(), length, other->generator->clone());

    Ordinal new_length = length + other->length;
    return new LazySequence<T>(new_gen, new_length, cache.get_capacity());
}

// ==========================

template <class T>
template <class U>
LazySequence<U>* LazySequence<T>::map(std::function<U(const T&)> func) {
    if (generator == nullptr) {
        return new LazySequence<U>(cache.get_capacity());
    }

    Generator<U>* new_gen = new MapGenerator<T, U>(generator->clone(), func);

    return new LazySequence<U>(new_gen, length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::where(std::function<bool(const T&)> pred) {
    if (generator == nullptr) {
        return new LazySequence<T>(cache.get_capacity());
    }

    Generator<T>* new_gen = new WhereGenerator<T>(generator->clone(), pred);

    return new LazySequence<T>(new_gen, length, cache.get_capacity());
}

template <class T>
template <class U, class R>
LazySequence<R>* LazySequence<T>::zip(LazySequence<U>* other, std::function<R(const T&, const U&)> combiner) {
    if (other == nullptr) throw std::invalid_argument("Other is nullptr");

    if (generator == nullptr || other->generator == nullptr) {
        return new LazySequence<R>(cache.get_capacity());
    }

    Generator<T>* a_gen = generator->clone();
    Generator<U>* b_gen = other->generator->clone();
    Generator<R>* new_gen = new ZipGenerator<T, U, R>(a_gen, b_gen, combiner);

    // Длина zip = min(this.length, other.length).
    Ordinal new_length = (length < other->length) ? length : other->length;

    return new LazySequence<R>(new_gen, new_length, cache.get_capacity());
}

template <class T>
T LazySequence<T>::reduce(std::function<T(const T&, const T&)> f, const T& initial) {
    if (length.is_infinite()) throw std::logic_error("Reduce on infinite LazySequence");

    size_t total = length.get_value();
    T accumulator = initial;
    for (size_t index = 0; index < total; index++) {
        accumulator = f(accumulator, get(static_cast<int>(index)));
    }

    return accumulator;
}

template <class T>
LazySequence<T>::~LazySequence() {
    delete generator;
}

#endif
