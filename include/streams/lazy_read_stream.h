#ifndef LAZY_READ_STREAM_H
#define LAZY_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "lazy/lazy_sequence.h"

template <class T>
class LazyReadStream : public ReadOnlyStream<T> { // Поток для чтения над LazySequence, источником не владеем
    private:
        LazySequence<T>* source; // Не владеем
    public:
        // Создаёт стрим над source. source != nullptr иначе throw
        // Стрим в закрытом состоянии, нужно вызвать open
        explicit LazyReadStream(LazySequence<T>* source);

        bool is_end_of_stream() const override; // True если source финитный и position достиг его длины, на бесконечной LazySequence всегда возвращает false

        T read() override; // Вызывает source->get(position), продвигает position

        bool is_can_seek() const override { return true; } // Вперед можем
        bool is_can_go_back() const override { return false; } // Назад нет, кэш не позволяет

        size_t seek(size_t index) override; // Прыжок только вперёд, position перепрыгивает на index, материализация при следующем read и жля финитной source ограничивает index до длины

        void open() override;
        void close() override;

        ~LazyReadStream() override = default;
};

#include "lazy_read_stream.tpp"

#endif
