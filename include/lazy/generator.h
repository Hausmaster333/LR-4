#ifndef GENERATOR_H
#define GENERATOR_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "lazy/ordinal.h"
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

        virtual Ordinal estimate_remaining() const { return Ordinal::infinity(); } // Оценка остатка (для финитных) и infinity() если поток бесконечен или мы не знаем точно

        virtual Generator<T>* clone() const = 0; // Копия генератора с pos = 0, используется в derive-операциях, строится от всей исходной посл-сти, не от текущей pos

        virtual ~Generator() = default;
};

// Линейный материализатор: клонирует gen и прокручивает до target_index-го элемента.
// Универсальный fallback для генераторов без OrdinalIndexable. O(N) на вызов.
template <class T>
inline T materialize_at_linear(const Generator<T>* gen, size_t target_index) {
    if (gen == nullptr) throw std::logic_error("materialize_at_linear: upstream is null");
    Generator<T>* probe = gen->clone();
    T value;
    try {
        for (size_t i = 0; i <= target_index; i++) {
            if (!probe->has_next()) {
                delete probe;
                throw std::out_of_range("materialize_at_linear: index past finite upstream");
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

// Ординальный материализатор: если gen реализует OrdinalIndexable - форвардит get_at;
// иначе для финитной части использует линейный fallback; для omega_part > 0 бросает.
template <class T>
inline T materialize_at_ord(const Generator<T>* gen, OrdinalIndex idx) {
    auto* up = dynamic_cast<const OrdinalIndexable<T>*>(gen);
    if (up != nullptr) return up->get_at(idx);
    if (idx.omega_part != 0)
        throw std::logic_error("materialize_at_ord: generator is not ordinal-indexable, cannot access omega-part");
    return materialize_at_linear(gen, idx.finite_part);
}

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

        Ordinal estimate_remaining() const override { return Ordinal::infinity(); }

        Generator<T>* clone() const override;
};

// SourceGenerator превращает любой готовый Sequence в Generator.
// Реализует OrdinalIndexable<T> — доступ только в финитной части (omega_part == 0).
template <class T>
class SourceGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Sequence<T>* owned;
        IEnumerator<T>* iterator;
        size_t pos;
        size_t total;

        static Sequence<T>* copy_of(const Sequence<T>* source);

        SourceGenerator() : owned(nullptr), iterator(nullptr), pos(0), total(0) {}
    public:
        SourceGenerator(const Sequence<T>* source); // deep copy

        static SourceGenerator<T>* own(Sequence<T>* source); // takes ownership

        size_t position() const override { return pos; }
        bool has_next() const override { return pos < total; }

        T get_next() override;
        Option<T> try_get_next() override;

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        T get_at(OrdinalIndex idx) const override;

        ~SourceGenerator() override;
};

// Реализует OrdinalIndexable<T>: {0,0} -> head_item, {0, k>0} -> upstream[k-1], {p>0, k} -> upstream[{p, k}] (1+ω=ω).
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

        T get_at(OrdinalIndex idx) const override;

        ~PrependGenerator() override;
};

// MapGenerator<U, T> - применяет f к каждому элементу upstream<U>, выдаёт T.
// Реализует OrdinalIndexable<T> с форвардом на OrdinalIndexable<U> upstream
// (через dynamic_cast); если upstream не индексируемый, доступен только линейный fallback
// для omega_part == 0.
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

        Ordinal estimate_remaining() const override { return upstream->estimate_remaining(); }

        Generator<T>* clone() const override;

        T get_at(OrdinalIndex idx) const override;

        ~MapGenerator() override;
};

// WhereGenerator фильтрует upstream через pred
// has_next возвращает upstream->has_next
// если на бесконечной pred никогда не срабатывает, то get_next зависнет.
//
// ВНИМАНИЕ: WhereGenerator НЕ реализует OrdinalIndexable - фильтрация делает
// ординальные индексы недетерминированными без полной материализации. Если
// нужен ординальный доступ, ставьте where() ПОСЛЕ финитизации (take/...).
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

