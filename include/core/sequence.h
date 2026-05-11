#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "core/dynamic_array.h"
#include "core/linked_list.h"
#include "core/option.h"

template <class T>
class Sequence {
    protected:
        virtual void sys_append(const T& item) = 0;
        
        virtual Sequence<T>* CreateEmpty() const = 0;
    public:
        virtual const T& get_first() const = 0; // const T& ссылка, чтобы не копировать объект, а просто const запрещает менять значение через эту ссылку
        virtual const T& get_last() const = 0;

        virtual Option<T> try_get_first() const = 0;
        virtual Option<T> try_get_last() const = 0;

        virtual int get_count() const = 0;

        virtual Sequence<T>* get_sub_sequence(int start, int end) const;

        virtual Sequence<T>* append(const T& item) = 0;
        virtual Sequence<T>* prepend(const T& item) = 0;
        virtual Sequence<T>* insert_at(const T& item, int index) = 0;

        virtual Sequence<T>* concat(const Sequence<T>* other) const;
        virtual Sequence<T>* map(T (*func)(const T& elem)) const;
        virtual Sequence<T>* where(bool (*predicate)(const T& elem)) const;
        virtual T reduce(T (*func)(const T& accumulator, const T& current), const T& initial_elem) const;

        virtual Sequence<T>* slice(int index, int count, const Sequence<T>* replace_seq = nullptr) const;

        virtual IEnumerator<T>* get_enumerator() const = 0;

        virtual ~Sequence() {}
};

template <class T>
class ArraySequence : public Sequence<T> {
    template <class U>
    friend size_t measure_array_seq_memory(const ArraySequence<U>& seq);
    protected:    
        DynamicArray<T> array;
        int count;

        void sys_append(const T& item) override;

        virtual ArraySequence<T>* Instance() = 0;
        virtual ArraySequence<T>* CreateEmpty() const = 0;
    public:
        ArraySequence();
        ArraySequence(const T* items, int count); // const items
        ArraySequence(const DynamicArray<T>& other);
        ArraySequence(const ArraySequence<T>& other);

        const T& get_first() const override;
        const T& get_last() const override;
        const T& get(int index) const;

        Option<T> try_get_first() const override;
        Option<T> try_get_last() const override;
        Option<T> try_get(int index) const;

        int get_count() const override;

        Sequence<T>* append(const T& item) override;
        Sequence<T>* prepend(const T& item) override;
        Sequence<T>* insert_at(const T& item, int index) override;

        IEnumerator<T>* get_enumerator() const override {
            return array.get_enumerator(count);
        }

        ~ArraySequence() {}
};

template <class T>
class ListSequence : public Sequence<T> {
    template <class U>
    friend size_t measure_list_seq_memory(const ListSequence<U>& seq);
    protected:    
        LinkedList<T> list;

        void sys_append(const T& item) override;

        virtual ListSequence<T>* Instance() = 0;
        virtual ListSequence<T>* CreateEmpty() const = 0;        
    public:
        ListSequence();
        ListSequence(const T* items, int count);
        ListSequence(const LinkedList<T>& other);
        ListSequence(const ListSequence<T>& other);

        const T& get_first() const override;
        const T& get_last() const override;

        Option<T> try_get_first() const override;
        Option<T> try_get_last() const override;

        int get_count() const override;

        Sequence<T>* append(const T& item) override;
        Sequence<T>* prepend(const T& item) override;
        Sequence<T>* insert_at(const T& item, int index) override;

        IEnumerator<T>* get_enumerator() const override {
            return list.get_enumerator();
        }

        ~ListSequence() {}
};

template <class T, class Derived, bool Mutable, template <class> class Base>
class SequenceCRTP : public Base<T> {
    protected:
        Base<T>* Instance() override {
            if constexpr (Mutable) {
                return this;
            } else {
                return new Derived(static_cast<const Derived&>(*this));
            }
        }

        Base<T>* CreateEmpty() const override {
            return new Derived();
        }
    public:
        using Base<T>::Base;

        Derived* Clone() const {
            return new Derived(static_cast<const Derived&>(*this));
        }

        Derived* Empty() const {
            return new Derived();
        }

        Derived* map_defined(T (*func)(const T&)) const {
            return static_cast<Derived*>(this->map(func));
        }
        
        Derived* where_defined(bool (*predicate)(const T&)) const {
            return static_cast<Derived*>(this->where(predicate));
        }

        Derived* concat_defined(const Sequence<T>* other) const {
            return static_cast<Derived*>(this->concat(other));
        }

        Derived* get_sub_sequence_defined(int start, int end) const {
            return static_cast<Derived*>(this->get_sub_sequence(start, end));
        }

        Derived* slice_defined(int index, int count, const Sequence<T>* replace_seq = nullptr) const {
            return static_cast<Derived*>(this->slice(index, count, replace_seq));
        }

        Derived& chain_append(const T& item) {
            static_assert(Mutable, "chain_append is only available for Mutable sequences"); // Работает на этапе компиляции, поэтому используем его
            this->append(item);
            return *static_cast<Derived*>(this);
        }

        Derived& chain_prepend(const T& item) {
            static_assert(Mutable, "chain_prepend is only available for Mutable sequences");
            this->prepend(item);
            return *static_cast<Derived*>(this);
        }

        Derived& chain_insert_at(const T& item, int index) {
            static_assert(Mutable, "chain_insert_at is only available for Mutable sequences");
            this->insert_at(item, index);
            return *static_cast<Derived*>(this);
        }
};

template <class T>
class MutableArraySequence : public SequenceCRTP<T, MutableArraySequence<T>, true, ArraySequence> {
    public:
        using SequenceCRTP<T, MutableArraySequence<T>, true, ArraySequence>::SequenceCRTP;
};

template <class T>
class ImmutableArraySequence : public SequenceCRTP<T, ImmutableArraySequence<T>, false, ArraySequence> {
    public:
        using SequenceCRTP<T, ImmutableArraySequence<T>, false, ArraySequence>::SequenceCRTP;
};

template <class T>
class MutableListSequence : public SequenceCRTP<T, MutableListSequence<T>, true, ListSequence> {
    public:
        using SequenceCRTP<T, MutableListSequence<T>, true, ListSequence>::SequenceCRTP;
};

template <class T>
class ImmutableListSequence : public SequenceCRTP<T, ImmutableListSequence<T>, false, ListSequence> {
    public:
        using SequenceCRTP<T, ImmutableListSequence<T>, false, ListSequence>::SequenceCRTP;
};

#include "sequence.tpp"

#endif