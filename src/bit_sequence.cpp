#include "types/bit_sequence.h"
#include <stdexcept>

int BitSequence::check_bytes_needed(int n) {
    if (n <= 0) return 0;
    return (n + 7) / 8;
}

bool BitSequence::get_bit(int index) const {
    unsigned char byte = data.get(index / 8); // Определяем, в каком байте лежит нужный бит
    // В битах нумерование справа налево
    return (byte >> (index % 8)) & 1; // Определяем позицию внутри этого байта (0–7) через index % 8 и сдвигаем бит вправо, чтобы бит оказался на позиции 0, после чего выполняем побитовое И с 1
}

void BitSequence::set_bit(int index, bool value) {
    int byte_index = index / 8;
    int bit_index = index % 8;
    unsigned char byte = data.get(byte_index);

    if (value) { // Если 1
        byte |= (1 << bit_index); // Сдвигом получаем маску и используем ИЛИ, чтобы поставить 1 в нужный бит
    } else { // Если 0
        byte &= ~(1 << bit_index); // Сдвигаем + инвертируем и используем И, чтобы оставить 1 только там, где обе 1, а в остальных будет 0
    }

    data.set(byte_index, byte);
}

void BitSequence::clear_unused_bits() {
    int used_bits = bit_count % 8;

    if (used_bits == 0 || bit_count == 0) return;

    int last_byte_idx = check_bytes_needed(bit_count) - 1;
    unsigned char mask = (1 << used_bits) - 1;
    data.set(last_byte_idx, data.get(last_byte_idx) & mask);
}

void BitSequence::sys_append(const Bit& item) {
    int new_count = bit_count + 1;
    int new_bytes = check_bytes_needed(new_count);

    if (new_bytes > data.get_size()) {
        data.resize(new_bytes);
        data.set(new_bytes - 1, 0);
    }

    bit_count = new_count;
    set_bit(bit_count - 1, item.get());
}

Sequence<Bit>* BitSequence::CreateEmpty() const {
    return new BitSequence();
}

BitSequence::BitSequence() : bit_count(0), cached_bit(false) {
    data = DynamicArray<unsigned char>(0);
}

BitSequence::BitSequence(const Bit* items, int count) : bit_count(count), cached_bit(false) {
    if (count < 0) throw std::invalid_argument("Count cannot be negative");

    int bytes = check_bytes_needed(count);
    data = DynamicArray<unsigned char>(bytes);

    for (int idx = 0; idx < bytes; idx++) { // Зануляем все байты
        data.set(idx, 0);
    }

    for (int idx = 0; idx < count; idx++) { // Заполняем байты битами
        set_bit(idx, items[idx].get());
    }
}

BitSequence::BitSequence(const BitSequence& other) : bit_count(other.bit_count), cached_bit(false) {
    data = DynamicArray<unsigned char>(other.data);
}

const Bit& BitSequence::get_first() const {
    if (bit_count == 0) throw std::out_of_range("Sequence is empty");

    cached_bit = Bit(get_bit(0));

    return cached_bit;
}

const Bit& BitSequence::get_last() const {
    if (bit_count == 0) throw std::out_of_range("Sequence is empty");

    cached_bit = Bit(get_bit(bit_count - 1));

    return cached_bit;
}

const Bit& BitSequence::get(int index) const {
    if (index < 0 || index >= bit_count) throw std::out_of_range("Index out of range");

    cached_bit = Bit(get_bit(index));

    return cached_bit;
}

Option<Bit> BitSequence::try_get_first() const {
    if (bit_count == 0) return Option<Bit>();

    return Option<Bit>(Bit(get_bit(0)));
}

Option<Bit> BitSequence::try_get_last() const {
    if (bit_count == 0) return Option<Bit>();

    return Option<Bit>(Bit(get_bit(bit_count - 1)));
}

Option<Bit> BitSequence::try_get(int index) const {
    if (index < 0 || index >= bit_count) return Option<Bit>();

    return Option<Bit>(Bit(get_bit(index)));
}

int BitSequence::get_count() const {
    return bit_count;
}

Sequence<Bit>* BitSequence::append(const Bit& item) {
    int new_count = bit_count + 1;
    int new_bytes = check_bytes_needed(new_count);

    if (new_bytes > data.get_size()) {
        data.resize(new_bytes);
        data.set(new_bytes - 1, 0);
    }

    bit_count = new_count;
    set_bit(bit_count - 1, item.get());

    return this;
}

