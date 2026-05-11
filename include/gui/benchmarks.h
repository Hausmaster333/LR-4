#ifndef BENCHMARKS_H
#define BENCHMARKS_H

#include "deque/segment_deque.h"
#include "imgui.h"
#include <chrono>
#include <cstdlib>
#include <random>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #include <malloc.h>
#else
    #include <cstdio>
    #include <unistd.h>
    #include <malloc.h>
#endif

struct BenchmarkParams {
    const int* sizes;
    int sizes_count; // Сколько элементов в массиве sizes(читай количество прогонов)
    int get_accesses; // Сколько случайных обращений get(idx) делать для каждого size
    size_t os_memory_warmup_bytes; // Прогреваем аллокатор(выделяем и освобождаем большой кусок памяти)
    int os_memory_pause_ms; // Задержка, чтобы ОС успела обновить статистику памяти
};

size_t get_process_memory() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;  // байты физической памяти
    }
    return 0;
#else
    // /proc/self/statm: размеры в страницах, поля: size resident shared text lib data dt
    FILE* f = std::fopen("/proc/self/statm", "r");
    if (!f) return 0;
    long total_pages = 0, rss_pages = 0;
    if (std::fscanf(f, "%ld %ld", &total_pages, &rss_pages) != 2) {
        std::fclose(f);
        return 0;
    }
    std::fclose(f);
    long page_size = sysconf(_SC_PAGESIZE);  // В среднем обычно 4096
    return (size_t)rss_pages * (size_t)page_size;  // Байты физической памяти
#endif
}

// Просим аллокатор вернуть свободные страницы ОС
void heap_trim() {
#ifdef _WIN32
    _heapmin();
#else
    malloc_trim(0);
#endif
}

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

template <class U>
size_t measure_deque_memory(const SegmentedDeque<U>& deque) {
    size_t size = sizeof(SegmentedDeque<U>);
    size += deque.map_capacity * sizeof(U*);
    
    int allocated = 0;
    for (int idx = 0; idx < deque.map_capacity; idx++) {
        if (deque.block_map.get(idx) != nullptr) allocated++;
    }
    size += allocated * deque.segment_size * sizeof(U);
    
    return size;
};

template <class U>
size_t measure_array_seq_memory(const ArraySequence<U>& dyn_array) {
    size_t size = sizeof(ArraySequence<U>);
    size += dyn_array.array.get_size() * sizeof(U); // Данные внутри массива

    return size;
};

template <class U>
size_t measure_list_seq_memory(const ListSequence<U>& linked_list) {
    size_t size = sizeof(ListSequence<U>);
    
    int count = linked_list.list.get_length();
    size += count * linked_list.list.get_node_size();
    
    return size;
};

void run_push_front_benchmark(double* deque_times, double* array_times, double* list_times, const BenchmarkParams& params) {
    for (int number = 0; number < params.sizes_count; number++) {
        int size = params.sizes[number];

        MutableSegmentedDeque<int> deque;
        auto start_deque = std::chrono::high_resolution_clock::now();

        for (int idx = 0; idx < size; idx++) {
            deque.push_front(idx);
        }

        auto finish_deque = std::chrono::high_resolution_clock::now();
        deque_times[number] = std::chrono::duration<double, std::milli>(finish_deque - start_deque).count();
        
        MutableArraySequence<int> array;
        auto start_array = std::chrono::high_resolution_clock::now();

        for (int idx = 0; idx < size; idx++) {
            array.prepend(idx);
        }

        auto finish_array = std::chrono::high_resolution_clock::now();
        array_times[number] = std::chrono::duration<double, std::milli>(finish_array - start_array).count();

        MutableListSequence<int> list;
        auto start_list = std::chrono::high_resolution_clock::now();

        for (int idx = 0; idx < size; idx++) {
            list.prepend(idx);
        }

        auto finish_list = std::chrono::high_resolution_clock::now();
        list_times[number] = std::chrono::duration<double, std::milli>(finish_list - start_list).count();
    }
};

