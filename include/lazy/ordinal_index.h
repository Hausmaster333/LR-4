#ifndef ORDINAL_INDEX_H
#define ORDINAL_INDEX_H

#include <cstddef>
#include <stdexcept>

// Ординальный индекс вида omega_part * w + finite_part.
// {0, n} - обычное натуральное n.
// {1, k} - омега + k, т.е. k-й элемент во втором w блоке.
// Используется для concat(infinity, infinity) и т.п.
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

    // Сдвиг на натуральное число
    OrdinalIndex operator+(size_t n) const {
        return OrdinalIndex(omega_part, finite_part + n);
    }
};

#endif
