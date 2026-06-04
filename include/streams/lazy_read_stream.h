#ifndef LAZY_READ_STREAM_H
#define LAZY_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "lazy/lazy_sequence.h"

template <class T>
class LazyReadStream : public ReadOnlyStream<T> {
    private:
        LazySequence<T>* source;
    public:
        LazyReadStream(LazySequence<T>* source);

        bool is_end_of_stream() const override;

        T read() override;

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return false; }

        size_t seek(size_t index) override;

        void open() override;
        void close() override;

        ~LazyReadStream() override = default;
};

#include "lazy_read_stream.tpp"

#endif
