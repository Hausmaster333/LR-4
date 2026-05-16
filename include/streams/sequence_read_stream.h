#ifndef SEQUENCE_READ_STREAM_H
#define SEQUENCE_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "core/sequence.h"
#include "core/ienumerator.h"

template <class T>
class SequenceReadStream : public ReadOnlyStream<T> { // Поток для чтения над Sequence, источником не владеем
    private:
        const Sequence<T>* source;
        IEnumerator<T>* iterator;
        size_t total; // Кэш source->get_count
    public:
        // Создаёт стрим над source. source != nullptr иначе throw
        // Стрим в закрытом состоянии, нужно вызвать open
        SequenceReadStream(const Sequence<T>* source);

        bool is_end_of_stream() const override;

        T read() override; // Возвращает текущий элемент через iterator, продвигает iterator + position

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return true; }

        size_t seek(size_t index) override; // Пересоздаёт iterator с начала и проматывает до min(index, total) и Возвращает фактическую позицию (= min(index, total))

        void open() override; // Создаёт iterator от source, position = 0, is_open = true
        void close() override; // Удаляет iterator, position = 0, is_open = false

        ~SequenceReadStream() override;
};

#include "sequence_read_stream.tpp"

#endif
