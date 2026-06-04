#ifndef ALLOC_STRATEGY_H
#define ALLOC_STRATEGY_H

#include "memory/memory_tape.h"

// Общий метод, который находит первый свободный фрагмент длиной >= size в [from, to)
inline int find_free_run(const MemoryTape& tape, int size, int from, int to) {
    int index = from;
    while (index < to) {
        if (tape.get_cell(index).used) {
            index++;
            continue;
        }

        int run_end = index;
        while (run_end < to && !tape.get_cell(run_end).used) {
            run_end++;
        }

        if (run_end - index >= size) return index;
        index = run_end;
    }
    return -1;
}

class AllocStrategy {
    public:
        virtual int select(const MemoryTape& tape, int size) = 0;
        virtual void reset() {}

        virtual ~AllocStrategy() = default;
};

// FirstFit - первый подходящий фрагмент от начала ленты
class FirstFitStrategy : public AllocStrategy {
    public:
        int select(const MemoryTape& tape, int size) override {
            return find_free_run(tape, size, 0, tape.get_capacity());
        }
};

// BestFit - минимальный по длине подходящий фрагмент
class BestFitStrategy : public AllocStrategy {
    public:
        int select(const MemoryTape& tape, int size) override {
            int capacity = tape.get_capacity();
            int best_start = -1;
            int best_length = -1;

            int index = 0;
            while (index < capacity) {
                if (tape.get_cell(index).used) {
                    index++;
                    continue;
                }

                int run_end = index;
                while (run_end < capacity && !tape.get_cell(run_end).used) {
                    run_end++;
                }

                int run_length = run_end - index;
                if (run_length >= size && (best_length < 0 || run_length < best_length)) {
                    best_length = run_length;
                    best_start = index;
                }

                index = run_end;
            }
            return best_start;
        }
};

// WorstFit - максимальный по длине фрагмент
class WorstFitStrategy : public AllocStrategy {
    public:
        int select(const MemoryTape& tape, int size) override {
            int capacity = tape.get_capacity();
            int best_start = -1;
            int best_length = -1;

            int index = 0;
            while (index < capacity) {
                if (tape.get_cell(index).used) {
                    index++;
                    continue;
                }

                int run_end = index;
                while (run_end < capacity && !tape.get_cell(run_end).used) {
                    run_end++;
                }

                int run_length = run_end - index;
                if (run_length >= size && run_length > best_length) {
                    best_length = run_length;
                    best_start = index;
                }

                index = run_end;
            }
            return best_start;
        }
};

// NextFit - FirstFit, но начинается с места прошлой выдачи
class NextFitStrategy : public AllocStrategy {
    private:
        int last_pos; // Индекс, с которого надо начинать следующий поиск
    public:
        NextFitStrategy() : last_pos(0) {}

        int select(const MemoryTape& tape, int size) override {
            int capacity = tape.get_capacity();
            if (last_pos >= capacity) last_pos = 0;

            int start = find_free_run(tape, size, last_pos, capacity);

            if (start < 0) {
                start = find_free_run(tape, size, 0, last_pos);
            }

            if (start < 0) return -1;

            last_pos = start + size;
            if (last_pos >= capacity) last_pos = 0;

            return start;
        }

        void reset() override { last_pos = 0; }
};

#endif
