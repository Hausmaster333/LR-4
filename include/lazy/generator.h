#ifndef GENERATOR_H
#define GENERATOR_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "lazy/cardinal.h"
#include "lazy/ordinal_index.h"
#include "lazy/ordinal_indexable.h"
#include <functional>

template <class T>
class Generator { // Выдает элементы по запросу и всё
    public:
        // Индекс следующего элемента, который выдаст get_next(). Растёт монотонно,
        // начальное значение 0. Используется LazySequence для синхронизации с кэшем
        virtual size_t position() const = 0;
        virtual bool has_next() const = 0; // Проверка, если ли остаток, для бесконечных очевидно всегда true

        virtual T get_next() = 0; // Продвинуться на 1 + вернуть значение
        virtual Option<T> try_get_next() = 0;

        virtual Cardinal estimate_remaining() const { return Cardinal::infinity(); } // Оценка остатка (для финитных) и infinity() если поток бесконечен или мы не знаем точно

        virtual Generator<T>* clone() const = 0; // Копия генератора с pos = 0, используется в derive-операциях, строится от всей исходной посл-сти, не от текущей pos

        virtual ~Generator() = default;
};

// Рекуррентное правило f(окно последних k) -> next
// На первом этапе выдаёт начальные элементы и заполняет окно, а дальше применяет к нему правило и сдвигает его влево
template <class T>
class RecurrenceGenerator : public Generator<T> {
    private:
        std::function<T(Sequence<T>*)> rule;
        size_t k; // Размер окна = число элементов в initial
        MutableArraySequence<T> initial; // Копия начальных элементов (для clone)
        MutableArraySequence<T> window; // Текущее окно из k последних значений

        size_t pos;
    public:
        RecurrenceGenerator(std::function<T(Sequence<T>*)> rule, const Sequence<T>* initial);

        size_t position() const override { return pos; }
        bool has_next() const override { return true; }

        T get_next() override;
        Option<T> try_get_next() override { return Option<T>::Some(get_next()); }

        Cardinal estimate_remaining() const override { return Cardinal::infinity(); }

        Generator<T>* clone() const override;
};

// SourceGenerator превращает любой готовый Sequence (массив, list и тд) в Generator, чтобы LazySequence могла работать с ним через свой единый интерфейс
template <class T>
class SourceGenerator : public Generator<T> {
    private:
        Sequence<T>* owned; // Исходная
        IEnumerator<T>* iterator; // Продвигается по owned
        size_t pos; // Сколько уже выдано(индекс следующего элемента, который выдаст get_next)
        size_t total; // Сколько элементов в owned

        static Sequence<T>* copy_of(const Sequence<T>* source); // Глубокая копия в новый MutableArray

        SourceGenerator() : owned(nullptr), iterator(nullptr), pos(0), total(0) {} // Используется только из own
    public:
        SourceGenerator(const Sequence<T>* source); // Создаёт независимую копию source внутри

        static SourceGenerator<T>* own(Sequence<T>* source); // Фабрика, которая создаёт SourceGenerator БЕЗ копирования source. Вызывающий передаёт уже готовый буфер, забирает его без копирования

        size_t position() const override { return pos; }
        bool has_next() const override { return pos < total; }

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~SourceGenerator() override;
};

template <class T>
class PrependGenerator : public Generator<T> { // Копия генератора оригинальной LazySequence
    private:
        T head_item;
        Generator<T>* upstream; // Основной генератор
        size_t pos;
    public:
        PrependGenerator(const T& item, Generator<T>* upstream); // Prepend хранит head_item и upstream. При обращении к нулевому элементу отдаёт head_item, дальше просто проксирует к upstream

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~PrependGenerator() override;        
};

// MapGenerator<U, T> - применяет f к каждому элементу upstream<U>, выдаёт T
// Обращается к исходному генератору и применяем к полученному значению функцию и возвращает новое значение
template <class U, class T>
class MapGenerator : public Generator<T> {
    private:
        Generator<U>* upstream; // Исходный генератор
        std::function<T(const U&)> func;
        size_t pos;
    public:
        MapGenerator(Generator<U>* upstream, std::function<T(const U&)> func);

        size_t position() const override { return pos; }
        bool has_next() const override { return upstream->has_next(); }

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override { return upstream->estimate_remaining(); }

        Generator<T>* clone() const override;

        ~MapGenerator() override;
};

// WhereGenerator фильтрует upstream через pred
// has_next возвращает upstream->has_next
// если на бесконечной pred никогда не срабатывает, то get_nextзависнет
template <class T>
class WhereGenerator : public Generator<T> {
    private:
        Generator<T>* upstream;
        std::function<bool(const T&)> pred;
        size_t pos;
    public:
        WhereGenerator(Generator<T>* upstream, std::function<bool(const T&)> pred);

