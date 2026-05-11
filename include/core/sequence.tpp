#include "core/sequence.h"

template <class T>
Sequence<T>* Sequence<T>::slice(int index, int count, const Sequence<T>* replace_seq) const {
    if (count < 0) throw std::invalid_argument("Slice count cannot be negative");

    int len = get_count();

    if (index < 0) {
        index = len + index;
    }

    if (index < 0 || index >= len) {
        throw std::out_of_range("Slice index out of range");
    }

    if (count > len - index) {
        count = len - index;
    }

    Sequence<T>* sliced_seq = CreateEmpty();

    // Элементы до index, и после index + count
    EnumeratorWrapper<T> iter(get_enumerator());
    int curr_idx = 0;
    while (iter.move_next()) {
        if (curr_idx < index) {
            sliced_seq->sys_append(iter.get_current());
        } else if (curr_idx == index && replace_seq != nullptr) {
            // Вставляем replace_seq один раз при достижении index
            EnumeratorWrapper<T> rep_iter(replace_seq->get_enumerator());

            while (rep_iter.move_next()) {
                sliced_seq->sys_append(rep_iter.get_current());
            }
        }
        if (curr_idx >= index + count) {
            sliced_seq->sys_append(iter.get_current());
        }
        curr_idx++;
    }

    return sliced_seq;
}

template <class T>
T Sequence<T>::reduce(T (*func)(const T& accumulator, const T& current), const T& initial_elem) const {
    if (func == nullptr) throw std::invalid_argument("Cannot reduce with nullptr function");

    T reduced_elem = initial_elem;

    EnumeratorWrapper<T> iter(get_enumerator());
    while (iter.move_next()) {
        T current_elem = iter.get_current();
        reduced_elem = func(current_elem, reduced_elem);
    }

    return reduced_elem;
}

template <class T>
Sequence<T>* Sequence<T>::map(T (*func)(const T& elem)) const {
    if (func == nullptr) throw std::invalid_argument("Cannot map with nullptr function");

    Sequence<T>* result = CreateEmpty();

    EnumeratorWrapper<T> iter(get_enumerator());
    while (iter.move_next()) {
        result->sys_append(func(iter.get_current()));
    }
    return result;
}

template <class T>
Sequence<T>* Sequence<T>::where(bool (*predicate)(const T& elem)) const {
    if (predicate == nullptr) throw std::invalid_argument("Cannot where with nullptr predicate");

    Sequence<T>* result = CreateEmpty();

    EnumeratorWrapper<T> iter(get_enumerator());
    while (iter.move_next()) {
        const T& elem = iter.get_current();
        if (predicate(elem)) {
            result->sys_append(elem);
        }
    }
    return result;
}

template <class T>
Sequence<T>* Sequence<T>::concat(const Sequence<T>* other) const {
    if (other == nullptr) throw std::invalid_argument("Cannot concat with nullptr");

    Sequence<T>* result = CreateEmpty();

    EnumeratorWrapper<T> this_iter(get_enumerator());
    while (this_iter.move_next()) {
        result->sys_append(this_iter.get_current());
    }

    EnumeratorWrapper<T> other_iter(other->get_enumerator());
    while (other_iter.move_next()) {
        result->sys_append(other_iter.get_current());
    }

    return result;
}

template <class T>
Sequence<T>* Sequence<T>::get_sub_sequence(int start, int end) const {
    if (start < 0 || end < 0 || start >= get_count() || end >= get_count() || start > end) {
        throw std::out_of_range("Index out of range");
    }

    Sequence<T>* result = CreateEmpty();

    int curr_index = 0;
    EnumeratorWrapper<T> iter(get_enumerator());
    while (iter.move_next()) {
        if (curr_index >= start && curr_index <= end) {
            result->sys_append(iter.get_current());
        }

        if (curr_index == end) {
            break;
        }

        curr_index++;
    }

    return result;
}

template <class T>
void ArraySequence<T>::sys_append(const T& item) {
    if (count >= array.get_size()) {
        int new_size = (array.get_size() == 0) ? 4 : array.get_size() * 2;
        array.resize(new_size);
    }
    array.set(count, item);
    count++;
}

template <class T>
ArraySequence<T>::ArraySequence() : count(0) {
    array = DynamicArray<T>();
}

template <class T>
ArraySequence<T>::ArraySequence(const T* items, int count) : count(count) {
    array = DynamicArray<T>(items, count);
}

template <class T>
ArraySequence<T>::ArraySequence(const DynamicArray<T>& other) : count(other.get_size()) {
    array = DynamicArray<T>(other);
}

template <class T>
ArraySequence<T>::ArraySequence(const ArraySequence<T>& other) : count(other.count) {
    array = DynamicArray<T>(other.array);
}

