#ifndef FILE_WRITE_STREAM_H
#define FILE_WRITE_STREAM_H

#include "streams/write_only_stream.h"
#include <string>
#include <fstream>
#include <functional>

template <class T>
class FileWriteStream : public WriteOnlyStream<T> {
    private:
        std::string path;
        std::function<std::string(const T&)> serializer;
        std::ofstream stream;
    public:
        // Создаёт стрим над файлом по path. serializer != nullptr иначе throw
        // Стрим в закрытом состоянии, нужно вызвать open (он откроет файл)
        FileWriteStream(std::string path, std::function<std::string(const T&)> serializer);

        size_t write(const T& value) override; // Сериализует value через serializer, пишет в файл строкой + '\n', возвращает обновлённую positio

        void open() override; // Открывает файл в режиме out | trunc (старое содержимое затирается)
        void close() override; // Сбрасывает буфер, закрывает файл

        ~FileWriteStream() override;
};

#include "file_write_stream.tpp"

#endif
