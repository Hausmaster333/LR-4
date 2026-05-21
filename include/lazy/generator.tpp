#ifndef GENERATOR_TPP
#define GENERATOR_TPP

#include "lazy/generator.h"

template <class T>
Sequence<T>* SourceGenerator<T>::copy_of(const Sequence<T>* source) {
    MutableArraySequence<T>* dest = new MutableArraySequence<T>();

    IEnumerator<T>* iter = source->get_enumerator();
    while (iter->move_next()) {
        dest->append(iter->get_current());
    }
    delete iter;

    return dest;
}

template <class T>
SourceGenerator<T>::SourceGenerator(const Sequence<T>* source) : owned(nullptr), owned_iter(nullptr), pos(0), owned_count(0) {
    if (source == nullptr) throw std::invalid_argument("Source is nullptr");

    owned = copy_of(source);
    owned_iter = owned->get_enumerator();
    owned_count = static_cast<size_t>(owned->get_count());
}

template <class T>
SourceGenerator<T>* SourceGenerator<T>::own(Sequence<T>* source) {
    if (source == nullptr) throw std::invalid_argument("Source is nullptr");

    SourceGenerator<T>* generator = new SourceGenerator<T>();
    generator->owned = source;
    generator->owned_iter = source->get_enumerator();
    generator->pos = 0;
    generator->owned_count = static_cast<size_t>(source->get_count());

    return generator;
}

template <class T>
T SourceGenerator<T>::get_next() {
    if (!has_next()) throw std::out_of_range("No more elements");

    owned_iter->move_next();
    T value = owned_iter->get_current();
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
    if (pos >= owned_count) return Ordinal::zero();

    return Ordinal::finite(owned_count - pos);
}

template <class T>
Generator<T>* SourceGenerator<T>::clone() const {
    return new SourceGenerator<T>(owned);
}

template <class T>
T SourceGenerator<T>::get_at(Ordinal idx) const {
    if (idx.get_omega_count() != 0) throw std::out_of_range("Source is finite, omega_part must be 0");
    if (idx.get_finite_part() >= owned_count) throw std::out_of_range("Index past end");

    auto* arr = dynamic_cast<ArraySequence<T>*>(owned);
    if (arr != nullptr) return arr->get(static_cast<int>(idx.get_finite_part()));

    IEnumerator<T>* iter = owned->get_enumerator();
    T value;
    try {
        for (size_t i = 0; i <= idx.get_finite_part(); i++) {
            if (!iter->move_next()) {
                delete iter;
                throw std::out_of_range("Owned exhausted unexpectedly");
            }
            value = iter->get_current();
        }
    } catch (...) {
        delete iter;
        throw;
    }
    delete iter;
    return value;
}

template <class T>
SourceGenerator<T>::~SourceGenerator() {
    delete owned_iter;
    delete owned;
}

// =================

template <class T>
RecurrenceGenerator<T>::RecurrenceGenerator(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial) : rule(rule), k(0), pos(0) {
    if (initial == nullptr) throw std::invalid_argument("Initial is nullptr");
    if (!rule) throw std::invalid_argument("Rule is empty");

    int initial_count = initial->get_count();
    if (initial_count <= 0) throw std::invalid_argument("Initial must have at least 1 element");

    k = static_cast<size_t>(initial_count);

    EnumeratorWrapper<T> iterator(initial->get_enumerator());
    while (iterator.move_next()) {
        this->initial.append(iterator.get_current());
    }
}

template <class T>
T RecurrenceGenerator<T>::get_next() {
    if (pos < k) {
        T value = initial.get(static_cast<int>(pos));
        window.append(value);
        pos++;

        return value;
    }

    // Считаем элемент, после чего создаем новое сдвинутое на этот элемент окно
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

// =================

template <class T>
PrependGenerator<T>::PrependGenerator(const T& item, Generator<T>* upstream) : head_item(item), upstream(upstream), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("Upstream is nullptr");
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

    if (!upstream->has_next()) throw std::out_of_range("Upstream exhausted");

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
T PrependGenerator<T>::get_at(Ordinal idx) const {
    if (idx.get_omega_count() == 0 && idx.get_finite_part() == 0) return head_item;

    Ordinal shifted = (idx.get_omega_count() == 0) ? Ordinal(0, idx.get_finite_part() - 1) : idx;

    return materialize_at(upstream, shifted);
}

template <class T>
PrependGenerator<T>::~PrependGenerator() {
    delete upstream;
}

// =================

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position, const T& item, Generator<T>* upstream)
    : inject_position(inject_position), upstream(upstream), injected(nullptr), injected_length(Ordinal::finite(1)), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("Upstream is nullptr");

    MutableArraySequence<T>* one = new MutableArraySequence<T>();
    one->append(item);
    injected = SourceGenerator<T>::own(one);
}

template <class T>
InsertAtGenerator<T>::InsertAtGenerator(size_t inject_position, Generator<T>* injected, Ordinal injected_length, Generator<T>* upstream)
    : inject_position(inject_position), upstream(upstream), injected(injected), injected_length(injected_length), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("Upstream is nullptr");
    if (injected == nullptr) throw std::invalid_argument("Injected is nullptr");
}

template <class T>
bool InsertAtGenerator<T>::has_next() const {
    Ordinal pos_ord = Ordinal::finite(pos);
    Ordinal injection_start = Ordinal::finite(inject_position);
    Ordinal injection_end = injection_start + injected_length;

    if (pos_ord < injection_start) {
        return upstream->has_next();
    }

    if (pos_ord < injection_end) {
        return injected->has_next();
    }

    return upstream->has_next();
}

