#ifndef DEFERRED_TAIL_TPP
#define DEFERRED_TAIL_TPP

#include "lazy/deferred_tail.h"

template <class T>
void DeferredTail<T>::push_append(const T& item) {
    Op operation;
    operation.kind = Kind::AppendOne;
    operation.single = item;
    ops.append(operation);
}

template <class T>
void DeferredTail<T>::push_concat(const Sequence<T>* other) {
    if (other == nullptr) throw std::invalid_argument("DeferredTail::push_concat: other is nullptr");

    Op operation;
    operation.kind = Kind::ConcatSeq;
    EnumeratorWrapper<T> iterator(other->get_enumerator());
    while (iterator.move_next()) {
        operation.seq.append(iterator.get_current());
    }
    ops.append(operation);
}

template <class T>
Cardinal DeferredTail<T>::get_added_length() const {
    size_t total = 0;
    int op_count = ops.get_count();

    for (int index = 0; index < op_count; index++) {
        const Op& operation = ops.get(index);

        if (operation.kind == Kind::AppendOne) {
            total += 1;
        } else {
            total += static_cast<size_t>(operation.seq.get_count());
        }
    }

    return Cardinal::finite(total);
}

template <class T>
T DeferredTail<T>::get(int tail_index) const {
    if (tail_index < 0) throw std::out_of_range("DeferredTail::get: negative index");

    int cursor = 0;
    int op_count = ops.get_count();

    for (int index = 0; index < op_count; index++) {
        const Op& operation = ops.get(index);

        if (operation.kind == Kind::AppendOne) {
            if (cursor == tail_index) return operation.single;
            cursor += 1;
        } else {
            int seq_length = operation.seq.get_count();
            if (tail_index < cursor + seq_length) {
                return operation.seq.get(tail_index - cursor);
            }
            cursor += seq_length;
        }
    }

    throw std::out_of_range("DeferredTail::get: index out of range");
}

template <class T>
void DeferredTail<T>::apply_to(MutableArraySequence<T>& target) const {
    int op_count = ops.get_count();

    for (int index = 0; index < op_count; index++) {
        const Op& operation = ops.get(index);

        if (operation.kind == Kind::AppendOne) {
            target.append(operation.single);
        } else {
            EnumeratorWrapper<T> iterator(operation.seq.get_enumerator());
            while (iterator.move_next()) target.append(iterator.get_current());
        }
    }
}

#endif
