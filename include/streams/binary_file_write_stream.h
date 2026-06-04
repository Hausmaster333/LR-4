#ifndef BINARY_FILE_WRITE_STREAM_H
#define BINARY_FILE_WRITE_STREAM_H

#include "streams/write_only_stream.h"
#include "streams/stream_exceptions.h"
#include <string>
#include <fstream>

// Пишет элементы T в файл как сырые байты (sizeof(T) байт на элемент, порядок байтов платформы)
template <class T>
class BinaryFileWriteStream : public WriteOnlyStream<T> {
    private:
        std::string path;
        std::ofstream stream;
    public:
        BinaryFileWriteStream(std::string path) : path(std::move(path)) {}

        // Записать значение T как sizeof(T) сырых байт
        size_t write(const T& value) override {
            if (!this->is_open) throw StreamNotOpen();

            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
            if (!stream) throw StreamWriteError("Write failed");

            this->position++;

            return this->position;
        }

        void open() override {
            if (this->is_open) return;

            stream.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!stream) throw StreamWriteError("Cannot open " + path);

            this->is_open = true;
            this->position = 0;
        }

        void close() override {
            if (!this->is_open) return;

            stream.flush();
            stream.close();
            this->is_open = false;
        }

        ~BinaryFileWriteStream() override {
            if (this->is_open) {
                stream.close();
            }
        }
};

#endif
