#ifndef WRITE_ONLY_STREAM_H
#define WRITE_ONLY_STREAM_H

#include "streams/stream_exceptions.h"

template <class T>
class WriteOnlyStream {
    protected:
        bool is_open;
        size_t position; // Текущая позиция в потоке
        WriteOnlyStream() : is_open(false), position(0) {}
    public:
        size_t get_position() const { return position; } // Число уже записанных элементов с момента открытия

        virtual size_t write(const T& value) = 0; // Записать элемент в конец потока, вернуть позицию после записи

        virtual void open() = 0;
        virtual void close() = 0;

        bool opened() const { return is_open; }

        virtual ~WriteOnlyStream() = default;
};

#endif
