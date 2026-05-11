#ifndef GENERATOR_H
#define GENERATOR_H

#include <functional>
#include <cstddef>
#include "core/sequence.h"
#include "core/option.h"
#include "lazy/cardinal.h"

// Forward declaration: Generator хранит указатель на хозяина, полный тип не нужен
template <class T> class LazySequence;

// Базовый абстрактный генератор. Один экземпляр на каждый LazySequence,
// недоступен извне, инкапсулирует правило порождения очередного элемента.
template <class T>
class Generator {
    protected:
        LazySequence<T>* owner; // не владеем — хозяин владеет генератором
        size_t position;        // сколько элементов уже выдали

        // Прокси к приватному API хозяина. Технически необходимы:
        // friendship НЕ наследуется. LazySequence объявил friend для базы
        // Generator<T>, и этот доступ есть только у методов САМОЙ базы.
        // Методы подклассов (RecurrenceGenerator::get_next и т.п.) friend-а
        // не наследуют — без прокси они не могут вызвать owner->cache_push
        // напрямую. Прокси определены в базе → имеют friend-доступ → подклассы
        // вызывают унаследованный protected-метод и через него попадают в кэш.
        // Реализация — в .tpp, нужен полный тип LazySequence.
        void cache_push(const T& item);
        const T& cache_at(size_t i) const;
        size_t cache_size() const;
        void set_length(Cardinal c);
        Cardinal length_hint() const;

        // LazySequence привязывает к себе «недопривязанный» генератор
        // (тот, что вернули append/insert/remove с owner == nullptr).
        void set_owner(LazySequence<T>* o) { owner = o; }
        friend class LazySequence<T>;
    public:
        Generator(LazySequence<T>* owner);
        virtual ~Generator() = default;

        virtual bool has_next() const = 0;
        virtual T get_next() = 0;                 // может бросить IndexOutOfRange (конец)
        virtual Option<T> try_get_next() = 0;     // безопасный аналог

        // Операции из ТЗ. Возвращают новый Generator с owner == nullptr;
        // привязка к новому хозяину делается LazySequence-конструктором,
        // который принимает Generator* и Cardinal.
        // (В ТЗ index у Insert/Remove не указан — трактуем как опечатку,
        // т.к. иначе нет симметрии с конструктором Generator(..., index, ...).
        // Append — частный случай Insert при index == length.)
        virtual Generator<T>* append(const T& item) const;
        virtual Generator<T>* append(const Sequence<T>* items) const;

        virtual Generator<T>* insert(const T& item, size_t index) const;
        virtual Generator<T>* insert(const Sequence<T>* items, size_t index) const;

        virtual Generator<T>* remove(size_t index) const;
        virtual Generator<T>* remove(size_t index, size_t count) const;
};

// Рекуррентное правило: f(последние k элементов) -> очередной элемент.
// Использует MutableArraySequence как «окно» из k последних значений
// (по сути кольцевой буфер фиксированной длины k).
template <class T>
class RecurrenceGenerator : public Generator<T> {
    private:
        std::function<T(Sequence<T>*)> rule;
        size_t k;                              // длина окна
        MutableArraySequence<T> initial;       // копия начальных элементов
        MutableArraySequence<T> window;        // окно последних k элементов
    public:
        RecurrenceGenerator(LazySequence<T>* owner,
                            std::function<T(Sequence<T>*)> rule,
                            const Sequence<T>* initial);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};
// Обёртка над готовой Sequence<T>* — превращает «жадную» последовательность
// в ленивую (читаем по одному элементу в кэш по запросу).
// Источником НЕ владеем: ответственность вызывающего держать его живым.
template <class T>
class SourceGenerator : public Generator<T> {
    private:
        const Sequence<T>* source; // не владеем
    public:
        SourceGenerator(LazySequence<T>* owner, const Sequence<T>* source);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Точечная модификация существующей LazySequence: вставка/удаление одного
// элемента или подпоследовательности в позиции index.
template <class T>
class ModifyingGenerator : public Generator<T> {
    public:
        enum class Kind { Insert, Remove };
    private:
        LazySequence<T>* base;       // не владеем
        size_t index;                // позиция модификации в base
        Sequence<T>* delta;          // владеем (что вставляем/что удаляем по форме)
        Kind kind;
    public:
        ModifyingGenerator(LazySequence<T>* owner,
                           LazySequence<T>* base,
                           size_t index,
                           Sequence<T>* delta,
                           Kind kind);
        ~ModifyingGenerator() override;

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Сцепление двух LazySequence. Если левый бесконечен — правый недостижим.
template <class T>
class ConcatGenerator : public Generator<T> {
    private:
        LazySequence<T>* left;       // не владеем
        LazySequence<T>* right;      // не владеем
    public:
        ConcatGenerator(LazySequence<T>* owner,
                        LazySequence<T>* left,
                        LazySequence<T>* right);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Map: входной тип U, выходной T. База — Generator<T>.
template <class U, class T>
class MapGenerator : public Generator<T> {
    private:
        LazySequence<U>* source;     // не владеем
        std::function<T(const U&)> f;
    public:
        MapGenerator(LazySequence<T>* owner,
                     LazySequence<U>* source,
                     std::function<T(const U&)> f);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Where: фильтр. Позиция в источнике расходится с позицией результата,
// поэтому отдельное поле source_position.
template <class T>
class WhereGenerator : public Generator<T> {
    private:
        LazySequence<T>* source;          // не владеем
        std::function<bool(const T&)> pred;
        size_t source_position;
    public:
        WhereGenerator(LazySequence<T>* owner,
                       LazySequence<T>* source,
                       std::function<bool(const T&)> pred);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Zip: пара двух LazySequence (типы U и V) в один тип T через combiner.
template <class U, class V, class T>
class ZipGenerator : public Generator<T> {
    private:
        LazySequence<U>* a;          // не владеем
        LazySequence<V>* b;          // не владеем
        std::function<T(const U&, const V&)> combiner;
    public:
        ZipGenerator(LazySequence<T>* owner,
                     LazySequence<U>* a,
                     LazySequence<V>* b,
                     std::function<T(const U&, const V&)> combiner);

        bool has_next() const override;
        T get_next() override;
        Option<T> try_get_next() override;
};

// Страховка: если кто-то включил generator.h напрямую, мы тянем lazy_sequence.h,
// чтобы в generator.tpp был полный тип LazySequence. include guards защищают
// от бесконечной рекурсии (см. lazy_sequence.h, который тоже подключает обратно).
#include "lazy/lazy_sequence.h"
#include "generator.tpp"

#endif
