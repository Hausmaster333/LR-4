#ifndef HANOI_RING_H
#define HANOI_RING_H

#include <cstring>

class Ring {
    private:
        int size; // Размер - определяет порядок (большее нельзя на меньшее)
        char* shape; // Форма
        char* color; // Цвет
    public:
        Ring() : size(0), shape(nullptr), color(nullptr) {}
        Ring(int size, char* shape, char* color) : size(size) {
            this->shape = new char[strlen(shape) + 1];
            strcpy(this->shape, shape);

            this->color = new char[strlen(color) + 1];
            strcpy(this->color, color);
        }
        Ring(const Ring& other) : size(other.size) {
            this->shape = new char[strlen(other.shape) + 1];
            strcpy(this->shape, other.shape);

            this->color = new char[strlen(other.color) + 1];
            strcpy(this->color, other.color);
        }

        int get_size() const { return size; }
        const char* get_shape() const { return shape; }
        const char* get_color() const { return color; }

        bool operator==(const Ring& other) const { return (size == other.size && strcmp(shape, other.shape) == 0 && strcmp(color, other.color) == 0); }
        bool operator!=(const Ring& other) const { return !(*this == other); }
        bool operator<(const Ring& other) const { return (size < other.size); }
        bool operator>(const Ring& other) const { return (size > other.size); }
        Ring& operator=(const Ring& other) {
            if (this == &other) return *this;

            delete[] shape;
            delete[] color;

            size = other.size;

            this->shape = new char[strlen(other.shape) + 1];
            strcpy(this->shape, other.shape);

            this->color = new char[strlen(other.color) + 1];
            strcpy(this->color, other.color);

            return *this;
        }

        ~Ring() {
            delete[] shape;
            delete[] color;
        }
};   

#endif