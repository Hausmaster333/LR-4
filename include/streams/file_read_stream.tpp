#ifndef FILE_READ_STREAM_TPP
#define FILE_READ_STREAM_TPP

#include "streams/file_read_stream.h"
#include <stdexcept>

template <class T>
FileReadStream<T>::FileReadStream(std::string path,
                                  std::function<T(const std::string&)> deserializer)
    : ReadOnlyStream<T>(),
      path(std::move(path)),
      deserializer(deserializer),
      is_eof_reached(false) {
    if (!this->deserializer) throw std::invalid_argument("FileReadStream: deserializer is empty");
}

template <class T>
FileReadStream<T>::~FileReadStream() {
    if (this->is_open) stream.close();
}

template <class T>
bool FileReadStream<T>::is_end_of_stream() const {
    if (!this->is_open) return false;
    if (is_eof_reached) return true;
    // peek без потребления: вернёт EOF, если данных больше нет
    // peek может выставить eofbit в потоке - это допустимо (mutable stream)
    return stream.peek() == std::char_traits<char>::eof();
}

template <class T>
T FileReadStream<T>::read() {
    if (!this->is_open) throw StreamNotOpen();
    if (is_eof_reached) throw EndOfStream();

    line_offsets.append(stream.tellg());
    std::string line;
    if (!std::getline(stream, line)) {
        is_eof_reached = true;
        // Откатываем последнюю запись offset-а - мы не прочитали строку
        MutableArraySequence<std::streampos> truncated;
        for (int index = 0; index < line_offsets.get_count() - 1; index++) {
            truncated.append(line_offsets.get(index));
        }
        line_offsets = truncated;
        throw EndOfStream();
    }
    this->position++;

    return deserializer(line);
}

template <class T>
size_t FileReadStream<T>::seek(size_t index) {
    if (!this->is_open) throw StreamNotOpen();

    int recorded_count = line_offsets.get_count();
    if (index <= static_cast<size_t>(recorded_count)) {
        // Прыжок в уже виденную область или ровно на следующую за последней прочитанной
        if (index < this->position) {
            // Назад: переоткрываем файл, проматываем до начала index-ой строки
            stream.clear();
            stream.close();
            stream.open(path, std::ios::in);
            if (!stream) throw StreamWriteError("FileReadStream: cannot reopen " + path);

            // Усекаем line_offsets до index записей
            MutableArraySequence<std::streampos> truncated;
            for (size_t step = 0; step < index; step++) {
                truncated.append(line_offsets.get(static_cast<int>(step)));
            }
            line_offsets = truncated;

            if (index == 0) {
                this->position = 0;
                is_eof_reached = false;
                return 0;
            }

            // Промотать stream до начала index-ой строки
            // line_offsets[index-1] - начало (index-1)-ой строки
            // Прочитать одну строку чтобы оказаться в начале index-ой
            stream.seekg(line_offsets.get(static_cast<int>(index - 1)));
            std::string dummy;
            if (!std::getline(stream, dummy)) {
                is_eof_reached = true;
            } else {
                is_eof_reached = false;
            }
            this->position = index;

            return index;
        }
        // index == position - ничего не делаем
        return this->position;
    }

    // index > recorded_count: вперёд через ещё-непрочитанные строки, отбрасываем
    while (this->position < index && !is_eof_reached) {
        try {
            (void)read();
        } catch (const EndOfStream&) {
            break;
        }
    }

    return this->position;
}

template <class T>
void FileReadStream<T>::open() {
    if (this->is_open) return;

    stream.open(path, std::ios::in);
    if (!stream) throw StreamWriteError("FileReadStream: cannot open " + path);
    this->is_open = true;
    this->position = 0;
    is_eof_reached = false;
    line_offsets = MutableArraySequence<std::streampos>();
}

template <class T>
void FileReadStream<T>::close() {
    if (!this->is_open) return;

    stream.close();
    this->is_open = false;
    this->position = 0;
    is_eof_reached = false;
    line_offsets = MutableArraySequence<std::streampos>();
}

#endif
