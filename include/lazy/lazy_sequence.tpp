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
      base_length(Ordinal::zero()),
      length(Ordinal::zero()),
      cache(cache_capacity),
      tail() {}

template <class T>
LazySequence<T>::LazySequence(const T* items, int count, int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Ordinal::zero()),
      length(Ordinal::zero()),
      cache(cache_capacity),
      tail() {
    if (count < 0) throw std::out_of_range("Count cannot be < 0");

    if (count > 0) {
        MutableArraySequence<T>* source = new MutableArraySequence<T>();
        for (int index = 0; index < count; index++) source->append(items[index]);
        // SourceGenerator забирает буфер в собственность через own()
        generator = SourceGenerator<T>::own(source);
    }
    base_length = Ordinal::finite(static_cast<size_t>(count));
    length = base_length;
}

template <class T>
LazySequence<T>::LazySequence(const Sequence<T>* source, int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Ordinal::zero()),
      length(Ordinal::zero()),
      cache(cache_capacity),
      tail() {
    if (source == nullptr) throw std::invalid_argument("Cannot create from nullptr sequence");

    int source_count = source->get_count();
    if (source_count > 0) {
        // Копирующий ctor SourceGenerator сам делает deep-copy source внутрь
        generator = new SourceGenerator<T>(source);
    }
    base_length = Ordinal::finite(static_cast<size_t>(source_count));
    length = base_length;
}

template <class T>
LazySequence<T>::LazySequence(std::function<T(Sequence<T>*)> rule,
                              const Sequence<T>* initial,
                              int cache_capacity)
    : generator(nullptr),
      gen_pos(0),
      base_length(Ordinal::infinity()),
      length(Ordinal::infinity()),
      cache(cache_capacity),
      tail() {
    generator = new RecurrenceGenerator<T>(rule, initial);
}

template <class T>
LazySequence<T>::LazySequence(Generator<T>* generator, Ordinal base_length, int cache_capacity)
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
Pair<Generator<T>*, Ordinal> LazySequence<T>::build_full_generator(LazySequence<T>* src) {
    using FullPair = Pair<Generator<T>*, Ordinal>;
    bool tail_empty = src->tail.is_empty();

    if (src->generator == nullptr) {
        // Пустая base: full = source из tail (если непуст), либо ничего.
        if (tail_empty) return FullPair(nullptr, Ordinal::zero());
        MutableArraySequence<T>* buf = new MutableArraySequence<T>();
        int n = static_cast<int>(src->tail.get_added_length().get_value());
        for (int i = 0; i < n; i++) buf->append(src->tail.get(i));
        return FullPair(SourceGenerator<T>::own(buf),
                        Ordinal::finite(static_cast<size_t>(n)));
    }
    if (tail_empty) {
        return FullPair(src->generator->clone(), src->base_length);
    }
    // base + tail через ConcatGenerator(base_clone, base_length, SourceGen из tail).
    MutableArraySequence<T>* buf = new MutableArraySequence<T>();
    int n = static_cast<int>(src->tail.get_added_length().get_value());
    for (int i = 0; i < n; i++) buf->append(src->tail.get(i));
    Generator<T>* tail_gen = SourceGenerator<T>::own(buf);
    Generator<T>* base_clone = src->generator->clone();
    Generator<T>* full = new ConcatGenerator<T>(base_clone, src->base_length, tail_gen);
    Ordinal full_len = src->base_length + Ordinal::finite(static_cast<size_t>(n));
    return FullPair(full, full_len);
}

