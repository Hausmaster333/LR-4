#ifndef SORTING_STATION_H
#define SORTING_STATION_H

#include "tuples.h"
#include "deque/segment_deque.h"

using Wagon = Pair<int, int>;

enum class StationAction {
    PopFromTrain,         // Вагон отцеплен с переднего края состава
    PushToSiding,         // Вагон поставлен в тупик
    PushToResult,         // Вагон сразу в результат (direct type)
    MoveSidingToResult    // Вагон переехал из тупика в результат
};

struct StationStep {
    StationAction action;
    Wagon wagon;
    int siding_index; // индекс тупика; для PushToResult/PopFromTrain не используется
};

MutableSegmentedDeque<int> collect_types(const MutableSegmentedDeque<Wagon>& train);

int count_wagon_types(const MutableSegmentedDeque<Wagon>& train);

MutableSegmentedDeque<Wagon> sort_train_by_type(const MutableSegmentedDeque<Wagon>& train, MutableSegmentedDeque<StationStep>* steps = nullptr);

#endif
