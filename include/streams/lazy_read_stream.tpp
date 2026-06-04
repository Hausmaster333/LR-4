#ifndef LAZY_READ_STREAM_TPP
#define LAZY_READ_STREAM_TPP

#include "streams/lazy_read_stream.h"
#include <stdexcept>

template <class T>
LazyReadStream<T>::LazyReadStream(LazySequence<T>* source) : ReadOnlyStream<T>(), source(source) {
    if (source == nullptr) throw std::invalid_argument("Source is nullptr");
}

template <class T>
bool LazyReadStream<T>::is_end_of_stream() const {
    Ordinal length = source->get_length();
    if (length.is_infinite()) return false;

    return this->position >= length.get_value();
}

template <class T>
T LazyReadStream<T>::read() {
    if (!this->is_open) throw StreamNotOpen();
    if (is_end_of_stream()) throw EndOfStream();

    T value = source->get(static_cast<int>(this->position));
    this->position++;

    return value;
}

template <class T>
size_t LazyReadStream<T>::seek(size_t index) {
    if (!this->is_open) throw StreamNotOpen();
    if (index < this->position) throw GoBackUnsupported();

    Ordinal length = source->get_length();
    if (length.is_finite() && index > length.get_value()) {
        index = length.get_value();
    }

    this->position = index;

    return this->position;
}

template <class T>
void LazyReadStream<T>::open() {
    this->is_open = true;
    this->position = 0;
}

template <class T>
void LazyReadStream<T>::close() {
    this->is_open = false;
    this->position = 0;
}

#endif
