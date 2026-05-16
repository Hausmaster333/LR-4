#ifndef LAZY_SEQUENCE_TPP
#define LAZY_SEQUENCE_TPP

#include "lazy/lazy_sequence.h"
#include "core/ienumerator.h"
#include <stdexcept>
#include <climits>

template <class T>
LazySequence<T>::LazySequence(int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Cardinal::zero()),
      length(Cardinal::zero()),
      cache(cache_capacity),
      tail() {}

template <class T>
LazySequence<T>::LazySequence(const T* items, int count, int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Cardinal::zero()),
      length(Cardinal::zero()),
      cache(cache_capacity),
      tail() {
    if (count < 0) throw std::out_of_range("Count cannot be < 0");

    if (count > 0) {
        MutableArraySequence<T>* source = new MutableArraySequence<T>();
        for (int index = 0; index < count; index++) source->append(items[index]);
        // SourceGenerator забирает буфер в собственность через own()
        generator = SourceGenerator<T>::own(source);
    }
    base_length = Cardinal::finite(static_cast<size_t>(count));
    length = base_length;
}

template <class T>
LazySequence<T>::LazySequence(const Sequence<T>* source, int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Cardinal::zero()),
      length(Cardinal::zero()),
      cache(cache_capacity),
      tail() {
    if (source == nullptr) throw std::invalid_argument("Cannot create from nullptr sequence");

    int source_count = source->get_count();
    if (source_count > 0) {
        // Копирующий ctor SourceGenerator сам делает deep-copy source внутрь
        generator = new SourceGenerator<T>(source);
    }
    base_length = Cardinal::finite(static_cast<size_t>(source_count));
    length = base_length;
}

template <class T>
LazySequence<T>::LazySequence(std::function<T(Sequence<T>*)> rule,
                              const Sequence<T>* initial,
                              int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Cardinal::infinity()),
      length(Cardinal::infinity()),
      cache(cache_capacity),
      tail() {
    generator = new RecurrenceGenerator<T>(rule, initial);
}

template <class T>
LazySequence<T>::LazySequence(Generator<T>* generator, Cardinal base_length, int cache_capacity)
    : generator(generator),
      gen_pos(generator != nullptr ? generator->position() : 0),
      base_length(base_length),
      length(base_length),
      cache(cache_capacity),
      tail() {}

template <class T>
LazySequence<T>::~LazySequence() {
    delete generator;
}

template <class T>
void LazySequence<T>::sys_append(const T& item) {
    if (length.is_infinite()) {
        throw std::logic_error("sys_append on infinite LazySequence has no meaning");
    }
    tail.push_append(item);
    length = length + Cardinal::finite(1);
}

template <class T>
Sequence<T>* LazySequence<T>::CreateEmpty() const {
    return new LazySequence<T>(cache.get_capacity());
}

template <class T>
void LazySequence<T>::materialize_up_to(size_t target_index) const {
    if (base_length.is_finite() && target_index >= base_length.get_value()) {
        throw std::out_of_range("materialize_up_to: index >= base_length");
    }

    if (cache.contains(target_index)) return;

    if (!cache.is_empty() && target_index < cache.get_first_index()) {
        throw std::out_of_range("LazySequence: index already evicted from cache");
    }

    if (generator == nullptr) throw std::out_of_range("LazySequence: no generator");

    while (gen_pos <= target_index) {
        if (!generator->has_next()) {
            // Generator кончился раньше, чем мы ожидали - уточняем длины
            base_length = Cardinal::finite(gen_pos);
            length = base_length + tail.get_added_length();
            throw std::out_of_range("LazySequence: ran out of elements");
        }
        T value = generator->get_next();
        cache.push(value);
        gen_pos++;
    }
}

template <class T>
const T& LazySequence<T>::get_first() const {
    if (length.is_finite() && length.get_value() == 0) {
        throw std::out_of_range("Lazy sequence is empty");
    }
    if (base_length.is_finite() && base_length.get_value() == 0) {
        // base пуст - первый элемент в tail. Контракт возвращает const T&, на временную копию ссылку отдать нельзя
        throw std::logic_error("get_first() with empty base + non-empty tail is not supported as const T& (use get(0))");
    }

    materialize_up_to(0);
    return cache.get(0);
}