Sequence<Bit>* BitSequence::prepend(const Bit& item) {
    DynamicArray<unsigned char>* old_data = new DynamicArray<unsigned char>(data);
    int old_count = bit_count;

    int new_count = old_count + 1;
    int new_bytes = check_bytes_needed(new_count);
    data.resize(new_bytes);

    for (int idx = 0; idx < new_bytes; idx++) {
        data.set(idx, 0);
    }

    bit_count = new_count;

    set_bit(0, item.get());
    for (int idx = 0; idx < old_count; idx++) {
        unsigned char old_byte = old_data->get(idx / 8);
        bool val = (old_byte >> (idx % 8)) & 1;
        set_bit(idx + 1, val);
    }
    delete old_data;

    return this;
}

Sequence<Bit>* BitSequence::insert_at(const Bit& item, int index) {
    if (index < 0 || index > bit_count) throw std::out_of_range("Index out of range");

    DynamicArray<unsigned char> old_data = DynamicArray<unsigned char>(data);
    int old_count = bit_count;
    
    int new_count = old_count + 1;
    int new_bytes = check_bytes_needed(new_count);
    data.resize(new_bytes);

    for (int idx = 0; idx < new_bytes; idx++) {
        data.set(idx, 0);
    }

    bit_count = new_count;

    for (int idx = 0; idx < index; idx++) {
        unsigned char old_byte = old_data.get(idx / 8);
        bool val = (old_byte >> (idx % 8)) & 1;
        set_bit(idx, val);
    }

    set_bit(index, item.get());

    for (int idx = index; idx < old_count; idx++) {
        unsigned char old_byte = old_data.get(idx / 8);
        bool val = (old_byte >> (idx % 8)) & 1;
        set_bit(idx + 1, val);
    }

    return this;
}

BitSequence* BitSequence::bit_and(const BitSequence& other) const {
    int min_count = (bit_count < other.bit_count) ? bit_count : other.bit_count;
    int min_bytes = check_bytes_needed(min_count);

    BitSequence* and_bit_seq = new BitSequence();
    and_bit_seq->data.resize(min_bytes);
    and_bit_seq->bit_count = min_count;

    for (int idx = 0; idx < min_bytes; idx++) {
        and_bit_seq->data.set(idx, data.get(idx) & other.data.get(idx));
    }

    and_bit_seq->clear_unused_bits();
    return and_bit_seq;
}

BitSequence* BitSequence::bit_or(const BitSequence& other) const {
    int max_count = (bit_count > other.bit_count) ? bit_count : other.bit_count;
    int max_bytes = check_bytes_needed(max_count);

    BitSequence* or_bit_seq = new BitSequence();
    or_bit_seq->data.resize(max_bytes);
    or_bit_seq->bit_count = max_count;

    for (int idx = 0; idx < max_bytes; idx++) {
        unsigned char a = (idx < check_bytes_needed(bit_count)) ? data.get(idx) : 0;
        unsigned char b = (idx < check_bytes_needed(other.bit_count)) ? other.data.get(idx) : 0;

        or_bit_seq->data.set(idx, a | b);
    }

    or_bit_seq->clear_unused_bits();
    return or_bit_seq;
}

BitSequence* BitSequence::bit_xor(const BitSequence& other) const {
    int max_count = (bit_count > other.bit_count) ? bit_count : other.bit_count;
    int max_bytes = check_bytes_needed(max_count);

    BitSequence* xor_bit_seq = new BitSequence();
    xor_bit_seq->data.resize(max_bytes);
    xor_bit_seq->bit_count = max_count;

    for (int idx = 0; idx < max_bytes; idx++) {
        unsigned char a = (idx < check_bytes_needed(bit_count)) ? data.get(idx) : 0;
        unsigned char b = (idx < check_bytes_needed(other.bit_count)) ? other.data.get(idx) : 0;

        xor_bit_seq->data.set(idx, a ^ b);
    }

    xor_bit_seq->clear_unused_bits();
    return xor_bit_seq;
}

BitSequence* BitSequence::bit_not() const {
    int bytes = check_bytes_needed(bit_count);

    BitSequence* not_bit_seq = new BitSequence();
    not_bit_seq->data.resize(bytes);
    not_bit_seq->bit_count = bit_count;

    for (int idx = 0; idx < bytes; idx++) {
        not_bit_seq->data.set(idx, ~data.get(idx));
    }

    not_bit_seq->clear_unused_bits();
    return not_bit_seq;
}