#ifndef STRING_READ_STREAM_TPP
#define STRING_READ_STREAM_TPP

#include "streams/string_read_stream.h"
#include <stdexcept>

template <class T>
StringReadStream<T>::StringReadStream(std::string source,
                                     std::function<T(const std::string&)> deserializer)
    : ReadOnlyStream<T>(),
      source(std::move(source)),
      deserializer(deserializer),
      cursor(0),
      is_eof_reached(false) {
    if (!this->deserializer) throw std::invalid_argument("StringReadStream: deserializer is empty");
}

template <class T>
std::string StringReadStream<T>::read_line() {
    // Предполагается, что cursor < source.size() и !is_eof_reached
    size_t newline_position = source.find('\n', cursor);
    if (newline_position == std::string::npos) {
        std::string line = source.substr(cursor);
        cursor = source.size();
        is_eof_reached = true;
        return line;
    }

    std::string line = source.substr(cursor, newline_position - cursor);
    cursor = newline_position + 1;
    if (cursor >= source.size()) {
        // Достигли последнего '\n' - следующий read() даст EOS
        is_eof_reached = true;
    }

    return line;
}

template <class T>
bool StringReadStream<T>::is_end_of_stream() const {
    return is_eof_reached && cursor >= source.size();
}

template <class T>
T StringReadStream<T>::read() {
    if (!this->is_open) throw StreamNotOpen();
    if (is_eof_reached && cursor >= source.size()) throw EndOfStream();
    if (cursor >= source.size()) {
        is_eof_reached = true;
        throw EndOfStream();
    }

    line_offsets.append(cursor);
    std::string line = read_line();
    this->position++;

    return deserializer(line);
}

template <class T>
size_t StringReadStream<T>::seek(size_t index) {
    if (!this->is_open) throw StreamNotOpen();

    if (index <= static_cast<size_t>(line_offsets.get_count())) {
        // Прыжок в уже прочитанную область или ровно на следующую за прочитанной
        if (index < this->position) {
            // Назад: усекаем line_offsets до index, восстанавливаем cursor
            MutableArraySequence<size_t> truncated;
            for (size_t step = 0; step < index; step++) {
                truncated.append(line_offsets.get(static_cast<int>(step)));
            }
            line_offsets = truncated;

            if (index == 0) {
                cursor = 0;
            } else {
                // Следующая позиция после последней прочитанной строки
                size_t previous_start = line_offsets.get_last();
                size_t newline_position = source.find('\n', previous_start);
                cursor = (newline_position == std::string::npos) ? source.size() : (newline_position + 1);
            }
            this->position = index;
            is_eof_reached = false;

            return index;
        }
        // index == position - ничего не делаем
        return this->position;
    }

    // index > line_offsets.count: продвигаемся вперёд, отбрасывая результаты
    while (this->position < index && !is_end_of_stream()) {
        if (cursor >= source.size()) {
            is_eof_reached = true;
            break;
        }
        line_offsets.append(cursor);
        read_line();
        this->position++;
    }

    return this->position;
}

template <class T>
void StringReadStream<T>::open() {
    cursor = 0;
    is_eof_reached = source.empty();
    this->position = 0;
    line_offsets = MutableArraySequence<size_t>();
    this->is_open = true;
}

template <class T>
void StringReadStream<T>::close() {
    this->is_open = false;
    cursor = 0;
    this->position = 0;
    is_eof_reached = false;
    line_offsets = MutableArraySequence<size_t>();
}

#endif
