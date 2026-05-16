#ifndef STRING_READ_STREAM_H
#define STRING_READ_STREAM_H

#include "streams/read_only_stream.h"
#include "core/sequence.h"
#include <string>
#include <functional>

// Поток только-для-чтения над std::string. Источник разбивается по '\n'
// на «строки», каждая строка десериализуется через переданный std::function в T
// can_seek = true, can_go_back = true (line_offsets хранит начала уже прочитанных строк)
template <class T>
class StringReadStream : public ReadOnlyStream<T> {
    private:
        std::string source;
        std::function<T(const std::string&)> deserializer;
        size_t cursor; // Байтовый, начало следующей строки в source
        MutableArraySequence<size_t> line_offsets; // Оффсеты начал прочитанных строк
        bool is_eof_reached;

        std::string read_line(); // Прочитать строку начиная с cursor, обновить cursor, возвращает строку без '\n' в конце
    public:
        // Создаёт стрим над копией source. deserializer != nullptr иначе throw
        // Стрим в закрытом состоянии, нужно вызвать open
        StringReadStream(std::string source, std::function<T(const std::string&)> deserializer);

        bool is_end_of_stream() const override; // True если cursor дошёл до конца source и больше строк нет

        T read() override; // Прочитать следующую строку, десериализовать и вернуть как T

        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return true; }

        // Для хода назад усекает line_offsets до index, восстанавливает cursor на следующую строку после прочитанной
        // Для хода вперед дочитывает без сохранения значений до index или EOS
        size_t seek(size_t index) override; // Возвращает фактическую достигнутую позицию

        void open() override; // Сброс позиции, cursor = 0, очистка line_offsets
        void close() override; // Закрыть, сбросить всё

        ~StringReadStream() override = default;
};

#include "string_read_stream.tpp"

#endif
