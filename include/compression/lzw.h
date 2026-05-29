#ifndef LZW_H
#define LZW_H

#include "core/sequence.h"
#include <string>
#include <cstdint>
#include <stdexcept>

static const uint16_t LZW_MAX_DICT_SIZE = 65535;

struct LzwChildEdge {
    uint8_t byte;
    uint16_t code;
};

class LzwEncodeDict {
    private:
        MutableArraySequence<MutableArraySequence<LzwChildEdge>*> children;
        uint16_t next_code;
    public:
        LzwEncodeDict() : next_code(256) {
            // Для каждого одиночного байта 0..255 создаём пустой список детей
            for (int i = 0; i < 256; i++) {
                children.append(new MutableArraySequence<LzwChildEdge>());
            }
        }

        int find_child(uint16_t parent, uint8_t byte) const { // Ищем ребенка-пару (родитель-байт)
            auto edges = children.get(parent); // Получаем все ребра родителя

            for (int edge_idx = 0; edge_idx < edges->get_count(); edge_idx++) {
                const LzwChildEdge& edge = edges->get(edge_idx);
                if (edge.byte == byte) return edge.code;
            }
            return -1; // Не нашли
        }

        void add_child(uint16_t parent, uint8_t byte) {
            uint16_t new_code = next_code++;

            LzwChildEdge edge = {byte, new_code};
            children.get(parent)->append(edge); // Добавляем ребро в список детей родителей

            children.append(new MutableArraySequence<LzwChildEdge>()); // Новый список детей для нового кода
        }

        bool can_add() const { return next_code < LZW_MAX_DICT_SIZE; }\

        ~LzwEncodeDict() {
            for (int i = 0; i < children.get_count(); i++) {
                delete children.get(i);
            }
        }
};

inline MutableArraySequence<uint16_t>* lzw_compress(const Sequence<uint8_t>* input) {
    if (input == nullptr) throw std::invalid_argument("Input is nullptr");

    MutableArraySequence<uint16_t>* output = new MutableArraySequence<uint16_t>();

    int count = input->get_count();
    if (count == 0) return output;

    LzwEncodeDict dict;

    EnumeratorWrapper<uint8_t> iter(input->get_enumerator());
    iter.move_next();
    uint16_t curr_code = iter.get_current(); // Текущий код

    while (iter.move_next()) {
        uint8_t next_byte = iter.get_current(); // Следующий код
        int child = dict.find_child(curr_code, next_byte); // Ищем такую пару

        if (child >= 0) {
            curr_code = static_cast<uint16_t>(child); // Просто приводим и идем дальше
        } else {
            output->append(curr_code);

            if (dict.can_add()) {
                dict.add_child(curr_code, next_byte); // Добавляем в словарь новую пару
            }
            curr_code = next_byte;
        }
    }
    output->append(curr_code);

    return output;
}

inline MutableArraySequence<uint8_t>* lzw_decompress(const Sequence<uint16_t>* compressed) {
    if (compressed == nullptr) throw std::invalid_argument("Input is nullptr");

    auto* output = new MutableArraySequence<uint8_t>();
    int count = compressed->get_count();
    if (count == 0) return output;

    MutableArraySequence<std::string> dictionary;
    for (int idx = 0; idx < 256; idx++) {
        dictionary.append(std::string(1, static_cast<char>(idx)));
    }

    EnumeratorWrapper<uint16_t> iter(compressed->get_enumerator());
    iter.move_next();

    uint16_t code = iter.get_current(); // Первый код
    if (code >= static_cast<uint16_t>(dictionary.get_count())) {
        throw std::runtime_error("Invalid first code");
    }

    std::string previous = dictionary.get(code); // Строка первого кода
    for (size_t idx = 0; idx < previous.size(); idx++) {
        output->append(static_cast<uint8_t>(previous[idx]));
    }

    while (iter.move_next()) {
        code = iter.get_current(); // Текущий код
        std::string entry;

        if (code < static_cast<uint16_t>(dictionary.get_count())) { // Уже в словаре
            entry = dictionary.get(code);
        } else if (code == static_cast<uint16_t>(dictionary.get_count())) { // Код должен быть добавлен ниже в словарь, но восстановить надо уже сейчас (пример XYXYXY)
            entry = previous + previous[0];
        } else {
            throw std::runtime_error("Invalid code");
        }

        for (size_t i = 0; i < entry.size(); i++) {
            output->append(static_cast<uint8_t>(entry[i])); // Добавляем по 1 байту
        }

        if (static_cast<uint16_t>(dictionary.get_count()) < LZW_MAX_DICT_SIZE) {
            dictionary.append(previous + std::string(1, entry[0])); // Добавляем новую запись в виде прошлой подстроки + первого байта новой
        }

        previous = entry;
    }

    return output;
}

#endif
