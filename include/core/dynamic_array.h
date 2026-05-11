#ifndef ARRAY_H
#define ARRAY_H

#include "core/ienumerator.h"

template <class T>
class DynamicArray {
    private:
        T* data;
        int size;
    public:
        DynamicArray();
        DynamicArray(int size);
        DynamicArray(const T* items, int count);
        DynamicArray(const DynamicArray<T>& other);

        DynamicArray<T>& operator=(const DynamicArray<T>& other);

        const T& get(int index) const;

        int get_size() const;

        void set(int index, const T& value);

        void resize(int newSize);

        ~DynamicArray();

        class Enumerator : public IEnumerator<T> {
            private:
                T* data;
                int size;
                int index;
            public : 
                Enumerator(T* data, int size) : data(data), size(size), index(-1) {}

                bool move_next() override {
                    index++;
                    return index < size;
                }

                const T& get_current() const override {
                    return data[index];
                }

                void reset() override {
                    index = -1;
                }
        };

        IEnumerator<T>* get_enumerator() const {
            return new Enumerator(data, size);
        }

        IEnumerator<T>* get_enumerator(int count) const {
            return new Enumerator(data, count);
        }
};

#include "dynamic_array.tpp"

#endif