#ifndef GENERATOR_TPP
#define GENERATOR_TPP

#include "lazy/generator.h"

// ============ SourceGenerator

template <class T>
Sequence<T>* SourceGenerator<T>::copy_of(const Sequence<T>* source) {
    MutableArraySequence<T>* destination = new MutableArraySequence<T>();

    IEnumerator<T>* iterator = source->get_enumerator();
    while (iterator->move_next()) {
        destination->append(iterator->get_current());
    }
    delete iterator;

    return destination;
}

template <class T>
SourceGenerator<T>::SourceGenerator(const Sequence<T>* source) : owned(nullptr), iterator(nullptr), pos(0), total(0) {
    if (source == nullptr) throw std::invalid_argument("SourceGenerator: source is nullptr");

    owned = copy_of(source);
    iterator = owned->get_enumerator();
    total = static_cast<size_t>(owned->get_count());
}

template <class T>
SourceGenerator<T>* SourceGenerator<T>::own(Sequence<T>* source) {
    if (source == nullptr) throw std::invalid_argument("SourceGenerator::own: source is nullptr");

    SourceGenerator<T>* generator = new SourceGenerator<T>();
    generator->owned = source;
    generator->iterator = source->get_enumerator();
    generator->pos = 0;
    generator->total = static_cast<size_t>(source->get_count());

    return generator;
}

template <class T>
SourceGenerator<T>::~SourceGenerator() {
    delete iterator;
    delete owned;
}

template <class T>
T SourceGenerator<T>::get_next() {
    if (!has_next()) throw std::out_of_range("SourceGenerator: no more elements");

    iterator->move_next();
    T value = iterator->get_current();
    pos++;

    return value;
}

template <class T>
Option<T> SourceGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
Ordinal SourceGenerator<T>::estimate_remaining() const {
    if (pos >= total) return Ordinal::zero();

    return Ordinal::finite(total - pos);
}

template <class T>
Generator<T>* SourceGenerator<T>::clone() const {
    // Копия с pos = 0 - копирующий конструктор сам стартует с нуля
    return new SourceGenerator<T>(owned);
}

template <class T>
T SourceGenerator<T>::get_at(OrdinalIndex idx) const {
    if (idx.omega_part != 0)
        throw std::out_of_range("SourceGenerator::get_at: source is finite, omega_part must be 0");
    if (idx.finite_part >= total)
        throw std::out_of_range("SourceGenerator::get_at: index past end");
    // Fast path: если owned - ArraySequence (типично), O(1) индекс.
    // Иначе fallback на линейную материализацию (для list-based sequence).
    auto* arr = dynamic_cast<ArraySequence<T>*>(owned);
    if (arr != nullptr) return arr->get(static_cast<int>(idx.finite_part));
    return materialize_at_linear(static_cast<const Generator<T>*>(this), idx.finite_part);
}

// ================= RecurrenceGenerator

template <class T>
RecurrenceGenerator<T>::RecurrenceGenerator(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial) : rule(rule), k(0), pos(0) {
    if (initial == nullptr) throw std::invalid_argument("RecurrenceGenerator: initial is nullptr");
    if (!rule) throw std::invalid_argument("RecurrenceGenerator: rule is empty");

    int initial_count = initial->get_count();
    if (initial_count <= 0) throw std::invalid_argument("RecurrenceGenerator: initial must have at least 1 element");

    k = static_cast<size_t>(initial_count);

    EnumeratorWrapper<T> iterator(initial->get_enumerator());
    while (iterator.move_next()) {
        this->initial.append(iterator.get_current());
    }
}

template <class T>
T RecurrenceGenerator<T>::get_next() {
    if (pos < k) {
        // Начальный этап, где выдаём начальные элементы, попутно заполняя окно
        T value = initial.get(static_cast<int>(pos));
        window.append(value);
        pos++;

        return value;
    }

    // pos >= k и window содержит последние k элементов, приненяем правило
    T next = rule(&window);

    MutableArraySequence<T> new_window;
    for (size_t index = 1; index < k; index++) { // Обновляем окно
        new_window.append(window.get(static_cast<int>(index)));
    }
    new_window.append(next);
    window = new_window;

    pos++;

    return next;
}

template <class T>
Generator<T>* RecurrenceGenerator<T>::clone() const {
    return new RecurrenceGenerator<T>(rule, &initial);
}

// ================= PrependGenerator<T>

template <class T>
PrependGenerator<T>::PrependGenerator(const T& item, Generator<T>* upstream) : head_item(item), upstream(upstream), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("PrependGenerator: upstream is nullptr");
}

template <class T>
PrependGenerator<T>::~PrependGenerator() {
    delete upstream;
}

template <class T>
bool PrependGenerator<T>::has_next() const {
    if (pos == 0) return true;

    return upstream->has_next();
}

