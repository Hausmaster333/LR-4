#ifndef BINARY_FILE_READ_STREAM_H
#define BINARY_FILE_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "streams/stream_exceptions.h"
#include <string>
#include <fstream>

// Читает элементы T из файла как сырые байты (sizeof(T) байт на элемент)
template <class T>
class BinaryFileReadStream : public ReadOnlyStream<T> {
    private:
        std::string path;
        mutable std::ifstream stream;
    public:
        BinaryFileReadStream(std::string path) : path(std::move(path)) {}

        bool is_end_of_stream() const override {
            if (!this->is_open) return false;

            return stream.peek() == std::char_traits<char>::eof();
        }

        // Прочитать sizeof(T) байт как одно значение T
        T read() override {
            if (!this->is_open) throw StreamNotOpen();

            T value;
            stream.read(reinterpret_cast<char*>(&value), sizeof(T));
            if (stream.gcount() != static_cast<std::streamsize>(sizeof(T))) throw EndOfStream();

            this->position++;

            return value;
        }

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return false; }

        size_t seek(size_t index) override {
            if (!this->is_open) throw StreamNotOpen();
            if (index < this->position) throw GoBackUnsupported();

            while (this->position < index && !is_end_of_stream()) {
                read();
            }

            return this->position;
        }

        void open() override {
            if (this->is_open) return;

            stream.open(path, std::ios::in | std::ios::binary);
            if (!stream) throw std::runtime_error("Cannot open " + path);

            this->is_open = true;
            this->position = 0;
        }

        void close() override {
            if (!this->is_open) return;

            stream.close();
            this->is_open = false;
            this->position = 0;
        }

        ~BinaryFileReadStream() override {
            if (this->is_open) {
                stream.close();
            }
        }
};

#endif