template <class T>
void LazySequence<T>::sys_append(const T& item) {
    if (length.is_infinite()) {
        throw std::logic_error("sys_append on infinite LazySequence has no meaning");
    }
    tail.push_append(item);
    length = length + Ordinal::finite(1);
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
            base_length = Ordinal::finite(gen_pos);
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

    // Base-зона - если кэш ещё содержит индекс (или может его материализовать), используем его.
    // Иначе пробуем ординальный доступ через OrdinalIndexable (для concat/insert и любого
    // генератора, форвардящего get_at). Это нужно, чтобы после линейного прохода вперёд
    // можно было читать любую позицию без перестройки кэша.
    if (cache.is_empty() || logical_index >= cache.get_first_index()) {
        materialize_up_to(logical_index);
        return cache.get(logical_index);
    }
    auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);
    if (indexable != nullptr) {
        return indexable->get_at(OrdinalIndex(0, logical_index));
    }
    throw std::out_of_range("LazySequence: index already evicted from cache and generator is not ordinal-indexable");
}

template <class T>
T LazySequence<T>::get(OrdinalIndex idx) {
    // Тривиальный случай: натуральный индекс
    if (idx.omega_part == 0) {
        if (length.is_finite() && idx.finite_part >= length.get_value()) {
            throw std::out_of_range("Lazy sequence ordinal index out of range");
        }
        return get(static_cast<int>(idx.finite_part));
    }

    // omega_part >= 1: проверяем что индекс в допустимом диапазоне длины.
    // Длина имеет вид ω·omega_count + finite_part. Допустимы все idx < length.
    // Верхней границы на omega_count больше нет — поддерживается ω·k для любого k
    // (chained concat'ы могут давать ω·3, ω·4 и так далее).
    if (!(idx < length)) {
        throw std::out_of_range("Lazy sequence ordinal index beyond length");
    }

    // Ординальный доступ работает для любого корневого генератора, реализующего OrdinalIndexable:
    // ConcatGenerator, InsertAtGenerator, MapGenerator, ZipGenerator, SourceGenerator, PrependGenerator.
    // WhereGenerator НЕ поддерживается (фильтрация делает индексы недетерминированными).
    auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);
    if (indexable == nullptr) {
        throw std::logic_error(
            "Lazy sequence: ordinal index requires an ordinal-aware root generator "
            "(WhereGenerator is the only non-indexable; use concat/insert/map/zip)"
        );
    }
    return indexable->get_at(idx);
}

template <class T>
LazySequence<T>* LazySequence<T>::take(int n) {
    if (n < 0) throw std::out_of_range("take: n must be >= 0");

    Ordinal n_cardinal = Ordinal::finite(static_cast<size_t>(n));
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

    Ordinal new_length = n_cardinal + tail.get_added_length();
    Generator<T>* new_generator = SourceGenerator<T>::own(buffer);
    return new LazySequence<T>(new_generator, new_length, cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::take(OrdinalIndex limit) {
    if (limit.omega_part == 0) {
        return take(static_cast<int>(limit.finite_part));
    }
    // Трансфинитная обрезка: оборачиваем в TakeOrdinalGenerator, длина = limit как Ordinal.
    if (generator == nullptr) throw std::logic_error("take(OrdinalIndex): generator is empty");
    auto* indexable = dynamic_cast<OrdinalIndexable<T>*>(generator);
    if (indexable == nullptr) {
        throw std::logic_error("take(OrdinalIndex): generator is not ordinal-indexable");
    }
    Ordinal limit_ord = Ordinal::omega_times(limit.omega_part) + Ordinal::finite(limit.finite_part);
    if (!(limit_ord <= length)) {
        throw std::out_of_range("take(OrdinalIndex): limit exceeds sequence length");
    }
    Generator<T>* wrapped = new TakeOrdinalGenerator<T>(generator->clone(), limit);
    return new LazySequence<T>(wrapped, limit_ord, cache.get_capacity());
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
                               Ordinal::finite(static_cast<size_t>(end - start + 1)),
                               cache.get_capacity());
}

template <class T>
LazySequence<T>* LazySequence<T>::append(const T& item) {
    Generator<T>* new_generator = (generator != nullptr) ? generator->clone() : nullptr;
    LazySequence<T>* result = new LazySequence<T>(new_generator, base_length, cache.get_capacity());
    result->tail = tail;
    result->tail.push_append(item);
    result->length = length + Ordinal::finite(1);

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::prepend(const T& item) {
    if (generator == nullptr) {
        // Пустая base - создаём seq из одного элемента + старый tail
        T single_item[1] = { item };
        LazySequence<T>* result = new LazySequence<T>(single_item, 1, cache.get_capacity());
        result->tail = tail;
        result->length = Ordinal::finite(1) + tail.get_added_length();

        return result;
    }

    Generator<T>* base_generator_clone = generator->clone();
    Generator<T>* new_generator = new PrependGenerator<T>(item, base_generator_clone);
    // Элемент идёт ПЕРЕД базой: 1 + base_length с левой абсорбцией финита.
    // Для finite N: 1 + N. Для infinite: 1 + ω = ω (omega_count и finite_part не растут).
    Ordinal new_base_length = Ordinal::finite(1) + base_length;

    LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());
    result->tail = tail;
    result->length = Ordinal::finite(1) + length;

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
        // Вставка ВНУТРИ базы: ординально target_index + 1 + (base - target_index) = ...
        // Для финитной базы это base + 1. Для бесконечной — абсорбируется: 1 + ω = ω.
        Ordinal new_base_length = Ordinal::finite(1) + base_length;

        LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());
        result->tail = tail;
        result->length = Ordinal::finite(1) + length;

        return result;
    }

    // target_index в tail-зоне - пока не поддержано (требует операций над tail)
    throw std::logic_error("insert_at into tail not implemented");
}

