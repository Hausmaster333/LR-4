#ifndef SEGMENT_DEQUE_H
#define SEGMENT_DEQUE_H

#include "core/sequence.h"

template <class T>
class SegmentedDeque: public Sequence<T> {
    template <class U>
    friend void render_deque(const SegmentedDeque<U>& deque);
    
    template <class U>
    friend size_t measure_deque_memory(const SegmentedDeque<U>& deque);
    private:
        void sys_push_front(const T& item); // Всегда меняют this вне зависимости от Mutable/Immutable
        void sys_push_back(const T& item);

        T sys_pop_front(); 
        T sys_pop_back();
    protected:
        DynamicArray<T*> block_map; // Динамический массив указателей на T
        int map_capacity;
        int front_block;
        int front_index; // Позиция первого элемента
        int back_block;
        int back_index; // Позиция за последним элементом
        int count;
        static const int segment_size = 8; // static - одно значение на класс, а не на объект. const - нельзя изменить
        
        T* allocate_block();
        void grow_map_front();
        void grow_map_back();

        void shrink_map();

        void resolve_index(int index, int* block, int* offset) const; // Из index получает пару block + offset(смещение внутри block)
        
        void sys_append(const T& item) override;

        virtual SegmentedDeque<T>* Instance() = 0;
        virtual SegmentedDeque<T>* CreateEmpty() const = 0;        
    public:
        SegmentedDeque();
        SegmentedDeque(const T* items, int count);
        SegmentedDeque(const SegmentedDeque<T>& other);

        SegmentedDeque<T>* push_front(const T& item);
        SegmentedDeque<T>* push_back(const T& item);
        SegmentedDeque<T>* pop_front(T* result);
        SegmentedDeque<T>* pop_back(T* result);

        SegmentedDeque<T>& operator=(const SegmentedDeque<T>& other);

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

        SegmentedDeque<T>* merge(const SegmentedDeque<T>* other, bool (*compare)(const T& a, const T& b) = nullptr); // Если nullptr, то сортируем через <
        void sort(bool (*compare)(const T& a, const T& b) = nullptr);
        int find_sub_sequence(const Sequence<T>* sub_seq) const;

        class Enumerator: public IEnumerator<T> {
            private:
                const SegmentedDeque<T>* deque;
                int index;
            public:
                Enumerator(const SegmentedDeque<T>* deque) : deque(deque), index(-1) {}

                bool move_next() override {
                    index++;
                    return index < deque->count;
                }

                const T& get_current() const override {
                    return deque->get(index);
                }

                void reset() override {
                    index = -1;
                }
        };

        IEnumerator<T>* get_enumerator() const override {
            return new Enumerator(this);
        }

        void reset_deque();

        ~SegmentedDeque() override;
};

template <class T>
class MutableSegmentedDeque : public SequenceCRTP<T, MutableSegmentedDeque<T>, true, SegmentedDeque> {
    public:
        using SequenceCRTP<T, MutableSegmentedDeque<T>, true, SegmentedDeque>::SequenceCRTP;
};

template <class T>
class ImmutableSegmentedDeque : public SequenceCRTP<T, ImmutableSegmentedDeque<T>, false, SegmentedDeque> {
    public:
        using SequenceCRTP<T, ImmutableSegmentedDeque<T>, false, SegmentedDeque>::SequenceCRTP;
};

#include "segment_deque.tpp"

#endif