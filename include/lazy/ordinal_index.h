#ifndef ORDINAL_INDEX_H
#define ORDINAL_INDEX_H

#include <cstddef>
#include <stdexcept>
#include "lazy/ordinal.h"

// Ординальный индекс вида omega_part * w + finite_part.
// {0, n} - обычное натуральное n.
// {1, k} - омега + k, т.е. k-й элемент во втором w блоке.
// Используется для concat(infinity, infinity), insert(..., inf) и chained-concat (см. P10).
struct OrdinalIndex {
    size_t omega_part;
    size_t finite_part;

    OrdinalIndex() : omega_part(0), finite_part(0) {}
    OrdinalIndex(size_t omega, size_t finite) : omega_part(omega), finite_part(finite) {}

    // Подъём натурального индекса в ординальный
    static OrdinalIndex finite(size_t n) { return OrdinalIndex(0, n); }
    static OrdinalIndex omega_plus(size_t k) { return OrdinalIndex(1, k); }

    bool is_finite_only() const { return omega_part == 0; }

    bool operator==(const OrdinalIndex& other) const {
        return omega_part == other.omega_part && finite_part == other.finite_part;
    }
    bool operator!=(const OrdinalIndex& other) const { return !(*this == other); }

    bool operator<(const OrdinalIndex& other) const {
        if (omega_part != other.omega_part) return omega_part < other.omega_part;
        return finite_part < other.finite_part;
    }
    bool operator<=(const OrdinalIndex& other) const { return *this < other || *this == other; }
    bool operator>(const OrdinalIndex& other) const  { return other < *this; }
    bool operator>=(const OrdinalIndex& other) const { return !(*this < other); }

    OrdinalIndex operator+(size_t n) const {
        return OrdinalIndex(omega_part, finite_part + n);
    }
};

// Сравнение ординального индекса с ординалом (длиной).
// idx < length означает, что idx — допустимая позиция в последовательности длины length.
inline bool operator<(const OrdinalIndex& idx, const Ordinal& length) {
    if (idx.omega_part != length.get_omega_count()) return idx.omega_part < length.get_omega_count();
    return idx.finite_part < length.get_finite_part();
}
inline bool operator>=(const OrdinalIndex& idx, const Ordinal& length) { return !(idx < length); }

// Ординальное вычитание индекса и ординала: β такое, что length + β = idx.
// Используется в ConcatGenerator::get_at для перехода в правую часть: right_idx = idx - left_length.
// Precondition: idx >= length.
inline OrdinalIndex operator-(const OrdinalIndex& idx, const Ordinal& length) {
    size_t a = length.get_omega_count();
    size_t m = length.get_finite_part();

    if (idx.omega_part > a) {
        return OrdinalIndex(idx.omega_part - a, idx.finite_part);
    }
    if (idx.omega_part == a) {
        if (idx.finite_part < m)
            throw std::logic_error("OrdinalIndex - Ordinal: underflow in finite part");
        return OrdinalIndex(0, idx.finite_part - m);
    }
    throw std::logic_error("OrdinalIndex - Ordinal: omega underflow");
}

#endif