template <class T>
const T& ArraySequence<T>::get_first() const {
    if (count == 0) throw std::out_of_range("Sequence is empty");

    return array.get(0);
}

template <class T>
const T& ArraySequence<T>::get_last() const {
    if (count == 0) throw std::out_of_range("Sequence is empty");

    int last_index = get_count() - 1;
    return array.get(last_index);
}

template <class T>
const T& ArraySequence<T>::get(int index) const {
    if (index < 0 || index >= count) throw std::out_of_range("Index out of range");

    return array.get(index);
}

template <class T>
Option<T> ArraySequence<T>::try_get_first() const {
    if (count == 0) return Option<T>::None();

    return Option<T>::Some(array.get(0));
}

template <class T>
Option<T> ArraySequence<T>::try_get_last() const {
    if (count == 0) return Option<T>::None();

    int last_index = get_count() - 1;
    return Option<T>::Some(array.get(last_index));
}

template <class T>
Option<T> ArraySequence<T>::try_get(int index) const {
    if (index < 0 || index >= count) return Option<T>::None();

    return Option<T>::Some(array.get(index));
}

template <class T>
int ArraySequence<T>::get_count() const {
    return count;
}

template <class T>
Sequence<T>* ArraySequence<T>::append(const T& item) {
    ArraySequence<T>* inst = Instance();

    int size = inst->array.get_size();

    if (inst->count >= size) {
        int new_size = (size == 0) ? 4 : size * 2;
        inst->array.resize(new_size);
    }

    inst->array.set(inst->count, item);
    inst->count++;

    return inst;
}

template <class T>
Sequence<T>* ArraySequence<T>::prepend(const T& item) {
    ArraySequence<T>* inst = Instance();

    int size = inst->array.get_size();

    if (inst->count >= size) {
        int new_size = (size == 0) ? 4 : size * 2;
        inst->array.resize(new_size);
    }

    for (int idx = inst->count; idx > 0; idx--) {
        inst->array.set(idx, inst->array.get(idx - 1));
    }

    inst->array.set(0, item);
    inst->count++;
    return inst;
}

template <class T>
Sequence<T>* ArraySequence<T>::insert_at(const T& item, int index) {
    ArraySequence<T>* inst = Instance();

    if (index < 0 || index > inst->count) {
        throw std::out_of_range("Index out of range");
    }

    int size = inst->array.get_size();

    if (inst->count >= size) {
        int new_size = (size == 0) ? 4 : size * 2;
        inst->array.resize(new_size);
    }

    for (int idx = inst->count; idx > index; idx--) {
        inst->array.set(idx, inst->array.get(idx - 1));
    }

    inst->array.set(index, item);
    inst->count++;
    return inst;
}

// ==================================================

template <class T>
void ListSequence<T>::sys_append(const T& item) {
    list.append(item);
}

template <class T>
ListSequence<T>::ListSequence() {
    list = LinkedList<T>();
}

template <class T>
ListSequence<T>::ListSequence(const T* items, int count) {
    list = LinkedList<T>(items, count);
}

template <class T>
ListSequence<T>::ListSequence(const LinkedList<T>& other) {
    list = LinkedList<T>(other);
}

template <class T>
ListSequence<T>::ListSequence(const ListSequence<T>& other) {
    list = LinkedList<T>(other.list);
}

template <class T>
const T& ListSequence<T>::get_first() const {
    if (list.get_length() == 0) throw std::out_of_range("Sequence is empty");
    
    return list.get_first();
}

template <class T>
const T& ListSequence<T>::get_last() const {
    if (list.get_length() == 0) throw std::out_of_range("Sequence is empty");

    return list.get_last();
}

template <class T>
Option<T> ListSequence<T>::try_get_first() const {
    if (list.get_length() == 0) return Option<T>::None();

    return Option<T>::Some(list.get_first());
}

template <class T>
Option<T> ListSequence<T>::try_get_last() const {
    if (list.get_length() == 0) return Option<T>::None();

    return Option<T>::Some(list.get_last());
}

template <class T>
int ListSequence<T>::get_count() const {
    return list.get_length();
}

template <class T>
Sequence<T>* ListSequence<T>::append(const T& item) {
    ListSequence<T>* inst = Instance();

    inst->list.append(item);
    return inst;
}

template <class T>
Sequence<T>* ListSequence<T>::prepend(const T& item) {
    ListSequence<T>* inst = Instance();

    inst->list.prepend(item);
    return inst;
}

template <class T>
Sequence<T>* ListSequence<T>::insert_at(const T& item, int index) {
    ListSequence<T>* inst = Instance();

    inst->list.insert_at(item, index);
    return inst;
}
