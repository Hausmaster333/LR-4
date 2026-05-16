#ifndef FILE_WRITE_STREAM_TPP
#define FILE_WRITE_STREAM_TPP

#include "streams/file_write_stream.h"
#include <stdexcept>

template <class T>
FileWriteStream<T>::FileWriteStream(std::string path,
                                    std::function<std::string(const T&)> serializer)
    : WriteOnlyStream<T>(),
      path(std::move(path)),
      serializer(serializer) {
    if (!this->serializer) throw std::invalid_argument("FileWriteStream: serializer is empty");
}

template <class T>
FileWriteStream<T>::~FileWriteStream() {
    if (this->is_open) stream.close();
}

template <class T>
size_t FileWriteStream<T>::write(const T& value) {
    if (!this->is_open) throw StreamNotOpen();

    stream << serializer(value) << '\n';
    if (!stream) throw StreamWriteError("FileWriteStream: write failed");
    this->position++;

    return this->position;
}

template <class T>
void FileWriteStream<T>::open() {
    if (this->is_open) return;

    stream.open(path, std::ios::out | std::ios::trunc);
    if (!stream) throw StreamWriteError("FileWriteStream: cannot open " + path);
    this->is_open = true;
    this->position = 0;
}

template <class T>
void FileWriteStream<T>::close() {
    if (!this->is_open) return;

    stream.flush();
    stream.close();
    this->is_open = false;
}

#endif
