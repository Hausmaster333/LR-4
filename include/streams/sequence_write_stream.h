#ifndef SEQUENCE_WRITE_STREAM_H
#define SEQUENCE_WRITE_STREAM_H

#include "streams/write_only_stream.h"
#include "core/sequence.h"

template <class T>
class SequenceWriteStream : public WriteOnlyStream<T> {
    private:
        Sequence<T>* destination;
    public:
        SequenceWriteStream(Sequence<T>* destination);

        size_t write(const T& value) override;

        void open() override;
        void close() override;

        ~SequenceWriteStream() override = default;
};

#include "sequence_write_stream.tpp"

#endif
