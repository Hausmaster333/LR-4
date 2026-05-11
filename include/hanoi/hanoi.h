#ifndef HANOI_H
#define HANOI_H

#include "hanoi/hanoi_ring.h"
#include "deque/segment_deque.h"

struct HanoiMove {
    int from_stick_index;
    int to_stick_index;
    int ring_size;
    char ring_color[64];
};

#include "hanoi_vis.h"

static void hanoi_recursive(int count, // Количество колец
                            MutableSegmentedDeque<Ring>& from, int from_id, // Откуда перемещаем
                            MutableSegmentedDeque<Ring>& to, int to_id, // Куда перемещаем
                            MutableSegmentedDeque<Ring>& aux, int aux_id,
                            MutableSegmentedDeque<HanoiMove>& moves)  { // Вспомогательный стержень
    
    Ring elem;

    if (count == 1) {
        from.pop_back(&elem);
        to.push_back(elem);

        HanoiMove move;
        move.from_stick_index = from_id;
        move.to_stick_index = to_id;
        move.ring_size = elem.get_size();
        strcpy(move.ring_color, elem.get_color());
        moves.push_back(move);

        return;
    }

    hanoi_recursive(count - 1, from, from_id, aux, aux_id, to, to_id, moves); // Перемещаем n - 1 колец на вспомогательный стержень

    from.pop_back(&elem); // Самое большое перемещаем
    to.push_back(elem);

    HanoiMove move;
    move.from_stick_index = from_id;
    move.to_stick_index = to_id;
    move.ring_size = elem.get_size();
    strcpy(move.ring_color, elem.get_color());
    moves.push_back(move);
        
    hanoi_recursive(count - 1, aux, aux_id, to, to_id, from, from_id, moves); // Перемещаем n - 1 колец на целевой стержень
};

static bool ring_compare(const Ring& a, const Ring& b) {
    return a < b;  // Большие кольца первыми
}

static MutableSegmentedDeque<HanoiMove> hanoi_collect_moves(const MutableSegmentedDeque<Ring>& rings, int start_stick, int target_stick) {
    if (start_stick < 0 || start_stick > 2 || target_stick < 0 || target_stick > 2 || target_stick == start_stick) {
        throw std::out_of_range("Invalid sticks");
    }
    
    MutableSegmentedDeque<HanoiMove> moves;
    MutableSegmentedDeque<Ring> sticks[3];
    sticks[start_stick] = rings;
    
    int aux = 3 - start_stick - target_stick;
    sticks[start_stick].sort(ring_compare);
    
    hanoi_recursive(sticks[start_stick].get_count(),
                    sticks[start_stick], start_stick,
                    sticks[target_stick], target_stick,
                    sticks[aux], aux,
                    moves);
    
    return moves;
}

void hanoi(MutableSegmentedDeque<Ring>& rings, int start_stick, int target_stick) { // Список предметов в виде дека + номер начального стержня -> создает 3 дека-стержня и кладет кольца на нужный
    auto moves = hanoi_collect_moves(rings, start_stick, target_stick);
    generate_hanoi_html(moves, rings, start_stick);
};

#endif