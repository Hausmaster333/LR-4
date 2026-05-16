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
Cardinal SourceGenerator<T>::estimate_remaining() const {
    if (pos >= total) return Cardinal::zero();

    return Cardinal::finite(total - pos);
}

template <class T>
Generator<T>* SourceGenerator<T>::clone() const {
    // Копия с pos = 0 - копирующий конструктор сам стартует с нуля
    return new SourceGenerator<T>(owned);
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
Cardinal PrependGenerator<T>::estimate_remaining() const {
    Cardinal upstream_remaining = upstream->estimate_remaining();
    if (pos == 0) return upstream_remaining + Cardinal::finite(1);

    return upstream_remaining;
}

template <class T>
Generator<T>* PrependGenerator<T>::clone() const {
    return new PrependGenerator<T>(head_item, upstream->clone());
}

// ================= InsertAtGenerator<T>

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position, const T& item, Generator<T>* upstream)
    : inject_position(inject_position),
      upstream(upstream),
      injected(nullptr),
      injected_length(Cardinal::finite(1)),
      pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("InsertAtGenerator: upstream is nullptr");

    // Оборачиваем единственный элемент в одноэлементный генератор
    MutableArraySequence<T>* one = new MutableArraySequence<T>();
    one->append(item);
    injected = SourceGenerator<T>::own(one);
}

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position,
                                        Generator<T>* injected, Cardinal injected_length,
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
Cardinal InsertAtGenerator<T>::estimate_remaining() const {
    Cardinal upstream_remaining = upstream->estimate_remaining();

    if (injected_length.is_finite()) {
        size_t m = injected_length.get_value();
        if (pos < inject_position) {
            // Впереди: остаток upstream до inject_position + m injected + хвост upstream
            return upstream_remaining + Cardinal::finite(m);
        }
        if (pos < inject_position + m) {
            // Внутри вставки: остаток injected + хвост upstream
            size_t remain_injected = inject_position + m - pos;
            return Cardinal::finite(remain_injected) + upstream_remaining;
        }
        return upstream_remaining;
    }

    // injected бесконечен
    if (pos < inject_position) {
        // upstream до p, потом injected (бесконечная), потом хвост upstream (за омегу)
        return upstream_remaining + Cardinal::infinity();
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
                return materialize_at(upstream, idx.finite_part);
            }
            if (idx.finite_part < inject_position + m) {
                return materialize_at(injected, idx.finite_part - inject_position);
            }
            // элемент upstream после вставки - сдвинут на m
            return materialize_at(upstream, idx.finite_part - m);
        }
        throw std::out_of_range("InsertAtGenerator::get_at: finite insert has no omega blocks");
    }

    // injected бесконечен
    if (idx.omega_part == 0) {
        if (idx.finite_part < inject_position) {
            return materialize_at(upstream, idx.finite_part);
        }
        // {0, p + k} - это k-й элемент injected
        return materialize_at(injected, idx.finite_part - inject_position);
    }
    if (idx.omega_part == 1) {
        // {1, k} - это (inject_position + k)-й элемент upstream (хвост за омегой)
        return materialize_at(upstream, inject_position + idx.finite_part);
    }
    throw std::out_of_range("InsertAtGenerator::get_at: ordinal index beyond omega*2");
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
Cardinal ZipGenerator<U, V, T>::estimate_remaining() const {
    Cardinal first_remaining = first->estimate_remaining();
    Cardinal second_remaining = second->estimate_remaining();

    return (first_remaining < second_remaining) ? first_remaining : second_remaining;
}

template <class U, class V, class T>
Generator<T>* ZipGenerator<U, V, T>::clone() const {
    return new ZipGenerator<U, V, T>(first->clone(), second->clone(), combiner);
}

// ================= ConcatGenerator

template <class T>
ConcatGenerator<T>::ConcatGenerator(Generator<T>* left, Cardinal left_length, Generator<T>* right)
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
    return right->get_next()
}

template <class T>
Option<T> ConcatGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
Cardinal ConcatGenerator<T>::estimate_remaining() const {
    if (left_length.is_infinite()) {
        return Cardinal::infinity() + right->estimate_remaining();
    }
    if (pos < left_length.get_value()) {
        Cardinal left_remaining = Cardinal::finite(left_length.get_value() - pos);
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
    if (idx.omega_part == 0) {
        if (left_length.is_finite()) {
            size_t left_len = left_length.get_value();
            if (idx.finite_part >= left_len) {
                // Перелив в правую часть для конечного left
                return materialize_at(right, idx.finite_part - left_len);
            }
        }
        return materialize_at(left, idx.finite_part);
    }
    if (idx.omega_part == 1) {
        if (!left_length.is_infinite()) {
            throw std::logic_error("ConcatGenerator::get_at: omega_part=1 requires infinite left");
        }
        return materialize_at(right, idx.finite_part);
    }
    throw std::logic_error("ConcatGenerator::get_at: ordinal index beyond omega*2 not supported");
}

template <class T>
ConcatGenerator<T>::~ConcatGenerator() {
    delete left;
    delete right;
}

#endif