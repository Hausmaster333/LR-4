#ifndef LAZY_SEQUENCE_H
#define LAZY_SEQUENCE_H

#include "core/sequence.h"
#include "core/option.h"
#include "core/ienumerator.h"
#include "lazy/cardinal.h"
#include "lazy/sliding_cache.h"
#include "lazy/deferred_tail.h"
#include <functional>
#include <cstdint>

template <class T> class Generator; // Пишем, потому что не надо полный include, его делаем перед tpp файлом, чтобы избежать циклических зависимостей

template <class T>
class LazySequence : public Sequence<T> {
    private:
        // mutable чтобы могли менять поле в const методах
        mutable Generator<T>* generator;
        mutable size_t gen_pos; // Текущая позиция генератора - число выданных элементов

        mutable Cardinal base_length; // Длина части, которую отдаёт generator (без tail)
        mutable Cardinal length; // base_length + tail.added_length (или infinity)

        mutable SlidingCache<T> cache; // Ограниченное окно последних материализованных значений
        mutable DeferredTail<T> tail; // Хвост для операций

        void materialize_up_to(size_t target_index) const; // Гарантирует, что в кэше есть элемент с таргет idx и пишут в кэш элементы, пока генератор не достигнет target_idx

        LazySequence(Generator<T>* generator, Cardinal base_length, int cache_capacity); // Способ создать LazySequence из готового генератора с base_length, нужен методам, которые знают точные параметры

        template <class U>
        friend LazySequence<U>* make_alloc_event_stream_impl(uint64_t, int, int, int); // Фабрики кастомных потоков, где наследник Generator реализован вручную без RecurrenceGenerator и имеет доступ к private конструктору
    protected:
        void sys_append(const T& item) override;
        Sequence<T>* CreateEmpty() const override;
    public:
        static constexpr int DEFAULT_CACHE_CAPACITY = 64;

        explicit LazySequence(int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(const T* items, int count, int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(const Sequence<T>* source, int cache_capacity = DEFAULT_CACHE_CAPACITY);
        LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial, int cache_capacity = DEFAULT_CACHE_CAPACITY);

        LazySequence(const LazySequence& other) = delete;
        LazySequence& operator=(const LazySequence& other) = delete;

        const T& get_first() const override;
        Option<T> try_get_first() const override;

        int get_count() const override; // Полная длина (только для финитных, throw на infinite)

        // На бесконечной в хвост добавляем, на финитной материализуется в конец 
        LazySequence<T>* append(const T& item) override;
        LazySequence<T>* prepend(const T& item) override;
        LazySequence<T>* insert_at(const T& item, int index) override;

        // ========= Бросают logic_error с указанием подходящего метода-замены
        const T& get_last() const override;
        Option<T> try_get_last() const override;
        Sequence<T>* get_sub_sequence(int start, int end) const override;
        Sequence<T>* concat(const Sequence<T>* other) const override;
        Sequence<T>* map(T (*func)(const T&)) const override;
        Sequence<T>* where(bool (*pred)(const T&)) const override;
        T reduce(T (*func)(const T&, const T&), const T& initial) const override;
        Sequence<T>* slice(int index, int count, const Sequence<T>* replace_seq = nullptr) const override;
        // ==============

        Cardinal get_length() const; // Полная длина (Cardinal: finite(N) или infinity)
        int get_materialized_count() const; // число материализованных элементов в кэше
        int get_cache_capacity() const; // Максимальная ёмкость кэша

        // Геттеры для визуализации внутреннего состояния в UI
        Cardinal get_base_length() const { return base_length; }
        int get_tail_op_count() const { return tail.get_op_count(); }
        Cardinal get_tail_added_length() const { return tail.get_added_length(); }

        // Прямое чтение кэша для визуализатора
        // Эти методы показывают, что уже находится в окне
        bool is_cache_empty() const { return cache.is_empty(); }
        size_t get_cache_first_index() const { return cache.get_first_index(); }
        size_t get_cache_last_index() const { return cache.get_last_index(); }
        const T& get_cache_at(size_t logical_index) const { return cache.get(logical_index); }

        // Доступ на чтение к tail (для рендера индивидуальных значений)
        T get_tail_at(int tail_index) const { return tail.get(tail_index); }

        T get(int index); // Чтение элемента по индексу. Материализует кэш до этого индекса, нельзя вызвать по индексу, который вытеснился из кэша

        LazySequence<T>* get_sub_sequence(int start, int end); // Создает финитную, длиной end - start + 1
        LazySequence<T>* take(int n); // Возвращает новую финитную LazySequence из первых n элементов, начиная с 0 idx. Если кэш сдвинулся, то не будет работать, надо делать reset

        // this finite + other finite - other добавляется в tail результата, новая LazySeq имеет тот же base и генератор что и this, но бОльший tail и this полностью не материализуется
        // this finite + other infinite - материализует this в буфер, строит ConcatGenerator в результате бесконечная LazySequence без tail
        // this infinite + other infinite - throw logic_error, т.к. right никогда не будет достижим, концепция не имеет смысла
        LazySequence<T>* concat(LazySequence<T>* other);

        template <class U>
        LazySequence<U>* map(std::function<U(const T&)> func); // Применяет func к каждому элементу, выдаёт LazySequence длиной this (включая tail). Элементы строятся через MapGenerator

        LazySequence<T>* where(std::function<bool(const T&)> pred); // Длина всегда base_length = infinity, точная длина не известна без полной материализации

        template <class U, class R>
        LazySequence<R>* zip(LazySequence<U>* other, std::function<R(const T&, const U&)> combiner); // Комбинация this и other через combiner длиной min(длины обоих, считая infinity бесконечной)

        T reduce(std::function<T(const T&, const T&)> f, const T& initial); // Только для финитных

        ~LazySequence() override;
        // Адаптер LazySequence -> IEnumerator. move_next лениво материализует следующий индекс через owner->get(index). 
        // reset формально работает, но если кэш уже сдвинулся то следующий move_next упадёт
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
