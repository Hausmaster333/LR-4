#ifndef ORDINAL_H
#define ORDINAL_H

#include <stdexcept>

class Ordinal {
    private:
        size_t omega_count;
        size_t finite_part;

    public:
        Ordinal(size_t omega_count, size_t finite_part) : omega_count(omega_count), finite_part(finite_part) {}
        Ordinal() : omega_count(0), finite_part(0) {}

        static Ordinal finite(size_t length) { return Ordinal(0, length); }
        static Ordinal infinity() { return Ordinal(1, 0); }
        static Ordinal omega_times(size_t k) { return Ordinal(k, 0); }
        static Ordinal zero() { return Ordinal(0, 0); }

        bool is_infinite() const { return omega_count > 0; }
        bool is_finite() const { return omega_count == 0; }

        size_t get_omega_count() const { return omega_count; }
        size_t get_finite_part() const { return finite_part; }

        size_t get_value() const {
            if (omega_count > 0) throw std::logic_error("Infinite ordinal has no finite value");

            return finite_part;
        }

        bool operator==(const Ordinal& other) const {
            return omega_count == other.omega_count && finite_part == other.finite_part;
        }

        bool operator!=(const Ordinal& other) const {
            return !(*this == other);
        }

        Ordinal operator+(const Ordinal& other) const {
            if (other.omega_count > 0) {
                return Ordinal(omega_count + other.omega_count, other.finite_part);
            }

            return Ordinal(omega_count, finite_part + other.finite_part);
        }

        // Определено, когда this >= other
        Ordinal operator-(const Ordinal& other) const { 
            if (omega_count == 0 && other.omega_count > 0) throw std::logic_error("finite - infinity would be negative");

            if (omega_count == other.omega_count) {
                if (finite_part < other.finite_part) throw std::logic_error("Ordinal subtraction underflow");

                return Ordinal(0, finite_part - other.finite_part);
            }

            if (omega_count > other.omega_count) {

                return Ordinal(omega_count - other.omega_count, finite_part);
            }

            throw std::logic_error("Omega underflow");
        }

        bool operator<(const Ordinal& other) const {
            if (omega_count != other.omega_count) return omega_count < other.omega_count;

            return finite_part < other.finite_part;
        }

        bool operator>(const Ordinal& other) const  { return other < *this; }
        bool operator<=(const Ordinal& other) const { return !(other < *this); }
        bool operator>=(const Ordinal& other) const { return !(*this < other); }
};

#endif