template <class T>
LazySequence<T>* LazySequence<T>::insert_at(LazySequence<T>* other, int index) {
    if (other == nullptr) throw std::invalid_argument("insert_at: other is nullptr");
    if (index < 0) throw std::out_of_range("insert_at: index < 0");
    if (length.is_finite() && static_cast<size_t>(index) > length.get_value()) {
        throw std::out_of_range("insert_at: index past end");
    }

    size_t target_index = static_cast<size_t>(index);

    // Вставка в tail-зону финитной this пока не поддержана (как и в одноэлементной версии).
    if (base_length.is_finite() && target_index > base_length.get_value()) {
        throw std::logic_error("insert_at(seq): tail-zone insertion not implemented");
    }

    // Если other бесконечен И у this непустой tail - корректно представить нельзя:
    // тailу this пришлось бы лежать после omega-блока other, а наша структура (base + DeferredTail)
    // финитный tail только в конце выражает. Просим пользователя сначала финитизировать через take().
    if (other->length.is_infinite() && !tail.is_empty()) {
        throw std::logic_error(
            "insert_at(seq): cannot insert infinite sequence into this with non-empty tail; "
            "call take(N) first to finitize"
        );
    }

    // Собираем "полный" generator для other (base + материализованный tail).
    // Это устраняет рассинхрон между injected generator (только base) и injected_length (= base + tail).
    using FullPair = Pair<Generator<T>*, Ordinal>;
    FullPair other_full = build_full_generator(other);
    Generator<T>* inj_full = other_full.first();
    Ordinal inj_full_len = other_full.second();

    // Пустой other - результат это копия this.
    if (inj_full == nullptr) {
        Generator<T>* this_clone = (generator != nullptr) ? generator->clone() : nullptr;
        LazySequence<T>* result = new LazySequence<T>(this_clone, base_length, cache.get_capacity());
        result->tail = tail;
        result->length = length;
        return result;
    }

    // Пустой this - результат = other_full с this.tail в конце.
    if (generator == nullptr) {
        if (target_index != 0) {
            delete inj_full;
            throw std::logic_error("insert_at(seq): empty base, non-zero index");
        }
        LazySequence<T>* result = new LazySequence<T>(inj_full, inj_full_len, cache.get_capacity());
        result->tail = tail;
        result->length = inj_full_len + tail.get_added_length();
        return result;
    }

    // Общий случай: this.base[0..p) + inj_full + this.base[p..end_base) + this.tail.
    // Линейный режим: InsertAtGenerator обходит base/injected/base линейно.
    // Ординальный режим: get_at форвардит через OrdinalIndexable.
    Generator<T>* this_base_clone = generator->clone();
    Generator<T>* new_generator = new InsertAtGenerator<T>(
        target_index, inj_full, inj_full_len, this_base_clone
    );

    // Длина базы результата по ординальной арифметике:
    //   new_base = p + inj_full_len + (base_length - p)
    // Эта формула универсальна и даёт правильный ответ для всех 4 комбинаций
    // финитности this/other (см. docs/architecture.md, раздел insert_at).
    Ordinal p_ord = Ordinal::finite(target_index);
    Ordinal remainder = base_length - p_ord;
    Ordinal new_base_length = p_ord + inj_full_len + remainder;

    LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());
    result->tail = tail;
    result->length = new_base_length + tail.get_added_length();

    return result;
}

