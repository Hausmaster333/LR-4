#ifndef CARDINAL_H
#define CARDINAL_H

#include <stdexcept>

class Cardinal { // Нужен для определения длины ленивой последовательности
    private:
        size_t omega_count;
        size_t finite_part;

        Cardinal(size_t omega_count, size_t finite_part) : omega_count(omega_count), finite_part(finite_part) {}
    public:
        static Cardinal finite(size_t length) { return Cardinal(0, length); } // Задаем длину
        static Cardinal infinity() { return Cardinal(1, 0); } // static для того, чтобы это была функция класса, которая создает объект этого класса
        // + чтобы не делать публичный метод со странным вызовом и запретить неправильные состояния
        static Cardinal omega_times(size_t k) { return Cardinal(k, 0); }
        static Cardinal zero() { return Cardinal(false, 0); } // обертка над finite(0)

        bool is_infinite() const { return omega_count > 0; }
        bool is_finite() const { return omega_count == 0; }

        size_t get_omega_count() const { return omega_count; }
        size_t get_finite_part() const { return finite_part; }

        size_t get_value() const {
            if (omega_count > 0) throw std::logic_error("Infinite cardinal has no finite value");

            return finite_part;
        }

        bool operator==(const Cardinal& other) const {
            return omega_count == other.omega_count && finite_part == other.finite_part;
        }

        bool operator!=(const Cardinal& other) const {
            return !(*this == other);
        }

        // w + n = w, n + w = w; w * a + n + w * b + m = w * (a + b) + m (finite_part левого поглощается омегой справа)
        Cardinal operator+(const Cardinal& other) const {
            if (other.omega_count > 0) {
                // что слева - не важно, всё уходит в омега-блок
                return Cardinal(omega_count + other.omega_count, other.finite_part);
            }
            // справа только конечная часть
            return Cardinal(omega_count, finite_part + other.finite_part);
        }

        Cardinal operator-(const Cardinal& other) const {
            if (omega_count == 0 && other.omega_count > 0)
                throw std::logic_error("finite - infinity would be negative");
            if (omega_count == other.omega_count) {
                if (finite_part < other.finite_part)
                    throw std::logic_error("Cardinal subtraction underflow");
                return Cardinal(0, finite_part - other.finite_part);
            }
            if (omega_count > other.omega_count) {
                // ω*a + n - ω*b + m = ω*(a-b) + n (грубо: разница в омега-блоках)
                return Cardinal(omega_count - other.omega_count, finite_part);
            }
            throw std::logic_error("Cardinal subtraction: omega underflow");
        }

        bool operator<(const Cardinal& other) const {
            if (omega_count != other.omega_count) return omega_count < other.omega_count;
            return finite_part < other.finite_part;
        }

        bool operator>(const Cardinal& other) const  { return other < *this; }
        bool operator<=(const Cardinal& other) const { return !(other < *this); }
        bool operator>=(const Cardinal& other) const { return !(*this < other); }
};

#endif