template <class T>
T PrependGenerator<T>::get_next() {
    if (pos == 0) {
        pos++;
        return head_item;
    }

    if (!upstream->has_next()) throw std::out_of_range("PrependGenerator: upstream exhausted");

    pos++;

    return upstream->get_next();
}

template <class T>
Option<T> PrependGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
Ordinal PrependGenerator<T>::estimate_remaining() const {
    Ordinal upstream_remaining = upstream->estimate_remaining();
    if (pos == 0) return upstream_remaining + Ordinal::finite(1);

    return upstream_remaining;
}

template <class T>
Generator<T>* PrependGenerator<T>::clone() const {
    return new PrependGenerator<T>(head_item, upstream->clone());
}

template <class T>
T PrependGenerator<T>::get_at(OrdinalIndex idx) const {
    // {0, 0} -> head, остальные сдвигаются.
    // Ординально: 1 + (a*ω + n) = a*ω + n при a > 0, либо 1 + n = 1 + n при a == 0.
    // Так что в финитной части индекс сдвигается на -1, в ω-блоках не меняется.
    if (idx.omega_part == 0) {
        if (idx.finite_part == 0) return head_item;
        OrdinalIndex shifted(0, idx.finite_part - 1);
        auto* up = dynamic_cast<OrdinalIndexable<T>*>(upstream);
        if (up != nullptr) return up->get_at(shifted);
        return materialize_at_linear(upstream, shifted.finite_part);
    }
    // omega_part > 0 - в апстриме на той же позиции
    auto* up = dynamic_cast<OrdinalIndexable<T>*>(upstream);
    if (up != nullptr) return up->get_at(idx);
    throw std::logic_error("PrependGenerator::get_at: upstream is not ordinal-indexable");
}

// ================= InsertAtGenerator<T>

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position, const T& item, Generator<T>* upstream)
    : inject_position(inject_position),
      upstream(upstream),
      injected(nullptr),
      injected_length(Ordinal::finite(1)),
      pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("InsertAtGenerator: upstream is nullptr");

    // Оборачиваем единственный элемент в одноэлементный генератор
    MutableArraySequence<T>* one = new MutableArraySequence<T>();
    one->append(item);
    injected = SourceGenerator<T>::own(one);
}

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position,
                                        Generator<T>* injected, Ordinal injected_length,
                                        Generator<T>* upstream)
    : inject_position(inject_position),
      upstream(upstream),
      injected(injected),
      injected_length(injected_length),
      pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("InsertAtGenerator: upstream is nullptr");
    if (injected == nullptr) throw std::invalid_argument("InsertAtGenerator: injected is nullptr");
}

template <class T>
InsertAtGenerator<T>::~InsertAtGenerator() {
    delete upstream;
    delete injected;
}

template <class T>
bool InsertAtGenerator<T>::has_next() const {
    // До точки вставки - смотрим upstream
    if (pos < inject_position) return upstream->has_next();

    // В точке вставки и далее
    if (injected_length.is_finite()) {
        size_t m = injected_length.get_value();
        if (pos < inject_position + m) {
            // Сейчас в зоне inserted
            return injected->has_next();
        }
        // После вставки - снова upstream
        return upstream->has_next();
    }
    // Бесконечная вставка: после inject_position навсегда injected
    return injected->has_next();
}

template <class T>
T InsertAtGenerator<T>::get_next() {
    if (pos < inject_position) {
        if (!upstream->has_next())
            throw std::out_of_range("InsertAtGenerator: upstream exhausted before inject_position");
        pos++;
        return upstream->get_next();
    }

    if (injected_length.is_finite()) {
        size_t m = injected_length.get_value();
        if (pos < inject_position + m) {
            if (!injected->has_next())
                throw std::out_of_range("InsertAtGenerator: injected exhausted unexpectedly");
            pos++;
            return injected->get_next();
        }
        // Зона после вставки - продолжаем upstream
        if (!upstream->has_next())
            throw std::out_of_range("InsertAtGenerator: upstream exhausted");
        pos++;
        return upstream->get_next();
    }

    // injected бесконечна: после inject_position - всегда injected
    if (!injected->has_next())
        throw std::out_of_range("InsertAtGenerator: injected exhausted unexpectedly");
    pos++;
    return injected->get_next();
}

template <class T>
Option<T> InsertAtGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();
    return Option<T>::Some(get_next());
}