template <class T>
LazySequence<T>* LazySequence<T>::concat(LazySequence<T>* other) {
    if (other == nullptr) throw std::invalid_argument("concat: other is nullptr");

    // Финитный this + финитный other: складываем other в tail (быстрый путь без копирования this).
    if (length.is_finite() && other->length.is_finite()) {
        size_t other_length = other->length.get_value();
        MutableArraySequence<T> other_buffer;
        for (size_t index = 0; index < other_length; index++) {
            other_buffer.append(other->get(static_cast<int>(index)));
        }

        Generator<T>* new_generator = (generator != nullptr) ? generator->clone() : nullptr;
        LazySequence<T>* result = new LazySequence<T>(new_generator, base_length, cache.get_capacity());
        result->tail = tail;
        result->tail.push_concat(&other_buffer);
        result->length = length + Ordinal::finite(other_length);

        return result;
    }

    // Хотя бы один операнд бесконечен - строим chained ConcatGenerator
    // с tail-прослойками между base и следующим оператором.
    // Это даёт ω·k для любого k через цепочки concat: concat(concat(inf,inf),inf) = ω·3.
    using FullPair = Pair<Generator<T>*, Ordinal>;
    FullPair this_full_pair = build_full_generator(this);
    FullPair other_full_pair = build_full_generator(other);

    if (this_full_pair.first() == nullptr) {
        // this пуст - результат это просто other_full
        if (other_full_pair.first() == nullptr) return new LazySequence<T>(cache.get_capacity());
        return new LazySequence<T>(other_full_pair.first(), other_full_pair.second(), cache.get_capacity());
    }
    if (other_full_pair.first() == nullptr) {
        return new LazySequence<T>(this_full_pair.first(), this_full_pair.second(), cache.get_capacity());
    }

    Generator<T>* new_generator = new ConcatGenerator<T>(
        this_full_pair.first(), this_full_pair.second(), other_full_pair.first()
    );
    Ordinal total = this_full_pair.second() + other_full_pair.second();
    return new LazySequence<T>(new_generator, total, cache.get_capacity());
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
    // Длина where точно неизвестна без полной материализации.
    // Используем верхнюю границу = base_length (для любого case: финитного, ω, ω·k).
    // Реальный count может быть меньше.
    Ordinal new_base_length = base_length;
    LazySequence<T>* result = new LazySequence<T>(new_generator, new_base_length, cache.get_capacity());

    // Фильтруем tail через pred. Tail материализуется eagerly - фильтр применяется здесь.
    int tail_count = tail.get_op_count() > 0 ? static_cast<int>(tail.get_added_length().get_value()) : 0;
    for (int index = 0; index < tail_count; index++) {
        T item = tail.get(index);
        if (pred(item)) result->tail.push_append(item);
    }
    // length = base upper-bound + actual filtered tail count.
    // Для финитной this: upper bound по base + точный счёт по tail.
    // Для бесконечной: ω + k (где k - отфильтрованный tail), консистентно с append на бесконечной.
    result->length = new_base_length + result->tail.get_added_length();

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

    Ordinal new_length = Ordinal::zero();
    if (base_length.is_infinite() && other->base_length.is_infinite()) {
        new_length = Ordinal::infinity();
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