void run_get_benchmark(double* deque_times, double* array_times, double* list_times, const BenchmarkParams& params) {
    std::mt19937 rng(42); // Генератор с фиксированным seed для воспроизводимости
 
    for (int number = 0; number < params.sizes_count; number++) {
        int size = params.sizes[number];
        if (size <= 0 || params.get_accesses <= 0) {
            deque_times[number] = 0;
            array_times[number] = 0;
            list_times[number] = 0;
            continue;
        }
        std::uniform_int_distribution<int> dist(0, size - 1);
 
        // Подготовка: заполняем структуры (это НЕ замеряем)
        MutableSegmentedDeque<int> deque;
        MutableArraySequence<int> array;
        MutableListSequence<int> list;
        for (int idx = 0; idx < size; idx++) {
            deque.push_back(idx);
            array.append(idx);
            list.append(idx);
        }

        // Генерируем случайные индексы заранее, чтобы все структуры обращались к одинаковым
        int* indices = new int[params.get_accesses];
        for (int idx = 0; idx < params.get_accesses; idx++) {
            indices[idx] = dist(rng);
        }
 
        // Замер SegmentedDeque
        auto t1 = std::chrono::high_resolution_clock::now();
        volatile int sum1 = 0;
        for (int idx = 0; idx < params.get_accesses; idx++) {
            sum1 += deque.get(indices[idx]);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        deque_times[number] = std::chrono::duration<double, std::milli>(t2 - t1).count();
 
        // Замер ArraySequence
        t1 = std::chrono::high_resolution_clock::now();
        volatile int sum2 = 0;
        for (int idx = 0; idx < params.get_accesses; idx++) {
            sum2 += array.get(indices[idx]);
        }
        t2 = std::chrono::high_resolution_clock::now();
        array_times[number] = std::chrono::duration<double, std::milli>(t2 - t1).count();
 
        // Замер ListSequence
        t1 = std::chrono::high_resolution_clock::now();
        volatile int sum3 = 0;

        EnumeratorWrapper<int> iter(list.get_enumerator());

        for (int idx = 0; idx < params.get_accesses; idx++) {
            int target_index = indices[idx];

            iter.reset();

            for (int curr_index = 0; curr_index <= target_index; curr_index++) {
                iter.move_next();
            }

            sum3 += iter.get_current();
        }

        t2 = std::chrono::high_resolution_clock::now();
        list_times[number] = std::chrono::duration<double, std::milli>(t2 - t1).count();
 
        delete[] indices;
    }
}

void run_memory_benchmark(double* deque_mem, double* array_mem, double* list_mem, const BenchmarkParams& params) {
    for (int number = 0; number < params.sizes_count; number++) {
        int size = params.sizes[number];

        // Создаём дек, заполняем, измеряем
        MutableSegmentedDeque<int> deque;
        for (int idx = 0; idx < size; idx++) {
            deque.push_back(idx);
        }
        deque_mem[number] = (double)measure_deque_memory(deque) / 1024.0; // в КБ

        // ArraySequence
        MutableArraySequence<int> array;
        for (int idx = 0; idx < size; idx++) {
            array.append(idx);
        }
        array_mem[number] = (double)measure_array_seq_memory(array) / 1024.0;

        // ListSequence
        MutableListSequence<int> list;
        for (int idx = 0; idx < size; idx++) {
            list.append(idx);
        }
        list_mem[number] = (double)measure_list_seq_memory(list) / 1024.0;
    }
}

void run_os_memory_benchmark(double* deque_mem, double* array_mem, double* list_mem, const BenchmarkParams& params) {
    // Выделяем и освобождаем большой буфер
    void* warmup = malloc(params.os_memory_warmup_bytes);
    if (warmup) free(warmup);
    heap_trim();  // Просим аллокатор вернуть свободные страницы ОС

    for (int number = 0; number < params.sizes_count; number++) {
        int size = params.sizes[number];
        
        heap_trim();
        sleep_ms(params.os_memory_pause_ms);  //  ОС обновляет статистику
        size_t before = get_process_memory();
        {
            MutableSegmentedDeque<int> deque;
            for (int idx = 0; idx < size; idx++) {
                deque.push_back(idx);
            }
            size_t after = get_process_memory();
            deque_mem[number] = (double)(after - before) / 1024.0;
        }
        
        heap_trim();
        sleep_ms(params.os_memory_pause_ms);
        before = get_process_memory();
        {
            MutableArraySequence<int> arr;
            for (int idx = 0; idx < size; idx++) {
                arr.append(idx);
            }
            size_t after = get_process_memory();
            array_mem[number] = (double)(after - before) / 1024.0;
        }
        
        heap_trim();
        sleep_ms(params.os_memory_pause_ms);
        before = get_process_memory();
        {
            MutableListSequence<int> list;
            for (int idx = 0; idx < size; idx++) {
                list.append(idx);
            }
            size_t after = get_process_memory();
            list_mem[number] = (double)(after - before) / 1024.0;
        }
    }
}

void run_benchmarks(double* deque_push_front_times, double* array_push_front_times, double* list_push_front_times,
                    double* deque_get_times, double* array_get_times, double* list_get_times,
                    double* deque_mem, double* array_mem, double* list_mem,
                    double* deque_os_mem, double* array_os_mem, double* list_os_mem,
                    const BenchmarkParams& params) {

    run_push_front_benchmark(deque_push_front_times, array_push_front_times, list_push_front_times, params);
    run_get_benchmark(deque_get_times, array_get_times, list_get_times, params);
    run_memory_benchmark(deque_mem, array_mem, list_mem, params);
    run_os_memory_benchmark(deque_os_mem, array_os_mem, list_os_mem, params);
};

#endif
