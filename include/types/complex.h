#ifndef COMPLEX_H
#define COMPLEX_H

#include <cmath>

class Complex {
    private:
        double real;
        double imag;
    public:
        Complex() : real(0.0), imag(0.0) {}
        Complex(double real, double imag) : real(real), imag(imag) {}

        double get_real() const { return real; }
        double get_imag() const { return imag; }
        double modulus() const { return sqrt(real * real + imag * imag); } // sqrt(real^2 + imag^2)

        bool operator==(const Complex& other) const { return (real == other.real && imag == other.imag); }
        bool operator!=(const Complex& other) const { return !(*this == other); } // Не (равно)
        bool operator<(const Complex& other) const { return (modulus() < other.modulus()); }
        bool operator>(const Complex& other) const { return (modulus() > other.modulus()); }
};

#endif