#ifndef LZW_INPUT_STREAM_H
#define LZW_INPUT_STREAM_H

#include "streams/read_only_stream.h"
#include "streams/stream_exceptions.h"
#include "compression/lzw_common.h"
#include <cstdint>
#include <cstring>
#include <stdexcept>

// Потоковый декодер .Z формата, выдаёт данные по байту
class LzwInputStream : public ReadOnlyStream<uint8_t> {
    private:
        ReadOnlyStream<uint8_t>* backing;
        int maxbits;
        bool block_mode;
        long maxmaxcode;

        uint16_t* tab_prefix; // Код записи словаря
        uint8_t* tab_suffix;  // Последний байт записи словаря
        uint8_t* stack;       // Байты текущего кода
        int stack_top;

        int n_bits;
        long maxcode;
        int free_ent;
        int oldcode;
        uint8_t finchar;
        int clear_flg;
        bool first_done;

        uint8_t buf[LZW_MAX_BITS + 4];
        int offset; // Позиция чтения
        int size;   // Валидных бит в буфере

        // Прочитать код из backing, управляя ростом ширины
        int get_code() {
            if (clear_flg || offset >= size || free_ent > maxcode) {
                if (free_ent > maxcode) {
                    n_bits++;
                    maxcode = (n_bits == maxbits) ? maxmaxcode : lzw_maxcode(n_bits);
                }

                if (clear_flg) {
                    n_bits = LZW_INIT_BITS;
                    maxcode = lzw_maxcode(LZW_INIT_BITS);
                    clear_flg = 0;
                }

                std::memset(buf, 0, sizeof(buf));

                int got = 0;
                while (got < n_bits && !backing->is_end_of_stream()) {
                    buf[got++] = backing->read();
                }

                if (got == 0) return -1;

                offset = 0;
                size = (got << 3) - (n_bits - 1);
            }

            int byte_index = offset >> 3;
            int bit = offset & 7;
            long code = static_cast<long>(buf[byte_index]) | (static_cast<long>(buf[byte_index + 1]) << 8) | (static_cast<long>(buf[byte_index + 2]) << 16);

            code = (code >> bit) & lzw_maxcode(n_bits);
            offset += n_bits;
            return static_cast<int>(code);
        }

        // Декодировать один код в стек
        bool decode_next() {
            if (!first_done) {
                int first_code = get_code();
                if (first_code == -1) return false;
                first_done = true;
                finchar = static_cast<uint8_t>(first_code);
                oldcode = first_code;
                stack[stack_top++] = finchar;
                return true;
            }

            int code = get_code();
            if (code == -1) return false;

            if (code == LZW_CLEAR && block_mode) {
                clear_flg = 1;
                free_ent = LZW_FIRST - 1;
                code = get_code();

                if (code == -1) return false;
            }

            int incode = code;
            if (code >= free_ent) {
                stack[stack_top++] = finchar;
                code = oldcode;
            }
            while (code >= 256) {
                stack[stack_top++] = tab_suffix[code];
                code = tab_prefix[code];
            }
            finchar = static_cast<uint8_t>(code);
            stack[stack_top++] = finchar;

            if (free_ent < maxmaxcode) {
                tab_prefix[free_ent] = static_cast<uint16_t>(oldcode);
                tab_suffix[free_ent] = finchar;
                free_ent++;
            }
            oldcode = incode;
            return true;
        }
    public:
        LzwInputStream(ReadOnlyStream<uint8_t>* backing) : backing(backing), maxbits(LZW_MAX_BITS), block_mode(true), maxmaxcode(LZW_MAXMAXCODE),
              tab_prefix(nullptr), tab_suffix(nullptr), stack(nullptr), stack_top(0), n_bits(LZW_INIT_BITS), maxcode(lzw_maxcode(LZW_INIT_BITS)), 
              free_ent(LZW_FIRST), oldcode(-1), finchar(0), clear_flg(0), first_done(false), offset(0), size(0) {
            if (backing == nullptr) throw std::invalid_argument("LzwInputStream: backing is nullptr");

            std::memset(buf, 0, sizeof(buf));
        }

        void open() override {
            if (this->is_open) return;
            if (!backing->opened()) backing->open();

            uint8_t magic1 = backing->read();
            uint8_t magic2 = backing->read();
            uint8_t flags = backing->read();
            if (magic1 != LZW_MAGIC1 || magic2 != LZW_MAGIC2) throw std::runtime_error("Not a .Z file");

            maxbits = flags & 0x1F;
            block_mode = (flags & LZW_BLOCK_MODE) != 0;
            maxmaxcode = 1L << maxbits;

            tab_prefix = new uint16_t[maxmaxcode];
            tab_suffix = new uint8_t[maxmaxcode];
            stack = new uint8_t[maxmaxcode + 1];
            for (int code = 0; code < 256; code++) {
                tab_prefix[code] = 0;
                tab_suffix[code] = static_cast<uint8_t>(code);
            }

            n_bits = LZW_INIT_BITS;
            maxcode = lzw_maxcode(LZW_INIT_BITS);
            free_ent = block_mode ? LZW_FIRST : 256;
            oldcode = -1;
            finchar = 0;
            clear_flg = 0;
            first_done = false;
            offset = 0;
            size = 0;
            stack_top = 0;
            std::memset(buf, 0, sizeof(buf));

            this->is_open = true;
            this->position = 0;
        }

        bool is_end_of_stream() const override {
            if (!this->is_open) return false;
            if (stack_top == 0) {
                const_cast<LzwInputStream*>(this)->decode_next();
            }

            return stack_top == 0;
        }

        // Выдать один разжатый байт
        uint8_t read() override {
            if (!this->is_open) throw StreamNotOpen();
            if (stack_top == 0) {
                if (!decode_next()) throw EndOfStream();
            }

            uint8_t result = stack[--stack_top];
            this->position++;

            return result;
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

        void close() override {
            if (!this->is_open) return;

            delete[] tab_prefix;
            tab_prefix = nullptr;

            delete[] tab_suffix;
            tab_suffix = nullptr;

            delete[] stack;
            stack = nullptr;

            stack_top = 0;
            this->is_open = false;
            this->position = 0;
        }

        ~LzwInputStream() override {
            delete[] tab_prefix;
            delete[] tab_suffix;
            delete[] stack;
        }
};

#endif
