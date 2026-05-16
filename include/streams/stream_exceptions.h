#ifndef STREAM_EXCEPTIONS_H
#define STREAM_EXCEPTIONS_H

#include <stdexcept>
#include <string>

// Достигнут конец потока - следующее чтение невозможно
struct EndOfStream : public std::runtime_error {
    EndOfStream() : std::runtime_error("End of stream reached") {}
};

// Stream не поддерживает seek
struct SeekUnsupported : public std::logic_error {
    SeekUnsupported() : std::logic_error("Stream does not support seek") {}
};

// Попытка seek в уже прочитанную позицию у потока, который не умеет назад
struct GoBackUnsupported : public std::logic_error {
    GoBackUnsupported() : std::logic_error("Stream does not support going back") {}
};

// Операция над незакрытым/неоткрытым потоком
struct StreamNotOpen : public std::logic_error {
    StreamNotOpen() : std::logic_error("Stream is not open") {}
};

// Ошибка записи или открытия файла
struct StreamWriteError : public std::runtime_error {
    StreamWriteError(const std::string& message) : std::runtime_error(message) {}
};

#endif
