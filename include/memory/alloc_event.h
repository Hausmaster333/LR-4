#ifndef ALLOC_EVENT_H
#define ALLOC_EVENT_H

// Стратегия размещения: First-fit / Best-fit / Worst-fit
enum class AllocStrategy {
    FirstFit = 0,
    BestFit = 1,
    WorstFit = 2,
};

// Вид события: Alloc(size) или Free(block_id)
enum class AllocEventKind : int {
    Alloc = 0,
    Free = 1,
};

// Событие на потоке: тип + один int-payload
// Alloc: payload = size (>= 1)
// Free:  payload = block_id (>= 0)
struct AllocEvent {
    AllocEventKind kind;
    int payload;

    static AllocEvent make_alloc(int size) {
        return {AllocEventKind::Alloc, size};
    }
    static AllocEvent make_free(int block_id) {
        return {AllocEventKind::Free, block_id};
    }
};

#endif
