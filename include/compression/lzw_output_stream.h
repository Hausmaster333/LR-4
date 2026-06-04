#ifndef LZW_OUTPUT_STREAM_H
#define LZW_OUTPUT_STREAM_H

#include "streams/write_only_stream.h"
#include "streams/stream_exceptions.h"
#include "compression/lzw_common.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>

class LzwOutputStream : public WriteOnlyStream<uint8_t> {
    private:
        WriteOnlyStream<uint8_t>* backing;
        LzwEncodeDict* dictionary;

        int ent;        // Текущий код
        int n_bits;     // Текущая ширина кода
        long maxcode;   // Максимальный код для текущей ширины
        int clear_flg;  // Если 1, то на следующем шаге сбрасываем ширину на 9 бит
        uint8_t buf[LZW_MAX_BITS + 4];
        int offset;     // позиция в буфере

        void flush_block(int byte_count) {
            for (int byte_index = 0; byte_index < byte_count; byte_index++) {
                backing->write(buf[byte_index]);
            }
            offset = 0;

            std::memset(buf, 0, sizeof(buf));
        }

        // Упаковать код битами
        void output_code(int code) {
            int byte_index = offset >> 3;
            int bit = offset & 7;
            long value = static_cast<long>(code) << bit;

            buf[byte_index] |= static_cast<uint8_t>(value & 0xFF);
            buf[byte_index + 1] |= static_cast<uint8_t>((value >> 8) & 0xFF);
            buf[byte_index + 2] |= static_cast<uint8_t>((value >> 16) & 0xFF);

            offset += n_bits;

            if (offset == (n_bits << 3)) flush_block(n_bits);

            // Если следующий код не влезет в текущую ширину - увеличиваем её
            if (dictionary->get_next_code() > maxcode || clear_flg) {
                if (offset > 0) flush_block(n_bits);

                if (clear_flg) {
                    n_bits = LZW_INIT_BITS;
                    maxcode = lzw_maxcode(LZW_INIT_BITS);
                    clear_flg = 0;
                } else {
                    n_bits++;
                    maxcode = (n_bits == LZW_MAX_BITS) ? LZW_LIMITCODE : lzw_maxcode(n_bits);
                }
            }
        }
    public:
        LzwOutputStream(WriteOnlyStream<uint8_t>* backing) : backing(backing), dictionary(nullptr), ent(-1), n_bits(LZW_INIT_BITS), 
                                                             maxcode(lzw_maxcode(LZW_INIT_BITS)), clear_flg(0), offset(0) {
            if (backing == nullptr) throw std::invalid_argument("Backing is nullptr");

            std::memset(buf, 0, sizeof(buf));
        }

        void open() override {
            if (this->is_open) return;
            if (!backing->opened()) backing->open();

            backing->write(LZW_MAGIC1);
            backing->write(LZW_MAGIC2);
            backing->write(static_cast<uint8_t>(LZW_MAX_BITS | LZW_BLOCK_MODE));

            dictionary = new LzwEncodeDict(LZW_FIRST);
            ent = -1;
            n_bits = LZW_INIT_BITS;
            maxcode = lzw_maxcode(LZW_INIT_BITS);
            clear_flg = 0;
            offset = 0;
            std::memset(buf, 0, sizeof(buf));

            this->is_open = true;
            this->position = 0;
        }

        size_t write(const uint8_t& value) override {
            if (!this->is_open) throw StreamNotOpen();

            if (ent < 0) {
                ent = value;
            } else {
                int child = dictionary->find_child(static_cast<uint16_t>(ent), value);
                if (child >= 0) {
                    ent = child; // Идем дальше, если можно расширить
                } else {
                    output_code(ent);
                    if (dictionary->get_next_code() < LZW_LIMITCODE) {
                        dictionary->add_child(static_cast<uint16_t>(ent), value);
                    } else {
                        // Сбрасываем полный словарь
                        delete dictionary;
                        dictionary = new LzwEncodeDict(LZW_FIRST);
                        clear_flg = 1;
                        output_code(LZW_CLEAR);
                    }
                    ent = value;
                }
            }

            this->position++;

            return this->position;
        }

        void close() override {
            if (!this->is_open) return;

            if (ent >= 0) output_code(ent);

            if (offset > 0) {
                int tail = (offset + 7) / 8;
                for (int byte_index = 0; byte_index < tail; byte_index++) {
                    backing->write(buf[byte_index]);
                }

                offset = 0;
            }

            backing->close();
            delete dictionary;
            dictionary = nullptr;
            this->is_open = false;
            this->position = 0;
        }

        ~LzwOutputStream() override {
            if (this->is_open) {
                try {
                    close();
                } catch (...) {}
            }
            delete dictionary;
        }
};

#endif