template <class T>
Ordinal InsertAtGenerator<T>::estimate_remaining() const {
    Ordinal upstream_remaining = upstream->estimate_remaining();

    if (injected_length.is_finite()) {
        size_t m = injected_length.get_value();
        if (pos < inject_position) {
            // Впереди: остаток upstream до inject_position + m injected + хвост upstream
            return upstream_remaining + Ordinal::finite(m);
        }
        if (pos < inject_position + m) {
            // Внутри вставки: остаток injected + хвост upstream
            size_t remain_injected = inject_position + m - pos;
            return Ordinal::finite(remain_injected) + upstream_remaining;
        }
        return upstream_remaining;
    }

    // injected бесконечен
    if (pos < inject_position) {
        // upstream до p, потом injected (бесконечная), потом хвост upstream (за омегу)
        return upstream_remaining + Ordinal::infinity();
    }
    // pos >= inject_position - мы уже в injected, остаток = injected.remaining + хвост upstream (за омегу)
    return injected->estimate_remaining() + upstream_remaining;
}

template <class T>
Generator<T>* InsertAtGenerator<T>::clone() const {
    return new InsertAtGenerator<T>(inject_position, injected->clone(), injected_length, upstream->clone());
}

template <class T>
T InsertAtGenerator<T>::get_at(OrdinalIndex idx) const {
    if (injected_length.is_finite()) {
        size_t m = injected_length.get_value();
        if (idx.omega_part == 0) {
            if (idx.finite_part < inject_position) {
                return materialize_at_ord(upstream, OrdinalIndex(0, idx.finite_part));
            }
            if (idx.finite_part < inject_position + m) {
                return materialize_at_ord(injected, OrdinalIndex(0, idx.finite_part - inject_position));
            }
            // элемент upstream после вставки - сдвинут на m
            return materialize_at_ord(upstream, OrdinalIndex(0, idx.finite_part - m));
        }
        // omega_part > 0 при финитной вставке - это просто upstream в той же ω-позиции
        return materialize_at_ord(upstream, idx);
    }

    // injected бесконечен
    if (idx.omega_part == 0) {
        if (idx.finite_part < inject_position) {
            return materialize_at_ord(upstream, OrdinalIndex(0, idx.finite_part));
        }
        // {0, p + k} - это k-й элемент injected
        return materialize_at_ord(injected, OrdinalIndex(0, idx.finite_part - inject_position));
    }
    if (idx.omega_part == 1) {
        // {1, k} - это (inject_position + k)-й элемент upstream (хвост за омегой)
        return materialize_at_ord(upstream, OrdinalIndex(0, inject_position + idx.finite_part));
    }
    // omega_part > 1: остальные ω-блоки находятся в апстриме (если он их имеет)
    return materialize_at_ord(upstream, idx);
}

// ================= MapGenerator

template <class U, class T>
MapGenerator<U, T>::MapGenerator(Generator<U>* upstream, std::function<T(const U&)> func) : upstream(upstream), func(func), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("MapGenerator: upstream is nullptr");
    if (!func) throw std::invalid_argument("MapGenerator: func is empty");
}

template <class U, class T>
MapGenerator<U, T>::~MapGenerator() {
    delete upstream;
}

template <class U, class T>
T MapGenerator<U, T>::get_next() {
    if (!upstream->has_next()) throw std::out_of_range("MapGenerator: upstream exhausted");

    U upstream_value = upstream->get_next();
    pos++;

    return func(upstream_value);
}

template <class U, class T>
Option<T> MapGenerator<U, T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class U, class T>
Generator<T>* MapGenerator<U, T>::clone() const {
    return new MapGenerator<U, T>(upstream->clone(), func);
}

template <class U, class T>
T MapGenerator<U, T>::get_at(OrdinalIndex idx) const {
    auto* up = dynamic_cast<OrdinalIndexable<U>*>(upstream);
    if (up != nullptr) return func(up->get_at(idx));
    // Линейный fallback: работает только для финитной части
    if (idx.omega_part != 0)
        throw std::logic_error("MapGenerator::get_at: upstream is not ordinal-indexable");
    return func(materialize_at_linear(upstream, idx.finite_part));
}

// ================= WhereGenerator

template <class T>
WhereGenerator<T>::WhereGenerator(Generator<T>* upstream, std::function<bool(const T&)> pred) : upstream(upstream), pred(pred), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("WhereGenerator: upstream is nullptr");
    if (!pred) throw std::invalid_argument("WhereGenerator: pred is empty");
}

template <class T>
WhereGenerator<T>::~WhereGenerator() {
    delete upstream;
}

template <class T>
T WhereGenerator<T>::get_next() {
    while (upstream->has_next()) {
        T value = upstream->get_next();
        if (pred(value)) {
            pos++;
            return value;
        }
    }

    throw std::out_of_range("WhereGenerator: upstream exhausted");
}

template <class T>
Option<T> WhereGenerator<T>::try_get_next() {
    while (upstream->has_next()) {
        Option<T> value = upstream->try_get_next();
        if (!value.has_value()) return Option<T>::None();
        if (pred(value.get_value())) {
            pos++;
            return value;
        }
    }

    return Option<T>::None();
}

