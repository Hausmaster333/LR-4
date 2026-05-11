#ifndef OPTION_H
#define OPTION_H

#include <stdexcept>

template <class T>
class Option {
    private:
        T value;
        bool is_value;
    public:
        Option() : is_value(false) {}
        Option(const T& val) : value(val), is_value(true) {}

        bool has_value() const { return is_value; }

        const T& get_value() const {
            if (!is_value) {
                throw std::runtime_error("Option has no value");
            }

            return value;
        }

        static Option<T> None() { return Option<T>(); } // Ничего нет (пустой конструктор)
        static Option<T> Some(const T& val) { return Option<T>(val); } // Что-то есть
};

#endif