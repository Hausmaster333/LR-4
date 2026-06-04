#ifndef OPTION_H
#define OPTION_H

#include <stdexcept>
#include <new>
#include <utility>

template <class T>
class Option {
    private:
        union { T value; };
        bool is_value;
    public:
        Option() : is_value(false) {}

        Option(const T& val) : is_value(true) {
            new (&value) T(val); // placement new в память union
        }

        Option(const Option& other) : is_value(other.is_value) {
            if (is_value) {
                new (&value) T(other.value);
            }
        }

        Option(Option&& other) noexcept : is_value(other.is_value) {
            if (is_value) {
                new (&value) T(std::move(other.value));
            }
        }

        Option& operator=(const Option& other) {
            if (this != &other) {
                if (is_value) {
                    value.~T();
                }

                is_value = other.is_value;

                if (is_value) {
                    new (&value) T(other.value);
                }
            }
            return *this;
        }

        Option& operator=(Option&& other) noexcept {
            if (this != &other) {
                if (is_value) {
                    value.~T();
                }

                is_value = other.is_value;
                if (is_value) {
                    new (&value) T(std::move(other.value));
                }
            }
            return *this;
        }

        ~Option() {
            if (is_value) value.~T();
        }

        bool has_value() const { return is_value; }

        const T& get_value() const {
            if (!is_value) throw std::runtime_error("Option has no value");

            return value;
        }

        static Option<T> None() { return Option<T>(); } // Ничего нет
        static Option<T> Some(const T& val) { return Option<T>(val); } // Что-то есть
};

#endif
