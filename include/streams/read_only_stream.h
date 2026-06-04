#ifndef READ_ONLY_STREAM_H
#define READ_ONLY_STREAM_H

#include "streams/stream_exceptions.h"

template <class T>
class ReadOnlyStream {
    protected:
        bool is_open;
        size_t position;
        ReadOnlyStream() : is_open(false), position(0) {}
    public:
        virtual bool is_end_of_stream() const = 0; // Проверка на конец потока

        virtual T read() = 0; // Читаем элемент, двигаем позицию

        size_t get_position() const { return position; }

        virtual bool is_can_seek() const = 0; // Поддерживает ли поток переход вперёд по индексу
        virtual bool is_can_go_back() const = 0; // Поддерживает ли поток переход назад по индексу
        virtual size_t seek(size_t index) = 0; // Переход на index. Возвращает фактическую позицию

        virtual void open() = 0; // Открыть поток
        virtual void close() = 0; // Закрыть поток

        bool opened() const { return is_open; } // Проверка на открытость потока

        virtual ~ReadOnlyStream() = default;
};

#endif