template <class T>
Option<T> LazySequence<T>::try_get_first() const {
    if (length.is_finite() && length.get_value() == 0) return Option<T>::None();

    try {
        if (base_length.is_finite() && base_length.get_value() == 0) {
            // base пуст - пробуем tail напрямую (по значению)
            return Option<T>::Some(tail.get(0));
        }
        materialize_up_to(0);
        return Option<T>::Some(cache.get(0));
    } catch (...) {
        return Option<T>::None();
    }
}

template <class T>
int LazySequence<T>::get_count() const {
    if (length.is_infinite()) throw std::logic_error("get_count() on infinite LazySequence");

    size_t value = length.get_value();
    if (value > static_cast<size_t>(INT_MAX)) {
        throw std::overflow_error("get_count() exceeds INT_MAX");
    }

    return static_cast<int>(value);
}

template <class T>
IEnumerator<T>* LazySequence<T>::get_enumerator() const {
    return new Enumerator(const_cast<LazySequence<T>*>(this));
}

template <class T>
const T& LazySequence<T>::get_last() const {
    if (length.is_infinite()) throw std::logic_error("get_last() on infinite LazySequence");
    if (length.get_value() == 0) throw std::out_of_range("Lazy sequence is empty");

    size_t last_index = length.get_value() - 1;
    if (base_length.is_finite() && last_index >= base_length.get_value()) {
        // Последний элемент - в tail, ссылку на временное значение нельзя
        throw std::logic_error("get_last() returns const T&, tail element cannot be returned by reference (use get(length-1))");
    }

    materialize_up_to(last_index);
    return cache.get(last_index);
}

