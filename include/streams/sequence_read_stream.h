#ifndef SEQUENCE_READ_STREAM_H
#define SEQUENCE_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "core/sequence.h"
#include "core/ienumerator.h"

template <class T>
class SequenceReadStream : public ReadOnlyStream<T> {
    private:
        const Sequence<T>* source;
        IEnumerator<T>* iterator;
        size_t total;
    public:
        SequenceReadStream(const Sequence<T>* source);

        bool is_end_of_stream() const override;

        T read() override;

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return true; }

        size_t seek(size_t index) override; // Пересоздаёт итератор с начала и проходит до min(index, total) и возвращает конечную позицию

        void open() override;
        void close() override;

        ~SequenceReadStream() override;
};

#include "sequence_read_stream.tpp"

#endif
