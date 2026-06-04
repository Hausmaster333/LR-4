#ifndef STREAM_API_H
#define STREAM_API_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "core/option.h"
#include <functional>
#include <stdexcept>

template <class T>
struct StreamStage {
    virtual Option<T> next() = 0; // возвращает следующий элемент в Option (None на конце потока)
    virtual ~StreamStage() {}
};

// Источник из Sequence, тянет элементы лениво через итератор
template <class T>
class SequenceSourceStage : public StreamStage<T> {
    private:
        EnumeratorWrapper<T> enumerator;
    public:
        SequenceSourceStage(IEnumerator<T>* iter) : enumerator(iter) {}

        Option<T> next() override {
            if (!enumerator.move_next()) return Option<T>::None();

            return Option<T>::Some(enumerator.get_current());
        }
};

// Копирует данные в собственную последовательность.
template <class T>
class ArraySourceStage : public StreamStage<T> {
    private:
        MutableArraySequence<T> items;
        int pos;
    public:
        ArraySourceStage(const T* data, int count) : items(data, count), pos(0) {}

        Option<T> next() override {
            if (pos >= items.get_count()) return Option<T>::None();

            return Option<T>::Some(items.get(pos++));
        }
};

template <class T>
class FilterStage : public StreamStage<T> {
    private:
        StreamStage<T>* upstream;
        std::function<bool(const T&)> predicate;
    public:
        FilterStage(StreamStage<T>* upstream, std::function<bool(const T&)> predicate) : upstream(upstream), predicate(predicate) {}

        Option<T> next() override {
            Option<T> value = upstream->next();
            while (value.has_value()) {
                if (predicate(value.get_value())) return value;

                value = upstream->next();
            }
            return Option<T>::None();
        }

        ~FilterStage() override { delete upstream; }
};

template <class T, class U>
class MapStage : public StreamStage<U> {
    private:
        StreamStage<T>* upstream;
        std::function<U(const T&)> mapper;
    public:
        MapStage(StreamStage<T>* upstream, std::function<U(const T&)> mapper) : upstream(upstream), mapper(mapper) {}

        Option<U> next() override {
            Option<T> value = upstream->next();
            if (!value.has_value()) return Option<U>::None();

            return Option<U>::Some(mapper(value.get_value()));
        }

        ~MapStage() override { delete upstream; }
};

// Пропуск первых n элементов
template <class T>
class SkipStage : public StreamStage<T> {
    private:
        StreamStage<T>* upstream;
        int to_skip;
        bool skipped;
    public:
        SkipStage(StreamStage<T>* upstream, int count) : upstream(upstream), to_skip(count < 0 ? 0 : count), skipped(false) {}

        Option<T> next() override {
            if (!skipped) {
                while (to_skip > 0) {
                    Option<T> value = upstream->next();

                    if (!value.has_value()) {
                        skipped = true;
                        return Option<T>::None();
                    }
                    to_skip--;
                }
                skipped = true;
            }

            return upstream->next();
        }

        ~SkipStage() override { delete upstream; }
};

// Взять первые n элементов
template <class T>
class TakeStage : public StreamStage<T> {
    private:
        StreamStage<T>* upstream;
        int remaining;
    public:
        TakeStage(StreamStage<T>* upstream, int count) : upstream(upstream), remaining(count < 0 ? 0 : count) {}

        Option<T> next() override {
            if (remaining <= 0) return Option<T>::None();

            Option<T> value = upstream->next();

            if (!value.has_value()) {
                remaining = 0;
                return Option<T>::None();
            }
            remaining--;

            return value;
        }

        ~TakeStage() override { delete upstream; }
};

template <class T>
class SortStage : public StreamStage<T> {
    private:
        StreamStage<T>* upstream;
        std::function<bool(const T&, const T&)> comparator;
        MutableArraySequence<T> buffer;
        int* order;
        int count;
        int pos;
        bool materialized;

