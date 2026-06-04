#ifndef LZW_COMMON_H
#define LZW_COMMON_H

#include "core/sequence.h"
#include <cstdint>

// Общие константы архиватора .Z формата
static const uint8_t LZW_MAGIC1 = 0x1F;
static const uint8_t LZW_MAGIC2 = 0x9D;
static const uint8_t LZW_BLOCK_MODE = 0x80;
static const int LZW_INIT_BITS = 9;                    // Начальная ширина кода
static const int LZW_MAX_BITS = 16;                    // Максимальная ширина кода
static const int LZW_CLEAR = 256;
static const int LZW_FIRST = 257;                      // Первый код данных (256 занят под CLEAR)
static const long LZW_LIMITCODE = 1L << LZW_MAX_BITS; // словарь до 2^16 кодов

inline long lzw_maxcode(int n_bits) { return (1L << n_bits) - 1; } // макс. код для ширины n_bits бит

struct LzwChildEdge {
    uint8_t byte;
    uint16_t code;
};

class LzwEncodeDict {
    private:
        MutableArraySequence<MutableArraySequence<LzwChildEdge>*> children;
        uint16_t next_code;
    public:
        LzwEncodeDict(int first_code = 256) : next_code(static_cast<uint16_t>(first_code)) {
            for (int code = 0; code < first_code; code++) {
                children.append(new MutableArraySequence<LzwChildEdge>());
            }
        }

        int get_next_code() const { return next_code; }

        // Найти код (parent, byte)
        int find_child(uint16_t parent, uint8_t byte) const {
            auto edges = children.get(parent);

            for (int edge_index = 0; edge_index < edges->get_count(); edge_index++) {
                const LzwChildEdge& edge = edges->get(edge_index);

                if (edge.byte == byte) {
                    return edge.code;
                }
            }

            return -1;
        }

        // Добавить ребро (parent, byte) с новым кодом
        void add_child(uint16_t parent, uint8_t byte) {
            uint16_t new_code = next_code++;
            LzwChildEdge edge = {byte, new_code};
            children.get(parent)->append(edge);
            children.append(new MutableArraySequence<LzwChildEdge>()); // список детей нового кода
        }

        ~LzwEncodeDict() {
            for (int index = 0; index < children.get_count(); index++) {
                delete children.get(index);
            }
        }
};

#endif