template <class T>
Generator<T>* WhereGenerator<T>::clone() const {
    return new WhereGenerator<T>(upstream->clone(), pred);
}

// ================= ZipGenerator

template <class U, class V, class T>
ZipGenerator<U, V, T>::ZipGenerator(Generator<U>* first, Generator<V>* second, std::function<T(const U&, const V&)> combiner) : first(first), second(second), combiner(combiner), pos(0) {
    if (first == nullptr || second == nullptr) throw std::invalid_argument("ZipGenerator: nullptr operand");
    if (!combiner) throw std::invalid_argument("ZipGenerator: combiner is empty");
}

template <class U, class V, class T>
ZipGenerator<U, V, T>::~ZipGenerator() {
    delete first;
    delete second;
}

template <class U, class V, class T>
T ZipGenerator<U, V, T>::get_next() {
    if (!has_next()) throw std::out_of_range("ZipGenerator: one of operands exhausted");

    U first_value = first->get_next();
    V second_value = second->get_next();
    pos++;

    return combiner(first_value, second_value);
}

template <class U, class V, class T>
Option<T> ZipGenerator<U, V, T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class U, class V, class T>
Ordinal ZipGenerator<U, V, T>::estimate_remaining() const {
    Ordinal first_remaining = first->estimate_remaining();
    Ordinal second_remaining = second->estimate_remaining();

    return (first_remaining < second_remaining) ? first_remaining : second_remaining;
}

template <class U, class V, class T>
Generator<T>* ZipGenerator<U, V, T>::clone() const {
    return new ZipGenerator<U, V, T>(first->clone(), second->clone(), combiner);
}

template <class U, class V, class T>
T ZipGenerator<U, V, T>::get_at(OrdinalIndex idx) const {
    auto* fo = dynamic_cast<OrdinalIndexable<U>*>(first);
    auto* so = dynamic_cast<OrdinalIndexable<V>*>(second);
    if (fo != nullptr && so != nullptr) {
        return combiner(fo->get_at(idx), so->get_at(idx));
    }
    if (idx.omega_part != 0)
        throw std::logic_error("ZipGenerator::get_at: at least one upstream is not ordinal-indexable");
    U u = (fo != nullptr) ? fo->get_at(idx) : materialize_at_linear(first, idx.finite_part);
    V v = (so != nullptr) ? so->get_at(idx) : materialize_at_linear(second, idx.finite_part);
    return combiner(u, v);
}

// ================= ConcatGenerator

template <class T>
ConcatGenerator<T>::ConcatGenerator(Generator<T>* left, Ordinal left_length, Generator<T>* right)
    : left(left), left_length(left_length), right(right), pos(0) {
    if (left == nullptr || right == nullptr) throw std::invalid_argument("Nullptr operand in concatenation");
}

template <class T>
bool ConcatGenerator<T>::has_next() const {
    if (left->has_next()) {
        if (left_length.is_finite() && pos >= left_length.get_value()) {
            return right->has_next();
        }
        return true;
    }

    return right->has_next();
}

template <class T>
T ConcatGenerator<T>::get_next() {
    bool left_exhausted = !left->has_next();

    if (left_length.is_finite() && pos >= left_length.get_value()) {
        left_exhausted = true;
    }

    if (!left_exhausted) {
        pos++;
        return left->get_next();
    }

    if (!right->has_next()) { throw std::out_of_range("both sides exhausted"); }

    pos++;
    return right->get_next();
}

template <class T>
Option<T> ConcatGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
Ordinal ConcatGenerator<T>::estimate_remaining() const {
    if (left_length.is_infinite()) {
        return Ordinal::infinity() + right->estimate_remaining();
    }
    if (pos < left_length.get_value()) {
        Ordinal left_remaining = Ordinal::finite(left_length.get_value() - pos);
        return left_remaining + right->estimate_remaining();
    }
    return right->estimate_remaining();
}

template <class T>
Generator<T>* ConcatGenerator<T>::clone() const {
    return new ConcatGenerator<T>(left->clone(), left_length, right->clone());
}

template <class T>
T ConcatGenerator<T>::get_at(OrdinalIndex idx) const {
    // Обобщённый алгоритм для трансфинитной конкатенации произвольной глубины:
    //   idx < left_length  -> в левой
    //   idx >= left_length -> right.get_at(idx - left_length)
    // Это позволяет chain'ить concat: concat(concat(inf, inf), inf) даст ω·3,
    // get(OrdinalIndex{2, k}) рекурсивно через dynamic_cast<OrdinalIndexable>.
    if (idx < left_length) {
        return materialize_at_ord(left, idx);
    }
    OrdinalIndex right_idx = idx - left_length;
    return materialize_at_ord(right, right_idx);
}

template <class T>
ConcatGenerator<T>::~ConcatGenerator() {
    delete left;
    delete right;
}

#endif