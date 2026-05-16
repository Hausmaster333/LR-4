#ifndef DEFERRED_TAIL_H
#define DEFERRED_TAIL_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "lazy/cardinal.h"

// Хвост отложенных операций над концом ленивой последовательности
// Хранит упорядоченный список операций (AppendOne / ConcatSeq), применяется к концу при take(N) или при выходе индекса за base_length

// Для трансфинитивности: на бесконечной LazySequence операции висят в tail
// до тех пор, пока последовательность не будет обрезана через take
// На конечной - tail виден через get(i) для i >= base_length
template <class T>
class DeferredTail {
    private:
        enum class Kind { // Типы операций
            AppendOne, // Добавить 1 элемент
            ConcatSeq // Добавить Последовательность
        };

        // Одна отложенная операция
        // Храним оба поля - используем то, что соответствует kind
        struct Op {
            Kind kind;
            T single; // Для AppendOne
            MutableArraySequence<T> seq; // Для ConcatSeq
            Op() : kind(Kind::AppendOne) {}
        };

        MutableArraySequence<Op> ops;
    public:
        DeferredTail() = default;
        DeferredTail(const DeferredTail& other) = default;

        DeferredTail& operator=(const DeferredTail& other) = default;

        bool is_empty() const { return ops.get_count() == 0; } // Проверка очереди на пустоту

        void push_append(const T& item); // Кладёт одиночный элемент в конец (операция AppendOne)
        void push_concat(const Sequence<T>* other); // Кладёт целую последовательность в конец (операция ConcatSeq), копирует элементы внутрь, исходник можно удалять после вызов

        int get_op_count() const { return ops.get_count(); } // Число операций в очереди (не общее число элементов)
        Cardinal get_added_length() const; // Число элементов, которое прибавится к последовательности

        // Обходит ops, тратя tail_index на каждый Op (1 на AppendOne, длина seq на ConcatSeq)
        T get(int tail_index) const; // Возвращает элемент по абсолютному индексу внутри tail от 0 до get_added_length - 1

        void apply_to(MutableArraySequence<T>& target) const; // Материализует все накопленные операции в конец target
};

#include "deferred_tail.tpp"

#endif
