#ifndef MEMORY_TAPE_H
#define MEMORY_TAPE_H

#include "core/dynamic_array.h"
#include <stdexcept>

struct Cell {
    bool used;
    int block_id;
};

class MemoryTape {
    private:
        DynamicArray<Cell> cells; // Хранилище ячеек
        int capacity;
        int next_block_id;
        int used_cells_count;
        int blocks_count;

        mutable bool frag_cached;
        mutable double frag_value;

        void invalidate_frag() { frag_cached = false; }

        static int validated_capacity(int capacity) {
            if (capacity <= 0) throw std::invalid_argument("MemoryTape: capacity must be > 0");

            return capacity;
        }
    public:
        MemoryTape(int capacity) : cells(validated_capacity(capacity)), capacity(capacity), next_block_id(0), used_cells_count(0), 
                                   blocks_count(0), frag_cached(false), frag_value(0.0) {
            for (int index = 0; index < capacity; index++) {
                cells.set(index, {false, -1});
            }
        }

        int get_capacity() const { return capacity; }
        int get_used_count() const { return used_cells_count; }
        int get_free_count() const { return capacity - used_cells_count; }
        int get_blocks_count() const { return blocks_count; }
        const Cell& get_cell(int index) const { return cells.get(index); }

        template <class Strategy>
        int alloc(int size, Strategy& strategy) {
            if (size <= 0 || size > capacity) return -1;

            int start = strategy.select(*this, size);
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
            for (int index = 0; index < capacity; index++) {
                cells.set(index, {false, -1});
            }

            next_block_id = 0;
            used_cells_count = 0;
            blocks_count = 0;
            invalidate_frag();
        }

        // Максимальное уплотнение: занятые блоки сдвигаются в начало ленты, новые идут первыми
        void compact() {
            int cap = (blocks_count > 0) ? blocks_count : 1;
            int* live_ids = new int[cap];   // id существующих блоков
            int* sizes = new int[cap];      // Размеры блоков
            int live_count = 0;

            // Собираем id и размеры до перезаписи
            for (int index = 0; index < capacity; index++) {
                const Cell& cell = cells.get(index);
                if (!cell.used) continue;

                int found = -1;
                for (int existing = 0; existing < live_count; existing++) {
                    if (live_ids[existing] == cell.block_id) {
                        found = existing;
                        break;
                    }
                }

                if (found < 0) {
                    live_ids[live_count] = cell.block_id;
                    sizes[live_count] = 1;
                    live_count++;
                } else {
                    sizes[found]++;
                }
            }

            // Сортируем по убыванию block_id через insertion sort
            for (int boundary = 1; boundary < live_count; boundary++) {
                int id_insert = live_ids[boundary];
                int size_insert = sizes[boundary];
                int slot = boundary - 1;

                while (slot >= 0 && live_ids[slot] < id_insert) {
                    live_ids[slot + 1] = live_ids[slot];
                    sizes[slot + 1] = sizes[slot];
                    slot--;
                }

                live_ids[slot + 1] = id_insert;
                sizes[slot + 1] = size_insert;
            }

            // Перезаписываем ленту
            int write_pos = 0;
            for (int block_idx = 0; block_idx < live_count; block_idx++) {
                for (int offset = 0; offset < sizes[block_idx]; offset++) {
                    cells.set(write_pos + offset, {true, live_ids[block_idx]});
                }
                write_pos += sizes[block_idx];
            }

            for (int index = write_pos; index < capacity; index++) {
                cells.set(index, {false, -1});
            }

            delete[] live_ids;
            delete[] sizes;
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
                while (run_end < capacity && !cells.get(run_end).used) {
                    run_end++;
                }

                if (run_end - index > max_run) {
                    max_run = run_end - index;
                }

                index = run_end;
            }

            frag_value = 1.0 - static_cast<double>(max_run) / static_cast<double>(free_total);
            frag_cached = true;
            return frag_value;
        }
};

#endif