// ZipGenerator<U, V, T> - попарная композиция через combiner.
// Конечен если хоть один из upstream-ов конечен.
// Реализует OrdinalIndexable<T>: требует обе стороны OrdinalIndexable
// (либо omega_part == 0 для линейного fallback'а).
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

        Ordinal estimate_remaining() const override;
        Generator<T>* clone() const override;

        T get_at(OrdinalIndex idx) const override;

        ~ZipGenerator() override;
};

// ConcatGenerator<T> - конкатенация двух последовательностей произвольной (в т.ч. трансфинитной) длины.
// Линейный режим get_next: обходит подряд left, потом right (для inf-left до right линейно не доберётся).
// Ординальный режим get_at: обобщённый для ω·k. Если idx < left_length - в левой;
// иначе right.get_at(idx - left_length). Поддерживает цепочки concat(concat(inf,inf),inf) = ω·3 и т.д.
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
        T get_at(OrdinalIndex idx) const override; // Ординальный доступ: idx={0,k} - левая часть, idx={1,k} - правая часть

        Ordinal estimate_remaining() const override;

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
        Ordinal injected_length;
        size_t pos;
    public:
        // Старый API: вставка одного элемента (внутри оборачивается в одноэлементный генератор)
        InsertAtGenerator(size_t inject_position, const T& item, Generator<T>* upstream);
        InsertAtGenerator(size_t inject_position, Generator<T>* injected, Ordinal injected_length, Generator<T>* upstream);

        size_t position() const override { return pos; }
        bool has_next() const override;

        T get_next() override;
        Option<T> try_get_next() override;

        Ordinal estimate_remaining() const override;

        Generator<T>* clone() const override;

        T get_at(OrdinalIndex idx) const override;

        ~InsertAtGenerator() override;
};

// TakeOrdinalGenerator<T> - обёртка над upstream с ограничением длины ординалом limit.
// Линейный get_next: продвигается по upstream, останавливается когда позиция >= limit (если limit финитен)
// или когда upstream выдохся.
// get_at(idx): требует idx < limit; форвардит upstream->get_at.
// Используется LazySequence::take(OrdinalIndex) для трансфинитной обрезки.
template <class T>
class TakeOrdinalGenerator : public Generator<T>, public OrdinalIndexable<T> {
    private:
        Generator<T>* upstream;
        OrdinalIndex limit;
        size_t pos;
    public:
        TakeOrdinalGenerator(Generator<T>* upstream, OrdinalIndex limit)
            : upstream(upstream), limit(limit), pos(0) {
            if (upstream == nullptr) throw std::invalid_argument("TakeOrdinalGenerator: upstream is nullptr");
        }

        size_t position() const override { return pos; }
        bool has_next() const override {
            // Если limit имеет omega_part > 0, линейный обход никогда не пересечёт первый ω-блок.
            // Так что в финитной части мы ограничены limit'ом только когда omega_part == 0.
            if (limit.omega_part == 0 && pos >= limit.finite_part) return false;
            return upstream->has_next();
        }

        T get_next() override {
            if (!has_next()) throw std::out_of_range("TakeOrdinalGenerator: limit reached");
            T value = upstream->get_next();
            pos++;
            return value;
        }
        Option<T> try_get_next() override {
            if (!has_next()) return Option<T>::None();
            return Option<T>::Some(get_next());
        }

        Ordinal estimate_remaining() const override {
            if (limit.omega_part == 0) {
                if (pos >= limit.finite_part) return Ordinal::zero();
                return Ordinal::finite(limit.finite_part - pos);
            }
            // limit имеет ω-блоки: бесконечно элементов осталось (мы в первом ω-блоке)
            return Ordinal::omega_times(limit.omega_part) + Ordinal::finite(limit.finite_part);
        }

        Generator<T>* clone() const override {
            return new TakeOrdinalGenerator<T>(upstream->clone(), limit);
        }

        T get_at(OrdinalIndex idx) const override {
            if (!(idx < limit_as_ordinal()))
                throw std::out_of_range("TakeOrdinalGenerator::get_at: index >= limit");
            return materialize_at_ord(upstream, idx);
        }

        ~TakeOrdinalGenerator() override { delete upstream; }
    private:
        Ordinal limit_as_ordinal() const {
            return Ordinal::omega_times(limit.omega_part) + Ordinal::finite(limit.finite_part);
        }
};

#include "generator.tpp"

#endif