        void materialize() {
            Option<T> value = upstream->next();
            while (value.has_value()) {
                buffer.append(value.get_value());
                value = upstream->next();
            }

            count = buffer.get_count();
            order = new int[count > 0 ? count : 1];
            for (int index = 0; index < count; index++) {
                order[index] = index;
            }

            // order[k] - индекс в буфере элемента, который встанет на k позицию
            for (int boundary = 1; boundary < count; boundary++) {
                int to_insert = order[boundary];
                int slot = boundary - 1;

                while (slot >= 0 && comparator(buffer.get(to_insert), buffer.get(order[slot]))) {
                    order[slot + 1] = order[slot];
                    slot--;
                }
                order[slot + 1] = to_insert;
            }
            materialized = true;
        }
    public:
        SortStage(StreamStage<T>* upstream, std::function<bool(const T&, const T&)> comparator) : upstream(upstream), comparator(comparator), 
                                                                                                  order(nullptr), count(0), pos(0), materialized(false) {}

        Option<T> next() override {
            if (!materialized) materialize();
            if (pos >= count) return Option<T>::None();

            return Option<T>::Some(buffer.get(order[pos++]));
        }

        ~SortStage() override {
            delete[] order;
            delete upstream;
        }
};

template <class T>
class StreamAPI {
    private:
        StreamStage<T>* head;

        StreamAPI(StreamStage<T>* head) : head(head) {}

        StreamStage<T>* take_head() {
            if (head == nullptr) throw std::logic_error("Stream already consumed");

            StreamStage<T>* result = head;
            head = nullptr;
            return result;
        }
    public:
        static StreamAPI<T> of(const Sequence<T>* source) {
            if (source == nullptr) throw std::invalid_argument("Source is nullptr");

            return StreamAPI<T>(new SequenceSourceStage<T>(source->get_enumerator()));
        }

        static StreamAPI<T> of(const T* items, int count) {
            if (count < 0) throw std::invalid_argument("Count < 0");

            return StreamAPI<T>(new ArraySourceStage<T>(items, count));
        }

        StreamAPI<T> filter(std::function<bool(const T&)> predicate) {
            return StreamAPI<T>(new FilterStage<T>(take_head(), predicate));
        }

        template <class U>
        StreamAPI<U> map(std::function<U(const T&)> mapper) {
            return StreamAPI<U>(new MapStage<T, U>(take_head(), mapper));
        }

        StreamAPI<T> sorted(std::function<bool(const T&, const T&)> comparator) {
            return StreamAPI<T>(new SortStage<T>(take_head(), comparator));
        }

        StreamAPI<T> sorted() {
            return sorted([](const T& a, const T& b) { return a < b; });
        }

        StreamAPI<T> take(int n) {
            return StreamAPI<T>(new TakeStage<T>(take_head(), n));
        }

        StreamAPI<T> skip(int n) {
            return StreamAPI<T>(new SkipStage<T>(take_head(), n));
        }

        MutableArraySequence<T>* to_array() {
            StreamStage<T>* stage = take_head();
            MutableArraySequence<T>* result = new MutableArraySequence<T>();

            Option<T> value = stage->next();
            while (value.has_value()) {
                result->append(value.get_value());
                value = stage->next();
            }

            delete stage;
            return result;
        }

        T reduce(std::function<T(const T&, const T&)> func, const T& initial) {
            StreamStage<T>* stage = take_head();
            T accumulator = initial;

            Option<T> value = stage->next();
            while (value.has_value()) {
                accumulator = func(accumulator, value.get_value());
                value = stage->next();
            }

            delete stage;
            return accumulator;
        }

        void for_each(std::function<void(const T&)> action) {
            StreamStage<T>* stage = take_head();

            Option<T> value = stage->next();
            while (value.has_value()) {
                action(value.get_value());
                value = stage->next();
            }

            delete stage;
        }

        int count() {
            StreamStage<T>* stage = take_head();
            int result = 0;

            Option<T> value = stage->next();
            while (value.has_value()) {
                result++;
                value = stage->next();
            }

            delete stage;
            return result;
        }

        StreamAPI(StreamAPI&& other) noexcept : head(other.head) {
            other.head = nullptr;
        }

        StreamAPI& operator=(StreamAPI&& other) noexcept {
            if (this != &other) {
                delete head;
                head = other.head;
                other.head = nullptr;
            }

            return *this;
        }

        StreamAPI(const StreamAPI&) = delete;
        StreamAPI& operator=(const StreamAPI&) = delete;

        ~StreamAPI() {
            delete head;
        }

        template <class U>
        friend class StreamAPI;
};

#endif
