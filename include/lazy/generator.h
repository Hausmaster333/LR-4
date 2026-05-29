#ifndef GENERATOR_H
#define GENERATOR_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "lazy/ordinal.h"
#include "lazy/ordinal_indexable.h"
#include <functional>

template <class T>
class Generator {
    public:
        virtual size_t position() const = 0; // Индекс следующего элемента, который выдаст get_next()
        virtual bool has_next() const = 0;

        virtual T get_next() = 0;
        virtual Option<T> try_get_next() = 0;

        virtual Ordinal estimate_remaining() const { return Ordinal::infinity(); } // Оценка остатка (для финитных) и infinity() если поток бесконечен или мы не знаем точно

        virtual Generator<T>* clone() const = 0; // Копия генератора с pos = 0, используется в derive-операциях, строится от всей исходной посл-сти, не от текущей pos

        virtual ~Generator() {};
};

template <class T>
inline T materialize_at(const Generator<T>* gen, Ordinal idx) { // Материализовать по ординальному индексу
    if (gen == nullptr) throw std::logic_error("Generator is null");

    auto* indexable = dynamic_cast<const OrdinalIndexable<T>*>(gen);
    if (indexable != nullptr) return indexable->get_at(idx);

    if (idx.get_omega_count() != 0) throw std::logic_error("Generator is not ordinal-indexable and cannot access omega part");

    Generator<T>* tmp_clone = gen->clone();
    T value;
    try {
        for (size_t i = 0; i <= idx.get_finite_part(); i++) {
            if (!tmp_clone->has_next()) {
                delete tmp_clone;
                throw std::out_of_range("Index past finite upstream");
            }
            value = tmp_clone->get_next();
        }
    } catch (...) {
        delete tmp_clone;
        throw;
    }

    delete tmp_clone;
    return value;
}

// SourceGenerator превращает любой готовый Sequence в Generator
template <class T>
class SourceGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Sequence<T>* owned;
        IEnumerator<T>* owned_iter;
        size_t pos;
        size_t owned_count;

        static Sequence<T>* copy_of(const Sequence<T>* source);

        SourceGenerator() : owned(nullptr), owned_iter(nullptr), pos(0), owned_count(0) {}
    public:
        SourceGenerator(const Sequence<T>* source);

        static SourceGenerator<T>* own(Sequence<T>* source); // Передача владения Sequence генератору

        size_t position() const override { return pos; }
        bool has_next() const override { return pos < owned_count; }

        T get_next() override;
        Option<T> try_get_next() override;
        T get_at(Ordinal idx) const override;

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~SourceGenerator() override;
};

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

        Ordinal estimate_remaining() const override { return Ordinal::infinity(); }

        Generator<T>* clone() const override;
};

template <class T>
class PrependGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        T head_item;
        Generator<T>* upstream;
        size_t pos;
    public:
        PrependGenerator(const T& item, Generator<T>* upstream);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        T get_at(Ordinal idx) const override;

        ~PrependGenerator() override;
};

template <class T>
class InsertAtGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Ordinal inject_position;
        Generator<T>* upstream;
        Generator<T>* injected;
        Ordinal injected_length;
        size_t pos;
    public:
        InsertAtGenerator(Ordinal inject_position, const T& item, Generator<T>* upstream);
        InsertAtGenerator(Ordinal inject_position, Generator<T>* injected, Ordinal injected_length, Generator<T>* upstream);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;
        T get_at(Ordinal idx) const override;

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~InsertAtGenerator() override;
};

// MapGenerator<U, T> применяет функцию к каждому элементу upstream<U> и выдаёт T
template <class U, class T>
class MapGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Generator<U>* upstream;
        std::function<T(const U&)> func;
        size_t pos;
    public:
        MapGenerator(Generator<U>* upstream, std::function<T(const U&)> func);

        size_t position() const override { return pos; }
        bool has_next() const override { return upstream->has_next(); }

        T get_next() override;
        Option<T> try_get_next() override;
        T get_at(Ordinal idx) const override;

        Ordinal estimate_remaining() const override { return upstream->estimate_remaining(); }

        Generator<T>* clone() const override;

        ~MapGenerator() override;
};

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

        Ordinal estimate_remaining() const override { return upstream->estimate_remaining(); }
        Generator<T>* clone() const override;

        ~WhereGenerator() override;        
};

template <class U, class V, class T>
class ZipGenerator : public Generator<T>, public OrdinalIndexable<T> {
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
        T get_at(Ordinal idx) const override;

        Ordinal estimate_remaining() const override;
        Generator<T>* clone() const override;

        ~ZipGenerator() override;
};

template <class T>
class ConcatGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Generator<T>* left;
        Ordinal left_length;
        Generator<T>* right;
        size_t pos;
    public:
        ConcatGenerator(Generator<T>* left, Ordinal left_length, Generator<T>* right);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;
        T get_at(Ordinal idx) const override; // Ординальный доступ: idx={0,k} - левая часть, idx={1,k} - правая часть

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        ~ConcatGenerator() override;
};

#include "generator.tpp"

#endif