template <class T>
Option<T> LazySequence<T>::try_get_last() const {
    if (length.is_infinite()) return Option<T>::None();
    if (length.get_value() == 0) return Option<T>::None();

    try {
        size_t last_index = length.get_value() - 1;
        if (base_length.is_finite() && last_index >= base_length.get_value()) {
            return Option<T>::Some(tail.get(last_index - base_length.get_value()));
        }
        materialize_up_to(last_index);
        return Option<T>::Some(cache.get(last_index));
    } catch (...) {
        return Option<T>::None();
    }
}

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
Cardinal LazySequence<T>::get_length() const {
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

template <class T>
T LazySequence<T>::get(int index) {
    if (index < 0) throw std::out_of_range("Lazy sequence index out of range");
    if (length.is_finite() && static_cast<size_t>(index) >= length.get_value()) {
        throw std::out_of_range("Lazy sequence index out of range");
    }

    size_t logical_index = static_cast<size_t>(index);

    // Tail-зона: index >= base_length (только если base_length финитная)
    if (base_length.is_finite() && logical_index >= base_length.get_value()) {
        return tail.get(static_cast<int>(logical_index - base_length.get_value()));
    }

    // Base-зона - материализуем через generator + кэш
    materialize_up_to(logical_index);
    return cache.get(logical_index);
}

template <class T>
LazySequence<T>* LazySequence<T>::take(int n) {
    if (n < 0) throw std::out_of_range("take: n must be >= 0");

    Cardinal n_cardinal = Cardinal::finite(static_cast<size_t>(n));
    // Требуем, чтобы base покрывал N - тогда мы можем взять первые N элементов из base, а потом dock tail. На бесконечной всегда хватает
    if (base_length < n_cardinal) throw std::out_of_range("take: not enough base elements");

    // Sliding-кэш мог уже сдвинуться вперёд - тогда индекс 0 вытеснен и take не может собрать первые N элементов из base
    if (!cache.is_empty() && cache.get_first_index() > 0) {
        throw std::out_of_range("take: cache window has shifted past index 0, early elements are evicted and cannot be re-materialized - reset first");
    }

    MutableArraySequence<T>* buffer = new MutableArraySequence<T>();
    for (int index = 0; index < n; index++) {
        materialize_up_to(static_cast<size_t>(index));
        buffer->append(cache.get(static_cast<size_t>(index)));
    }
    // Бесконечная становится конечной - отложенный хвост применяется в конец
    tail.apply_to(*buffer);

    Cardinal new_length = n_cardinal + tail.get_added_length();
    Generator<T>* new_generator = SourceGenerator<T>::own(buffer);
    return new LazySequence<T>(new_generator, new_length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::get_sub_sequence(int start, int end) {
    if (start < 0 || end < start) throw std::out_of_range("Subsequence indexes out of range");
    if (length.is_finite() && static_cast<size_t>(end) >= length.get_value()) {
        throw std::out_of_range("Subsequence end >= length");
    }

    MutableArraySequence<T>* buffer = new MutableArraySequence<T>();
    for (int index = start; index <= end; index++) buffer->append(get(index));

    Generator<T>* new_generator = SourceGenerator<T>::own(buffer);
    return new LazySequence<T>(new_generator,
                               Cardinal::finite(static_cast<size_t>(end - start + 1)),
                               cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::append(const T& item) {
    Generator<T>* new_generator = (generator != nullptr) ? generator->clone() : nullptr;
    LazySequence<T>* result = new LazySequence<T>(new_generator, base_length, cache.get_capacity());
    result->tail = tail;
    result->tail.push_append(item);
    result->length = length + Cardinal::finite(1);

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::prepend(const T& item) {
    if (generator == nullptr) {
        // Пустая base - создаём seq из одного элемента + старый tail
        T single_item[1] = { item };
        LazySequence<T>* result = new LazySequence<T>(single_item, 1, cache.get_capacity());
        result->tail = tail;
        result->length = Cardinal::finite(1) + tail.get_added_length();

        return result;
    }

    Generator<T>* base_generator_clone = generator->clone();
    Generator<T>* new_generator = new PrependGenerator<T>(item, base_generator_clone);
    Cardinal new_base_length = base_length + Cardinal::finite(1);

    LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());
    result->tail = tail;
    result->length = length + Cardinal::finite(1);

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::insert_at(const T& item, int index) {
    if (index < 0) throw std::out_of_range("Index out of range");
    if (length.is_finite() && static_cast<size_t>(index) > length.get_value()) {
        throw std::out_of_range("Index out of range");
    }
    // index == length на финитной = это append
    if (length.is_finite() && static_cast<size_t>(index) == length.get_value()) {
        return append(item);
    }

    size_t target_index = static_cast<size_t>(index);

    // index в base-зоне (или base бесконечна) - оборачиваем generator
    if (!base_length.is_finite() || target_index < base_length.get_value()) {
        if (generator == nullptr) {
            // base пуст и idx < 0 невозможно, idx == 0 уже обработано как append
            throw std::logic_error("insert_at: unreachable (empty base, non-zero idx)");
        }
        Generator<T>* base_generator_clone = generator->clone();
        Generator<T>* new_generator = new InsertAtGenerator<T>(target_index, item, base_generator_clone);
        Cardinal new_base_length = base_length + Cardinal::finite(1);

        LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());
        result->tail = tail;
        result->length = length + Cardinal::finite(1);

        return result;
    }

    // target_index в tail-зоне - пока не поддержано (требует операций над tail)
    throw std::logic_error("insert_at into tail not implemented");
}

template <class T>
LazySequence<T>* LazySequence<T>::concat(LazySequence<T>* other) {
    if (other == nullptr) throw std::invalid_argument("concat: other is nullptr");

    if (length.is_infinite() && other->length.is_infinite()) {
        throw std::logic_error("concat of two infinite sequences is not supported");
    }

    if (other->length.is_infinite()) {
        // this finite, other infinite - собираем ConcatGenerator
        // Сначала материализуем this целиком (включая tail) в this_buffer
        size_t this_length = length.get_value();
        MutableArraySequence<T>* this_buffer = new MutableArraySequence<T>();
        for (size_t index = 0; index < this_length; index++) {
            this_buffer->append(get(static_cast<int>(index)));
        }

        Generator<T>* this_generator = SourceGenerator<T>::own(this_buffer);
        Generator<T>* other_generator = other->generator->clone();
        Generator<T>* new_generator = new ConcatGenerator<T>(this_generator, length, other_generator);

        return new LazySequence<T>(new_generator, Cardinal::infinity(), cache.get_capacity());
    }

    // other finite - складываем в tail
    size_t other_length = other->length.get_value();
    MutableArraySequence<T> other_buffer;
    for (size_t index = 0; index < other_length; index++) {
        other_buffer.append(other->get(static_cast<int>(index)));
    }

    Generator<T>* new_generator = (generator != nullptr) ? generator->clone() : nullptr;
    LazySequence<T>* result = new LazySequence<T>(new_generator, base_length, cache.get_capacity());
    result->tail = tail; // Копируем старый tail this
    result->tail.push_concat(&other_buffer); // Добавляем other в конец tail
    result->length = length + Cardinal::finite(other_length);

    return result;
}

template <class T>
template <class U>
LazySequence<U>* LazySequence<T>::map(std::function<U(const T&)> func) {
    Generator<U>* new_generator = nullptr;
    if (generator != nullptr) {
        new_generator = new MapGenerator<T, U>(generator->clone(), func);
    }
    LazySequence<U>* result = new LazySequence<U>(new_generator, base_length, cache.get_capacity());

    // Преобразуем tail через f
    int tail_count = tail.get_op_count() > 0 ? static_cast<int>(tail.get_added_length().get_value()) : 0;
    for (int index = 0; index < tail_count; index++) {
        T original = tail.get(index);
        result->tail.push_append(func(original));
    }
    result->length = length;

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::where(std::function<bool(const T&)> pred) {
    Generator<T>* new_generator = nullptr;
    if (generator != nullptr) {
        new_generator = new WhereGenerator<T>(generator->clone(), pred);
    }
    // Длина where неизвестна - оставляем infinity (точное число знаем только после полной материализации, что невозможно для бесконечной)
    Cardinal new_base_length = Cardinal::infinity();
    LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());

    // Фильтруем tail через pred
    int tail_count = tail.get_op_count() > 0 ? static_cast<int>(tail.get_added_length().get_value()) : 0;
    for (int index = 0; index < tail_count; index++) {
        T item = tail.get(index);
        if (pred(item)) result->tail.push_append(item);
    }
    result->length = Cardinal::infinity();

    return result;
}

template <class T>
template <class U, class R>
LazySequence<R>* LazySequence<T>::zip(LazySequence<U>* other,
                                      std::function<R(const T&, const U&)> combiner) {
    if (other == nullptr) throw std::invalid_argument("zip: other is nullptr");

    if (generator == nullptr || other->generator == nullptr) {
        return new LazySequence<R>(cache.get_capacity());
    }

    Generator<T>* a_generator = generator->clone();
    Generator<U>* b_generator = other->generator->clone();
    Generator<R>* new_generator = new ZipGenerator<T, U, R>(a_generator, b_generator, combiner);

    Cardinal new_length = Cardinal::zero();
    if (base_length.is_infinite() && other->base_length.is_infinite()) {
        new_length = Cardinal::infinity();
    } else if (base_length.is_infinite()) {
        new_length = other->base_length;
    } else if (other->base_length.is_infinite()) {
        new_length = base_length;
    } else {
        new_length = (base_length < other->base_length) ? base_length : other->base_length;
    }

    // tail у zip не применяем - семантика "по парам", append к одной из последовательностей сюда не вписан
    return new LazySequence<R>(new_generator, new_length, cache.get_capacity());
}

template <class T>
T LazySequence<T>::reduce(std::function<T(const T&, const T&)> f, const T& initial) {
    if (length.is_infinite()) throw std::logic_error("reduce on infinite LazySequence");

    size_t total = length.get_value();
    T accumulator = initial;
    for (size_t index = 0; index < total; index++) {
        accumulator = f(accumulator, get(static_cast<int>(index)));
    }

    return accumulator;
}

#endif
