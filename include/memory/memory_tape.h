#ifndef MEMORY_TAPE_H
#define MEMORY_TAPE_H

#include "core/dynamic_array.h"
#include "memory/alloc_event.h"
#include <stdexcept>

// Одна ячейка ленты памяти
// Если used == false, ячейка свободна, block_id == -1
struct Cell {
    bool used;
    int block_id;
};

// Мутабельная «лента памяти» фиксированной длины
// alloc(size, strategy) ищет run свободных ячеек по выбранной стратегии,
// помечает их как занятые с уникальным block_id
// free(block_id) освобождает все ячейки данного блока
// Контейнер ячеек — DynamicArray<Cell>
class MemoryTape {
    private:
        DynamicArray<Cell> cells;
        int capacity;
        int next_block_id;
        int used_cells_count;
        int blocks_count;

        // Ищет позицию начала run-а длины >= size по стратегии
        // Возвращает -1 если подходящего run-а нет
        // FirstFit - первый найденный, BestFit - минимальный по длине,
        // WorstFit - максимальный по длине
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
                    if (strategy == AllocStrategy::FirstFit) {
                        return index;
                    }
                    if (strategy == AllocStrategy::BestFit) {
                        if (best_length < 0 || run_length < best_length) {
                            best_length = run_length;
                            best_start = index;
                        }
                    } else { // WorstFit
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

        // Проверяет, что capacity > 0, иначе throw. Возвращает capacity как есть
        // Нужен до конструктора DynamicArray, чтобы заранее отбросить плохой вход
        static int validated_capacity(int capacity) {
            if (capacity <= 0) throw std::invalid_argument("MemoryTape: capacity must be > 0");

            return capacity;
        }
    public:
        // Создаёт пустую ленту длины capacity. capacity > 0 иначе throw
        explicit MemoryTape(int capacity)
            : cells(validated_capacity(capacity)),
              capacity(capacity),
              next_block_id(0),
              used_cells_count(0),
              blocks_count(0) {
            for (int index = 0; index < capacity; index++) {
                cells.set(index, {false, -1});
            }
        }

        int get_capacity() const { return capacity; }
        int get_used_count() const { return used_cells_count; }
        int get_free_count() const { return capacity - used_cells_count; }
        int get_blocks_count() const { return blocks_count; }
        const Cell& get_cell(int index) const { return cells.get(index); }

        // Выделяет блок размера size по стратегии strategy
        // Возвращает уникальный block_id (>= 0) или -1 если не нашлось места
        // next_block_id растёт монотонно и сбрасывается только через reset()
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

            return new_id;
        }

        // Освобождает все ячейки с этим block_id. Возвращает true если найден
        // Сканирует все capacity ячеек один раз - O(capacity)
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
            }

            return found;
        }

        // Полный сброс: все ячейки свободны, next_block_id = 0
        void reset() {
            for (int index = 0; index < capacity; index++) cells.set(index, {false, -1});
            next_block_id = 0;
            used_cells_count = 0;
            blocks_count = 0;
        }

        // Коэффициент фрагментации = 1 - largest_free_run / total_free
        // 0 если total_free == 0 (полностью занятая лента не считается фрагментированной)
        // Чем ближе к 1, тем хуже: много мелких дыр и плохо для alloc(big_size)
        double fragmentation() const {
            int free_total = capacity - used_cells_count;
            if (free_total == 0) return 0.0;

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

            return 1.0 - static_cast<double>(max_run) / static_cast<double>(free_total);
        }
};

#endif
