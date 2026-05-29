#ifndef LZW_STREAM_H
#define LZW_STREAM_H

#include "streams/write_only_stream.h"
#include "streams/read_only_stream.h"
#include "compression/lzw.h"
#include <cstdint>
#include <string>

// Истинный потоковый энкодер LZW. На каждый write эмитит код только когда
// расширение текущей строки невозможно. Не хранит весь вход - между вызовами
// держит только current_code и trie-словарь.
class LzwOutputStream : public WriteOnlyStream<uint8_t> {
    private:
        WriteOnlyStream<uint16_t>* backing;
        LzwEncodeDict* dictionary;
        uint16_t current_code;
        bool has_current;
    public:
        LzwOutputStream(WriteOnlyStream<uint16_t>* backing)
            : backing(backing), dictionary(nullptr), current_code(0), has_current(false) {
            if (backing == nullptr) throw std::invalid_argument("Backing is nullptr");
        }

        void open() override {
            if (this->is_open) return;

            dictionary = new LzwEncodeDict();
            if (!backing->opened()) backing->open();
            current_code = 0;
            has_current = false;
            this->is_open = true;
            this->position = 0;
        }

        size_t write(const uint8_t& value) override {
            if (!this->is_open) throw StreamNotOpen();

            if (!has_current) {
                // Первый байт - это просто его код-байт
                current_code = value;
                has_current = true;
            } else {
                int child = dictionary->find_child(current_code, value);

                if (child >= 0) {
                    // Расширение есть - переходим к ребёнку, кода ещё не эмитим
                    current_code = static_cast<uint16_t>(child);
                } else {
                    // Расширения нет - эмитим текущий код и стартуем новую подстроку
                    backing->write(current_code);
                    if (dictionary->can_add()) {
                        dictionary->add_child(current_code, value);
                    }
                    current_code = value;
                }
            }

            this->position++;
            return this->position;
        }

        void close() override {
            if (!this->is_open) return;

            try {
                if (has_current) backing->write(current_code);
                backing->close();
            } catch (...) {
                delete dictionary;
                dictionary = nullptr;
                this->is_open = false;
                this->position = 0;
                throw;
            }

            delete dictionary;
            dictionary = nullptr;
            this->is_open = false;
            this->position = 0;
        }

        ~LzwOutputStream() override {
            if (this->is_open) {
                try { close(); } catch (...) {}
            }
            delete dictionary;
        }
};

// Истинный потоковый декодер LZW. На каждый read отдаёт один байт текущего
// разжатого chunk-а. Когда chunk кончился - читает следующий код из backing
// и декодирует его (включая KwKwK особый случай).
class LzwInputStream : public ReadOnlyStream<uint8_t> {
    private:
        ReadOnlyStream<uint16_t>* backing;
        MutableArraySequence<std::string>* dictionary;
        std::string previous;
        std::string current_chunk;
        size_t chunk_pos;
        bool first_code_read;
 
        // Декодирует следующий код из backing в current_chunk, обновляет словарь.
        // Возвращает false если backing исчерпан
        bool decode_next_chunk() {
            if (backing->is_end_of_stream()) return false;

            uint16_t code = backing->read();
            std::string entry;

            if (!first_code_read) {
                if (code >= static_cast<uint16_t>(dictionary->get_count())) {
                    throw std::runtime_error("LzwInputStream: invalid first code");
                }
                entry = dictionary->get(code);
                first_code_read = true;
            } else {
                if (code < static_cast<uint16_t>(dictionary->get_count())) {
                    entry = dictionary->get(code);
                } else if (code == static_cast<uint16_t>(dictionary->get_count())) {
                    // KwKwK особый случай
                    entry = previous + previous[0];
                } else {
                    throw std::runtime_error("LzwInputStream: invalid code");
                }

                if (static_cast<uint16_t>(dictionary->get_count()) < LZW_MAX_DICT_SIZE) {
                    dictionary->append(previous + std::string(1, entry[0]));
                }
            }

            previous = entry;
            current_chunk = entry;
            chunk_pos = 0;
            return true;
        }

    public:
        LzwInputStream(ReadOnlyStream<uint16_t>* backing)
            : backing(backing), dictionary(nullptr), chunk_pos(0), first_code_read(false) {
            if (backing == nullptr) throw std::invalid_argument("LzwInputStream: backing is nullptr");
        }

        void open() override {
            if (this->is_open) return;

            if (!backing->opened()) backing->open();

            dictionary = new MutableArraySequence<std::string>();
            for (int i = 0; i < 256; i++) {
                dictionary->append(std::string(1, static_cast<char>(i)));
            }
            previous.clear();
            current_chunk.clear();
            chunk_pos = 0;
            first_code_read = false;

            this->is_open = true;
            this->position = 0;
        }

        bool is_end_of_stream() const override {
            if (!this->is_open) return false;
            return chunk_pos >= current_chunk.size() && backing->is_end_of_stream();
        }

        uint8_t read() override {
            if (!this->is_open) throw StreamNotOpen();

            if (chunk_pos >= current_chunk.size()) {
                if (!decode_next_chunk()) throw EndOfStream();
            }

            uint8_t result = static_cast<uint8_t>(current_chunk[chunk_pos++]);
            this->position++;
            return result;
        }

        // Forward-only seek через read-and-discard. Назад нельзя - декодер stateful
        bool is_can_seek() const override { return true; }
        bool is_can_go_back() const override { return false; }

        size_t seek(size_t index) override {
            if (!this->is_open) throw StreamNotOpen();
            if (index < this->position) throw GoBackUnsupported();
            while (this->position < index && !is_end_of_stream()) read();
            return this->position;
        }

        void close() override {
            if (!this->is_open) return;

            delete dictionary;
            dictionary = nullptr;
            previous.clear();
            current_chunk.clear();
            chunk_pos = 0;
            first_code_read = false;
            this->is_open = false;
            this->position = 0;
        }

        ~LzwInputStream() override {
            if (this->is_open) close();
            delete dictionary;
        }
};

#endif
