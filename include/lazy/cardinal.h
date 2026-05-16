#ifndef CARDINAL_H
#define CARDINAL_H

#include <stdexcept>

class Cardinal { // Нужен для определения длины ленивой последовательности
    private:
        bool infinite; // Определяет, бесконечная ли длина
        size_t finite_length; // Кардинальное число(количество элементов в множестве), в нашем случае это длина

        Cardinal(bool infinite, size_t finite_length) : infinite(infinite), finite_length(finite_length) {}
    public:
        static Cardinal finite(size_t finite_length) { return Cardinal(false, finite_length); } // Задаем длину
        static Cardinal infinity() { return Cardinal(true, 0); } // static для того, чтобы это была функция класса, которая создает объект этого класса
        // + чтобы не делать публичный метод со странным вызовом и запретить неправильные состояния
        static Cardinal zero() { return Cardinal(false, 0); } // обертка над finite(0)

        bool is_infinite() const { return infinite; }
        bool is_finite() const { return !infinite; }

        size_t get_value() const {
            if (infinite) throw std::logic_error("Infinite cardinal has no finite value");

            return finite_length;
        }

        bool operator==(const Cardinal& other) const {
            if (infinite || other.infinite) return infinite == other.infinite;

            return finite_length == other.finite_length;
        }

        bool operator!=(const Cardinal& other) const {
            return !(*this == other);
        }

        Cardinal operator+(const Cardinal& other) const { // Беск + конечн = беск
            if (infinite || other.infinite) return Cardinal::infinity();

            return Cardinal::finite(finite_length + other.finite_length);
        }

        Cardinal operator-(const Cardinal& other) const {
            if (infinite && other.infinite) throw std::logic_error("infinity - infinity is undefined"); // inf - inf даст неопределённость (logic_error)

            if (infinite) return Cardinal::infinity(); // inf - finite даст inf

            if (other.infinite) throw std::logic_error("finite - infinity would be negative"); // finite - inf даст отрицательное
            
            if (finite_length < other.finite_length) throw std::logic_error("Cardinal subtraction underflow");

            return Cardinal::finite(finite_length - other.finite_length);
        }

        bool operator<(const Cardinal& other) const { // Бесконечное больше любого конечного и равно само себе
            if (infinite) return false;             // inf < x всегда ложь
            if (other.infinite) return true;        // finite < inf всегда истина

            return finite_length < other.finite_length;
        }

        bool operator>(const Cardinal& other) const  { return other < *this; }
        bool operator<=(const Cardinal& other) const { return !(other < *this); }
        bool operator>=(const Cardinal& other) const { return !(*this < other); }
};

#endif