template <class T>
T InsertAtGenerator<T>::get_next() {
    Ordinal pos_ord = Ordinal::finite(pos);
    Ordinal injection_start = Ordinal::finite(inject_position);
    Ordinal injection_end = injection_start + injected_length;

    if (pos_ord < injection_start) {
        if (!upstream->has_next()) throw std::out_of_range("Upstream exhausted before inject_position");

        pos++;
        return upstream->get_next();
    }

    if (pos_ord < injection_end) {
        if (!injected->has_next()) throw std::out_of_range("Injected exhausted unexpectedly");

        pos++;
        return injected->get_next();
    }

    if (!upstream->has_next()) throw std::out_of_range("Upstream exhausted");

    pos++;
    return upstream->get_next();
}

template <class T>
Option<T> InsertAtGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
T InsertAtGenerator<T>::get_at(Ordinal idx) const {
    Ordinal injection_start = Ordinal::finite(inject_position);
    Ordinal injection_end = injection_start + injected_length;

    if (idx < injection_start) {
        return materialize_at(upstream, idx);
    }

    if (idx < injection_end) {
        return materialize_at(injected, idx - injection_start);
    }

    return materialize_at(upstream, injection_start + (idx - injection_end));
}

template <class T>
Ordinal InsertAtGenerator<T>::estimate_remaining() const {
    Ordinal pos_ord = Ordinal::finite(pos);
    Ordinal injection_start = Ordinal::finite(inject_position);
    Ordinal injection_end = injection_start + injected_length;

    Ordinal upstream_remaining = upstream->estimate_remaining();

    if (pos_ord < injection_start) {
        return upstream_remaining + injected_length;
    }

    if (pos_ord < injection_end) {
        return (injection_end - pos_ord) + upstream_remaining;
    }

    return upstream_remaining;
}

template <class T>
Generator<T>* InsertAtGenerator<T>::clone() const {
    return new InsertAtGenerator<T>(inject_position, injected->clone(), injected_length, upstream->clone());
}

template <class T>
InsertAtGenerator<T>::~InsertAtGenerator() {
    delete upstream;
    delete injected;
}

// =================

template <class U, class T>
MapGenerator<U, T>::MapGenerator(Generator<U>* upstream, std::function<T(const U&)> func) : upstream(upstream), func(func), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("Upstream is nullptr");
    if (!func) throw std::invalid_argument("Func is empty");
}

template <class U, class T>
T MapGenerator<U, T>::get_next() {
    if (!upstream->has_next()) throw std::out_of_range("Upstream exhausted");

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
T MapGenerator<U, T>::get_at(Ordinal idx) const {
    return func(materialize_at(upstream, idx));
}

template <class U, class T>
MapGenerator<U, T>::~MapGenerator() {
    delete upstream;
}

// =================

template <class T>
WhereGenerator<T>::WhereGenerator(Generator<T>* upstream, std::function<bool(const T&)> pred) : upstream(upstream), pred(pred), pos(0) {
    if (upstream == nullptr) throw std::invalid_argument("Upstream is nullptr");
    if (!pred) throw std::invalid_argument("Predicate is empty");
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

    throw std::out_of_range("Upstream exhausted");
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

template <class T>
WhereGenerator<T>::~WhereGenerator() {
    delete upstream;
}

// =================

template <class U, class V, class T>
ZipGenerator<U, V, T>::ZipGenerator(Generator<U>* first, Generator<V>* second, std::function<T(const U&, const V&)> combiner) : first(first), second(second), combiner(combiner), pos(0) {
    if (first == nullptr || second == nullptr) throw std::invalid_argument("Nullptr operand");
    if (!combiner) throw std::invalid_argument("Combiner is empty");
}

template <class U, class V, class T>
ZipGenerator<U, V, T>::~ZipGenerator() {
    delete first;
    delete second;
}

template <class U, class V, class T>
T ZipGenerator<U, V, T>::get_next() {
    if (!has_next()) throw std::out_of_range("One of operands exhausted");

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
T ZipGenerator<U, V, T>::get_at(Ordinal idx) const {
    U u = materialize_at(first, idx);
    V v = materialize_at(second, idx);

    return combiner(u, v);
}

// =================

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

    if (!right->has_next()) throw std::out_of_range("Both sides exhausted");

    pos++;
    return right->get_next();
}

template <class T>
Option<T> ConcatGenerator<T>::try_get_next() {
    if (!has_next()) return Option<T>::None();

    return Option<T>::Some(get_next());
}

template <class T>
T ConcatGenerator<T>::get_at(Ordinal idx) const {
    if (idx < left_length) {
        return materialize_at(left, idx);
    }

    Ordinal right_idx = idx - left_length;
    return materialize_at(right, right_idx);
}

template <class T>
Ordinal ConcatGenerator<T>::estimate_remaining() const {
    Ordinal pos_ord = Ordinal::finite(pos);
    Ordinal left_remaining = (pos_ord < left_length) ? left_length - pos_ord : Ordinal::zero();

    return left_remaining + right->estimate_remaining();
}

template <class T>
Generator<T>* ConcatGenerator<T>::clone() const {
    return new ConcatGenerator<T>(left->clone(), left_length, right->clone());
}

template <class T>
ConcatGenerator<T>::~ConcatGenerator() {
    delete left;
    delete right;
}

#endif