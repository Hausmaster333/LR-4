#ifndef SEQUENCE_STREAM_H
#define SEQUENCE_STREAM_H

#include "core/sequence.h"
#include <functional>
#include <stdexcept>

template <class T>
class SequenceStream {
    private:
        MutableArraySequence<T>* data;

        SequenceStream(MutableArraySequence<T>* data) : data(data) {}

        MutableArraySequence<T>* release() { // Передача владения 
            if (data == nullptr) throw std::logic_error("Stream already consumed");

            MutableArraySequence<T>* result = data;
            data = nullptr;

            return result;
        }
    public:
        static SequenceStream<T> of(const Sequence<T>* source) {
            if (source == nullptr) throw std::invalid_argument("Source is nullptr");

            MutableArraySequence<T>* buffer = new MutableArraySequence<T>();
            IEnumerator<T>* iter = source->get_enumerator();

            while (iter->move_next()) {
                buffer->append(iter->get_current());
            }

            delete iter;
            return SequenceStream<T>(buffer);
        }

        static SequenceStream<T> of(const T* items, int count) {
            if (count < 0) throw std::invalid_argument("Count < 0");

            MutableArraySequence<T>* buffer = new MutableArraySequence<T>(items, count);
            return SequenceStream<T>(buffer);
        }

        SequenceStream<T> filter(std::function<bool(const T&)> predicate) {
            MutableArraySequence<T>* source = release();
            MutableArraySequence<T>* result = new MutableArraySequence<T>();

            for (int i = 0; i < source->get_count(); i++) {
                const T& element = source->get(i);
                if (predicate(element)) result->append(element);
            }

            delete source;
            return SequenceStream<T>(result);
        }

        template <class U>
        SequenceStream<U> map(std::function<U(const T&)> func) {
            MutableArraySequence<T>* source = release();
            MutableArraySequence<U>* result = new MutableArraySequence<U>();

            for (int i = 0; i < source->get_count(); i++) {
                result->append(func(source->get(i)));
            }

            delete source;
            return SequenceStream<U>(result);
        }

        SequenceStream<T> sorted(std::function<bool(const T&, const T&)> comparator) {
            MutableArraySequence<T>* source = release();
            int count = source->get_count();

            // order[k] = индекс в source элемента, который должен встать на k-ю позицию результата.
            // Сортируем индексы, чтобы не двигать сами T-объекты (insertion sort)
            int* order = new int[count];
            for (int pos = 0; pos < count; pos++) order[pos] = pos;

            for (int boundary = 1; boundary < count; boundary++) {
                int to_insert = order[boundary];
                int slot = boundary - 1;
                while (slot >= 0 && comparator(source->get(to_insert), source->get(order[slot]))) {
                    order[slot + 1] = order[slot];
                    slot--;
                }
                order[slot + 1] = to_insert;
            }

            MutableArraySequence<T>* result = new MutableArraySequence<T>();
            for (int pos = 0; pos < count; pos++) {
                result->append(source->get(order[pos]));
            }

            delete[] order;
            delete source;
            return SequenceStream<T>(result);
        }

        SequenceStream<T> sorted() {
            return sorted([](const T& a, const T& b) { return a < b; });
        }

        SequenceStream<T> take(int n) {
            MutableArraySequence<T>* source = release();
            MutableArraySequence<T>* result = new MutableArraySequence<T>();

            int limit = (n < source->get_count()) ? n : source->get_count();
            for (int i = 0; i < limit; i++) {
                result->append(source->get(i));
            }

            delete source;
            return SequenceStream<T>(result);
        }

        SequenceStream<T> skip(int n) {
            MutableArraySequence<T>* source = release();
            MutableArraySequence<T>* result = new MutableArraySequence<T>();

            for (int i = n; i < source->get_count(); i++) {
                result->append(source->get(i));
            }

            delete source;
            return SequenceStream<T>(result);
        }

        MutableArraySequence<T>* to_array() {
            return release();
        }

        T reduce(std::function<T(const T&, const T&)> func, const T& initial) {
            MutableArraySequence<T>* source = release();
            T accumulator = initial;

            for (int i = 0; i < source->get_count(); i++) {
                accumulator = func(accumulator, source->get(i));
            }

            delete source;
            return accumulator;
        }

        void for_each(std::function<void(const T&)> action) {
            MutableArraySequence<T>* source = release();

            for (int i = 0; i < source->get_count(); i++) {
                action(source->get(i));
            }

            delete source;
        }

        int count() {
            MutableArraySequence<T>* source = release();
            int result = source->get_count();

            delete source;
            return result;
        }

        SequenceStream(SequenceStream&& other) noexcept : data(other.data) {
            other.data = nullptr;
        }

        SequenceStream& operator=(SequenceStream&& other) noexcept {
            if (this != &other) {
                delete data;
                data = other.data;
                other.data = nullptr;
            }
            return *this;
        }

        SequenceStream(const SequenceStream&) = delete;
        SequenceStream& operator=(const SequenceStream&) = delete;

        ~SequenceStream() {
            delete data;
        }

        template <class U>
        friend class SequenceStream;
};

#endif
