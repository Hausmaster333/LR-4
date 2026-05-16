#ifndef FILE_READ_STREAM_H
#define FILE_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "core/sequence.h"
#include <string>
#include <fstream>
#include <functional>

template <class T>
class FileReadStream : public ReadOnlyStream<T> {
    private:
        std::string path;
        std::function<T(const std::string&)> deserializer;
        mutable std::ifstream stream; // mutable - is_end_of_stream() делает peek
        MutableArraySequence<std::streampos> line_offsets; // Оффсеты начал прочитанных строк
        bool is_eof_reached;
    public:
        // Создаёт стрим над файлом по path. deserializer != nullptr иначе throw
        // Стрим в закрытом состоянии, нужно вызвать open
        FileReadStream(std::string path, std::function<T(const std::string&)> deserializer);

        bool is_end_of_stream() const override; // True если файл закончился = peek даёт EOF

        T read() override; // Читает следующую строку через std::getline, десериализует и возвращает T

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return true; }

        // Для хода назад переоткрывает файл, проматывает до line_offsets[index-1]
        // Для хода вперед дочитывает строки без сохранения значений до index или EOS
        size_t seek(size_t index) override; // Возвращает фактическую достигнутую позицию

        void open() override; // Открывает файл в text-mode, position = 0, очищает line_offsets
        void close() override; // Закрывает файл, position = 0, очищает line_offsets

        ~FileReadStream() override;
};

#include "file_read_stream.tpp"

#endif
