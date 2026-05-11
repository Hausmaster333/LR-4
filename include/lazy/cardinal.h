#ifndef CARDINAL_H
#define CARDINAL_H

#include <cstddef>
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
        static Cardinal zero() { return Cardinal(false, 0); } // удобный синоним finite(0)

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

        // Сложение: бесконечное + что-угодно = бесконечное
        Cardinal operator+(const Cardinal& other) const {
            if (infinite || other.infinite) return Cardinal::infinity();

            return Cardinal::finite(finite_length + other.finite_length);
        }

        // Вычитание:
        //   inf - finite = inf
        //   inf - inf    = неопределённость (logic_error)
        //   finite - inf = отрицательное (logic_error)
        //   finite - finite: если уменьшаемое < вычитаемого → logic_error
        Cardinal operator-(const Cardinal& other) const {
            if (infinite && other.infinite) {
                throw std::logic_error("infinity - infinity is undefined");
            }

            if (infinite) return Cardinal::infinity();

            if (other.infinite) {
                throw std::logic_error("finite - infinity would be negative");
            }
            
            if (finite_length < other.finite_length) {
                throw std::logic_error("Cardinal subtraction underflow");
            }

            return Cardinal::finite(finite_length - other.finite_length);
        }

        // Сравнения. Бесконечное больше любого конечного и равно само себе.
        bool operator<(const Cardinal& other) const {
            if (infinite) return false;             // inf < x всегда ложь
            if (other.infinite) return true;        // finite < inf всегда истина

            return finite_length < other.finite_length;
        }

        bool operator>(const Cardinal& other) const  { return other < *this; }
        bool operator<=(const Cardinal& other) const { return !(other < *this); }
        bool operator>=(const Cardinal& other) const { return !(*this < other); }
};

#endif
