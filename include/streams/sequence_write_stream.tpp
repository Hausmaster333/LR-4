#ifndef SEQUENCE_WRITE_STREAM_TPP
#define SEQUENCE_WRITE_STREAM_TPP

#include "streams/sequence_write_stream.h"
#include <stdexcept>

template <class T>
SequenceWriteStream<T>::SequenceWriteStream(Sequence<T>* destination)
    : WriteOnlyStream<T>(), destination(destination) {
    if (destination == nullptr) {
        throw std::invalid_argument("SequenceWriteStream: destination is nullptr");
    }
}

template <class T>
size_t SequenceWriteStream<T>::write(const T& value) {
    if (!this->is_open) throw StreamNotOpen();

    destination->append(value);
    this->position++;

    return this->position;
}

template <class T>
void SequenceWriteStream<T>::open() {
    this->is_open = true;
}

template <class T>
void SequenceWriteStream<T>::close() {
    this->is_open = false;
}

#endif
