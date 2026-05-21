#ifndef MEMORY_TAPE_H
#define MEMORY_TAPE_H

#include "core/dynamic_array.h"
#include "memory/alloc_event.h"
#include <stdexcept>

struct Cell {
    bool used; // Если used == false, ячейка свободна, block_id == -1
    int block_id;
};

class MemoryTape {
    private:
        DynamicArray<Cell> cells; // Хранилище ячеек
        int capacity;
        int next_block_id;
        int used_cells_count;
        int blocks_count;

        // Кэш фрагментации
        mutable bool frag_cached;
        mutable double frag_value;

        void invalidate_frag() { frag_cached = false; }

        int find_run(int size, AllocStrategy strategy) const {
            int best_start = -1;
            int best_length = -1;

            int index = 0;
            while (index < capacity) {
                if (cells.get(index).used) {
                    index++;
                    continue;
                }
                int run_end = index;
                while (run_end < capacity && !cells.get(run_end).used) run_end++;
                int run_length = run_end - index;

                if (run_length >= size) {
                    if (strategy == AllocStrategy::FirstFit) { // FirstFit - первый найденный
                        return index;
                    }
                    if (strategy == AllocStrategy::BestFit) { // BestFit - минимальный по длине
                        if (best_length < 0 || run_length < best_length) {
                            best_length = run_length;
                            best_start = index;
                        }
                    } else { // WorstFit - максимальный по длине
                        if (best_length < 0 || run_length > best_length) {
                            best_length = run_length;
                            best_start = index;
                        }
                    }
                }
                index = run_end;
            }

            return best_start;
        }

        static int validated_capacity(int capacity) {
            if (capacity <= 0) throw std::invalid_argument("MemoryTape: capacity must be > 0");

            return capacity;
        }
    public:
        MemoryTape(int capacity)
            : cells(validated_capacity(capacity)),
              capacity(capacity),
              next_block_id(0),
              used_cells_count(0),
              blocks_count(0),
              frag_cached(false),
              frag_value(0.0) {
            for (int index = 0; index < capacity; index++) {
                cells.set(index, {false, -1});
            }
        }

        int get_capacity() const { return capacity; }
        int get_used_count() const { return used_cells_count; }
        int get_free_count() const { return capacity - used_cells_count; }
        int get_blocks_count() const { return blocks_count; }
        const Cell& get_cell(int index) const { return cells.get(index); }

        // Выделяет блок размера size по 1 из 3 стратегий
        int alloc(int size, AllocStrategy strategy) {
            if (size <= 0 || size > capacity) return -1;
            int start = find_run(size, strategy);
            if (start < 0) return -1;

            int new_id = next_block_id++;
            for (int offset = 0; offset < size; offset++) {
                cells.set(start + offset, {true, new_id});
            }
            used_cells_count += size;
            blocks_count += 1;
            invalidate_frag();

            return new_id;
        }

        // Освобождает все ячейки с этим block_id
        bool free(int block_id) {
            bool found = false;
            int freed_count = 0;
            for (int index = 0; index < capacity; index++) {
                const Cell& cell = cells.get(index);
                if (cell.used && cell.block_id == block_id) {
                    cells.set(index, {false, -1});
                    found = true;
                    freed_count++;
                }
            }
            if (found) {
                used_cells_count -= freed_count;
                blocks_count -= 1;
                invalidate_frag();
            }

            return found;
        }

        void reset() {
            for (int index = 0; index < capacity; index++) cells.set(index, {false, -1});
            next_block_id = 0;
            used_cells_count = 0;
            blocks_count = 0;
            invalidate_frag();
        }

        // Коэффициент фрагментации = 1 - largest_free_run / total_free
        double fragmentation() const {
            if (frag_cached) return frag_value;

            int free_total = capacity - used_cells_count;
            if (free_total == 0) {
                frag_value = 0.0;
                frag_cached = true;
                return 0.0;
            }

            int max_run = 0;
            int index = 0;
            while (index < capacity) {
                if (cells.get(index).used) {
                    index++;
                    continue;
                }
                int run_end = index;
                while (run_end < capacity && !cells.get(run_end).used) run_end++;
                if (run_end - index > max_run) max_run = run_end - index;
                index = run_end;
            }

            frag_value = 1.0 - static_cast<double>(max_run) / static_cast<double>(free_total);
            frag_cached = true;
            return frag_value;
        }
};

#endif