        size_t position() const override { return pos; }
        bool has_next() const override { return upstream->has_next(); }

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override { return upstream->estimate_remaining(); }
        Generator<T>* clone() const override;

        ~WhereGenerator() override;        
};

// ZipGenerator<U, V, T> - попарная композиция через combiner
// Конечен если хоть один из upstream-ов конечен
template <class U, class V, class T>
class ZipGenerator : public Generator<T> {
    private:
        Generator<U>* first;
        Generator<V>* second;
        std::function<T(const U&, const V&)> combiner;
        size_t pos;
    public:
        ZipGenerator(Generator<U>* first, Generator<V>* second, std::function<T(const U&, const V&)> combiner);

        size_t position() const override { return pos; }
        bool has_next() const override { return first->has_next() && second->has_next(); }

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override;
        Generator<T>* clone() const override;

        ~ZipGenerator() override;        
};

// ConcatGenerator<T> - конкатенация двух последовательностей.
// Линейный режим get_next: обходит подряд left, потом right (для inf-left до right линейно не доберётся).
// Ординальный режим get_at: {0,k} - k-й слева, {1,k} - k-й справа, минуя left.
// Это позволяет concat(inf, inf) и доступ get({1, 2}) = 2-й элемент правой части.
template <class T>
class ConcatGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Generator<T>* left;
        Cardinal left_length;
        Generator<T>* right;
        size_t pos;

        // Создаёт клон gen и прокручивает до target_index-го элемента
        static T materialize_at(const Generator<T>* gen, size_t target_index) {
            if (gen == nullptr) throw std::logic_error("ConcatGenerator: upstream is null");
            Generator<T>* probe = gen->clone();
            T value;
            try {
                for (size_t i = 0; i <= target_index; i++) {
                    if (!probe->has_next()) {
                        delete probe;
                        throw std::out_of_range("ConcatGenerator: index past finite upstream");
                    }
                    value = probe->get_next();
                }
            } catch (...) {
                delete probe;
                throw;
            }
            delete probe;
            return value;
        }
    public:
        ConcatGenerator(Generator<T>* left, Cardinal left_length, Generator<T>* right);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;
        T get_at(OrdinalIndex idx) const override; // Ординальный доступ: idx={0,k} - левая часть, idx={1,k} - правая часть

        Cardinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~ConcatGenerator() override;
};

// InsertAtGenerator<T> - вставка одного элемента или целой последовательности (в т.ч. бесконечной) в произвольную позицию upstream.
//
// Линейный режим (get_next):
//   - injected финитный длины m: upstream[0..p), injected[0..m), upstream[p..)
//   - injected бесконечный: upstream[0..p), injected[0..) - upstream после p
//     линейно недостижим, остаётся для get_at({1, k})
//
// Ординальный режим (get_at):
//   injected финитный длины m:
//     {0, i}, i<p           -> upstream[i]
//     {0, p..p+m-1}         -> injected[i-p]
//     {0, i}, i>=p+m        -> upstream[i-m]
//   injected бесконечный:
//     {0, i}, i<p           -> upstream[i]
//     {0, i}, i>=p          -> injected[i-p]
//     {1, k}                -> upstream[p+k]  (хвост, "оттеснённый" за омегу)
template <class T>
class InsertAtGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        size_t inject_position;
        Generator<T>* upstream;
        Generator<T>* injected;
        Cardinal injected_length;
        size_t pos;

        // Перемотка свежего клона gen на k-й элемент
        static T materialize_at(const Generator<T>* gen, size_t target_index) {
            if (gen == nullptr) throw std::logic_error("InsertAtGenerator: upstream is null");
            Generator<T>* probe = gen->clone();
            T value;
            try {
                for (size_t i = 0; i <= target_index; i++) {
                    if (!probe->has_next()) {
                        delete probe;
                        throw std::out_of_range("InsertAtGenerator: index past finite upstream");
                    }
                    value = probe->get_next();
                }
            } catch (...) {
                delete probe;
                throw;
            }
            delete probe;
            return value;
        }
    public:
        // Старый API: вставка одного элемента (внутри оборачивается в одноэлементный генератор)
        InsertAtGenerator(size_t inject_position, const T& item, Generator<T>* upstream);
        InsertAtGenerator(size_t inject_position, Generator<T>* injected, Cardinal injected_length, Generator<T>* upstream);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;

        Cardinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        T get_at(OrdinalIndex idx) const override;

        ~InsertAtGenerator() override;
};

#include "generator.tpp"

#endif
