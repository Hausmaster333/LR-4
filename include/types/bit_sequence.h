#ifndef BIT_SEQUENCE_H
#define BIT_SEQUENCE_H

#include "core/sequence.h"
#include "types/bit.h"

class BitSequence : public Sequence<Bit> {
    private:
        DynamicArray<unsigned char> data;
        int bit_count;
        mutable Bit cached_bit; // Чтобы могли возвращаться const Bit&, т.к. биты упакованы по 8 бит в байт

        bool get_bit(int index) const;
        void set_bit(int index, bool value);
        void clear_unused_bits();

        static int check_bytes_needed(int n); // Количество байт, необходимое для хранения n бит
    protected:
        void sys_append(const Bit& item);
        
        Sequence<Bit>* CreateEmpty() const;
    public:
        BitSequence();
        BitSequence(const Bit* items, int count);
        BitSequence(const BitSequence& other);

        const Bit& get_first() const override;
        const Bit& get_last() const override;
        const Bit& get(int index) const;

        Option<Bit> try_get_first() const override;
        Option<Bit> try_get_last() const override;
        Option<Bit> try_get(int index) const;

        int get_count() const override;

        // Sequence<Bit>* get_sub_sequence(int start, int end) override;

        Sequence<Bit>* append(const Bit& item) override;
        Sequence<Bit>* prepend(const Bit& item) override;
        Sequence<Bit>* insert_at(const Bit& item, int index) override;

        BitSequence* bit_and(const BitSequence& other) const; // И
        BitSequence* bit_or(const BitSequence& other) const; // ИЛИ
        BitSequence* bit_xor(const BitSequence& other) const; // XOR
        BitSequence* bit_not() const; // НЕ

        class Enumerator : public IEnumerator<Bit> {
        private:
            const BitSequence* bit_seq;
            int index;
            mutable Bit current;
        public:
            Enumerator(const BitSequence* bit_seq) : bit_seq(bit_seq), index(-1), current(false) {}

            bool move_next() override {
                index++;
                return index < bit_seq->bit_count;
            }

            const Bit& get_current() const override {
                current = Bit(bit_seq->get_bit(index));
                return current;
            }

            void reset() override {
                index = -1;
            }
        };

        IEnumerator<Bit>* get_enumerator() const override {
            return new Enumerator(this);
        }

        ~BitSequence() override {};
};


#endif