#ifndef SEQUENCE_READ_STREAM_TPP
#define SEQUENCE_READ_STREAM_TPP

#include "streams/sequence_read_stream.h"
#include <stdexcept>

template <class T>
SequenceReadStream<T>::SequenceReadStream(const Sequence<T>* source) : ReadOnlyStream<T>(), source(source), iterator(nullptr), total(0) {
    if (source == nullptr) throw std::invalid_argument("Source is nullptr");

    total = static_cast<size_t>(source->get_count());
}

template <class T>
SequenceReadStream<T>::~SequenceReadStream() {
    delete iterator;
}

template <class T>
bool SequenceReadStream<T>::is_end_of_stream() const {
    return this->position >= total;
}

template <class T>
T SequenceReadStream<T>::read() {
    if (!this->is_open) throw StreamNotOpen();
    if (is_end_of_stream()) throw EndOfStream();

    iterator->move_next();
    T value = iterator->get_current();
    this->position++;

    return value;
}

template <class T>
size_t SequenceReadStream<T>::seek(size_t index) {
    if (!this->is_open) throw StreamNotOpen();

    delete iterator;
    iterator = source->get_enumerator();
    size_t target = (index > total) ? total : index;

    for (size_t step = 0; step < target; step++) {
        iterator->move_next();
    }

    this->position = target;

    return target;
}

template <class T>
void SequenceReadStream<T>::open() {
    if (this->is_open) return;

    delete iterator;
    iterator = source->get_enumerator();
    this->position = 0;
    this->is_open = true;
}

template <class T>
void SequenceReadStream<T>::close() {
    delete iterator;
    iterator = nullptr;
    this->is_open = false;
    this->position = 0;
}

#endif
