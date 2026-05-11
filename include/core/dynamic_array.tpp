#include "core/dynamic_array.h"
#include <stdexcept>

template <class T>
DynamicArray<T>::DynamicArray() : size(0) {
    data = new T[4];
}

template <class T>
DynamicArray<T>::DynamicArray(int size): size(size) {
    if (size < 0) throw std::out_of_range("Size must be >= 0");

    data = new T[size];
}

template <class T>
DynamicArray<T>::DynamicArray(const T* items, int count): size(count) {
    if (size < 0) throw std::out_of_range("Size must be >= 0");

    data = new T[size];

    for (int idx = 0; idx < size; idx++) {
        data[idx] = items[idx];
    }
}

template <class T>
DynamicArray<T>::DynamicArray(const DynamicArray<T>& other): size(other.size) {
    data = new T[size];

    for (int idx = 0; idx < size; idx++) {
        data[idx] = other.data[idx];
    }
}

template <class T>
DynamicArray<T>& DynamicArray<T>::operator=(const DynamicArray<T>& other) {
    if (this == &other) return *this; // Чтобы не самоприсваивать

    delete[] data;
    size = other.size;
    data = new T[size];

    for (int idx = 0; idx < size; idx++) {
        data[idx] = other.data[idx];
    }

    return *this;
}

template <class T>
const T& DynamicArray<T>::get(int index) const {
    if (index >= size || index < 0) throw std::out_of_range("Index out of range");

    return data[index];
}

template <class T>
int DynamicArray<T>::get_size() const {
    return size;
}

template <class T>
void DynamicArray<T>::set(int index, const T& value) {
    if (index >= size || index < 0) throw std::out_of_range("Index out of range");

    data[index] = value;
}

template <class T>
void DynamicArray<T>::resize(int new_size) {
    if (new_size < 0) throw std::out_of_range("Size must be >= 0");

    T* new_data = new T[new_size];

    int main_size = (new_size < size) ?  new_size : size;

    for (int idx = 0; idx < main_size; idx++) {
        new_data[idx] = data[idx];
    }

    delete[] data;
    data = new_data;
    size = new_size;
}

template <class T>
DynamicArray<T>::~DynamicArray() {
    delete[] data;
}
