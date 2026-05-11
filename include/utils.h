#ifndef UTILS_H
#define UTILS_H

#include "core/sequence.h"
#include "tuples.h"

template <class T>
Sequence<Sequence<T>*>* split(const Sequence<T>* seq, bool (*predicate)(const T&)) {
    if (seq == nullptr) throw std::invalid_argument("Cannot split nullptr sequence");
    if (predicate == nullptr) throw std::invalid_argument("Cannot split with nullptr predicate");

    auto* sequences = new MutableArraySequence<Sequence<T>*>();
    auto* current_seq = new MutableArraySequence<T>();

    EnumeratorWrapper<T> iter(seq->get_enumerator());
    while (iter.move_next()) {
        if (predicate(iter.get_current())) {
            sequences->append(current_seq);
            current_seq = new MutableArraySequence<T>();
        } else {
            current_seq->append(iter.get_current());
        }
    }

    sequences->append(current_seq);

    return sequences;
}

template <class T1, class T2>
Sequence<Pair<T1, T2>>* zip(const Sequence<T1>* first_seq, const Sequence<T2>* second_seq) {
    if (first_seq == nullptr || second_seq == nullptr) throw std::invalid_argument("Cannot zip with nullptr");

    auto* result = new MutableArraySequence<Pair<T1, T2>>();

    EnumeratorWrapper<T1> first_seq_iter(first_seq->get_enumerator());
    EnumeratorWrapper<T2> second_seq_iter(second_seq->get_enumerator());

    while (first_seq_iter.move_next() && second_seq_iter.move_next()) {
        result->append(Pair<T1, T2>(first_seq_iter.get_current(), second_seq_iter.get_current()));
    }

    return result;
}

template <class T1, class T2>
Pair<Sequence<T1>*, Sequence<T2>*> unzip(const Sequence<Pair<T1, T2>>* seq) {
    if (seq == nullptr) throw std::invalid_argument("Cannot unzip nullptr");

    auto* first_seq = new MutableArraySequence<T1>();
    auto* second_seq = new MutableArraySequence<T2>();

    EnumeratorWrapper<Pair<T1, T2>> iter(seq->get_enumerator());
    while (iter.move_next()) {
        const Pair<T1, T2>& curr_pair = iter.get_current();

        first_seq->append(curr_pair.first());
        second_seq->append(curr_pair.second());
    }

    return Pair<Sequence<T1>*, Sequence<T2>*>(first_seq, second_seq);
}

template <class T1, class T2, class T3>
Sequence<Triple<T1, T2, T3>>* zip3(const Sequence<T1>* first_seq, const Sequence<T2>* second_seq, const Sequence<T3>* third_seq) {
    if (first_seq == nullptr || second_seq == nullptr || third_seq == nullptr) throw std::invalid_argument("Cannot zip3 with nullptr");

    auto* result = new MutableArraySequence<Triple<T1, T2, T3>>();

    EnumeratorWrapper<T1> first_seq_iter(first_seq->get_enumerator());
    EnumeratorWrapper<T2> second_seq_iter(second_seq->get_enumerator());
    EnumeratorWrapper<T3> third_seq_iter(third_seq->get_enumerator());

    while (first_seq_iter.move_next() && second_seq_iter.move_next() && third_seq_iter.move_next()) {
        result->append(Triple<T1, T2, T3>(first_seq_iter.get_current(), second_seq_iter.get_current(), third_seq_iter.get_current()));
    }

    return result;
}

template <class T1, class T2, class T3>
Triple<Sequence<T1>*, Sequence<T2>*, Sequence<T3>*> unzip3(const Sequence<Triple<T1, T2, T3>>* seq) {
    if (seq == nullptr) throw std::invalid_argument("Cannot unzip3 nullptr");

    auto* first_seq = new MutableArraySequence<T1>();
    auto* second_seq = new MutableArraySequence<T2>();
    auto* third_seq = new MutableArraySequence<T3>();

    EnumeratorWrapper<Triple<T1, T2, T3>> iter(seq->get_enumerator());
    while (iter.move_next()) {
        const Triple<T1, T2, T3>& curr_triple = iter.get_current();

        first_seq->append(curr_triple.first());
        second_seq->append(curr_triple.second());
        third_seq->append(curr_triple.third());
    }

    return Triple<Sequence<T1>*, Sequence<T2>*, Sequence<T3>*>(first_seq, second_seq, third_seq);
}

#endif