#ifndef ALLOC_EVENT_H
#define ALLOC_EVENT_H

enum class AllocStrategy {
    FirstFit = 0,
    BestFit = 1,
    WorstFit = 2,
};

enum class AllocEventKind : int {
    Alloc = 0,
    Free = 1,
};

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
