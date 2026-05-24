#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include "core/sequence.h"
#include "core/option.h"
#include "core/ienumerator.h"
#include "lazy/ordinal.h"
#include "lazy/sliding_cache.h"
#include <functional>
#include <cstdint>

template <class T> class Generator;

template <class T>
class LazySequence : public Sequence<T> {
    private:
        // mutable - материализация в const методах меняет внутреннее состояние
        mutable Generator<T>* generator;
        mutable size_t gen_pos;         // сколько уже выдано генератором
        mutable Ordinal length;         // полная длина
        mutable SlidingCache<T> cache;  // окно последних материализованных значений

        void materialize_up_to(size_t target_index) const;

    protected:
        void sys_append(const T& item) override;
        Sequence<T>* CreateEmpty() const override;
    public:
        static constexpr int DEFAULT_CACHE_CAPACITY = 64;

        LazySequence(int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(const T* items, int count, int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(const Sequence<T>* source, int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial, int cache_capacity = DEFAULT_CACHE_CAPACITY);

        // Конструктор из готового generator-а с заявленной длиной. Используется derive-операциями
        // и фабриками кастомных стримов (например, make_alloc_event_stream).
        LazySequence(Generator<T>* generator, Ordinal length, int cache_capacity = DEFAULT_CACHE_CAPACITY);

        LazySequence(const LazySequence& other) = delete;
        LazySequence& operator=(const LazySequence& other) = delete;

        const T& get_first() const override;
        Option<T> try_get_first() const override;

        int get_count() const override;

        LazySequence<T>* append(const T& item) override; // На inf item_idx = (w, 0)
        LazySequence<T>* prepend(const T& item) override;

        LazySequence<T>* insert_at(const T& item, int index) override;
        LazySequence<T>* insert_at(const T& item, Ordinal position);

        LazySequence<T>* insert_at(LazySequence<T>* other, int index);
        LazySequence<T>* insert_at(LazySequence<T>* other, Ordinal position);

        // ========= throw logic_error
        const T& get_last() const override;
        Option<T> try_get_last() const override;
        Sequence<T>* get_sub_sequence(int start, int end) const override;
        Sequence<T>* concat(const Sequence<T>* other) const override;
        Sequence<T>* map(T (*func)(const T&)) const override;
        Sequence<T>* where(bool (*pred)(const T&)) const override;
        T reduce(T (*func)(const T&, const T&), const T& initial) const override;
        Sequence<T>* slice(int index, int count, const Sequence<T>* replace_seq = nullptr) const override;
        // ==============

        Ordinal get_length() const;
        int get_materialized_count() const;
        int get_cache_capacity() const;

        bool is_cache_empty() const { return cache.is_empty(); }
        size_t get_cache_first_index() const { return cache.get_first_index(); }
        size_t get_cache_last_index() const { return cache.get_last_index(); }
        const T& get_cache_at(size_t logical_index) const { return cache.get(logical_index); }

        T get(int index);
        T get(Ordinal idx);

        LazySequence<T>* get_sub_sequence(int start, int end);

        LazySequence<T>* take(int n);     // Финитизация первых n элементов в SourceGen.
        LazySequence<T>* take(Ordinal limit);  // Для omega_part > 0 - просто новый LazySequence с ограниченной длиной

        // concat(other): универсальная цепочка ConcatGenerator(this.gen, this.length, other.gen).
        // Длина = this.length + other.length (ординально). Цепочки concat дают ω·k.
        LazySequence<T>* concat(LazySequence<T>* other);

        template <class U>
        LazySequence<U>* map(std::function<U(const T&)> func);

        // where(pred): реальное длина известно только при материализации.
        LazySequence<T>* where(std::function<bool(const T&)> pred);

        template <class U, class R>
        LazySequence<R>* zip(LazySequence<U>* other, std::function<R(const T&, const U&)> combiner);

        T reduce(std::function<T(const T&, const T&)> f, const T& initial);

        ~LazySequence() override;

        class Enumerator : public IEnumerator<T> {
            private:
                LazySequence<T>* owner;
                int index;
                T current;
                bool has_current;
            public:
                Enumerator(LazySequence<T>* owner) : owner(owner), index(-1), has_current(false) {}

                bool move_next() override {
                    index++;
                    try {
                        current = owner->get(index);
                        has_current = true;
                        return true;
                    } catch (const std::out_of_range&) {
                        has_current = false;
                        return false;
                    }
                }

                const T& get_current() const override {
                    if (!has_current) {
                        throw std::logic_error("LazySequence::Enumerator: no current value (call move_next first)");
                    }
                    return current;
                }

                void reset() override {
                    index = -1;
                    has_current = false;
                }
        };

        IEnumerator<T>* get_enumerator() const override;
};

#include "lazy/generator.h"
#include "lazy_sequence.tpp"

#endif
