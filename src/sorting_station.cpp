#include "sorting_station/sorting_station.h"

static int find_type_index(const MutableSegmentedDeque<int>& types, int type) {
    for (int idx = 0; idx < types.get_count(); idx++) {
        if (types.get(idx) == type) {
            return idx;
        }
    }

    return -1;
}

MutableSegmentedDeque<int> collect_types(const MutableSegmentedDeque<Wagon>& train) {
    MutableSegmentedDeque<int> types;

    for (int idx = 0; idx < train.get_count(); idx++) {
        int type = train.get(idx).first();

        if (find_type_index(types, type) == -1) {
            types.push_back(type);
        }
    }

    return types;
}

int count_wagon_types(const MutableSegmentedDeque<Wagon>& train) {
    MutableSegmentedDeque<int> types = collect_types(train);
    
    return types.get_count();
}

MutableSegmentedDeque<Wagon> sort_train_by_type(const MutableSegmentedDeque<Wagon>& train, MutableSegmentedDeque<StationStep>* steps) {
    if (train.get_count() == 0) return MutableSegmentedDeque<Wagon>();
    
    MutableSegmentedDeque<int> types = collect_types(train);
    int siding_count = types.get_count();
    int direct_type = train.get(0).first();  // тип идущий напрямую

    MutableSegmentedDeque<Wagon>* sidings = new MutableSegmentedDeque<Wagon>[siding_count];

    MutableSegmentedDeque<Wagon> work_train(train);
    MutableSegmentedDeque<Wagon> result;
    Wagon wagon;

    while (work_train.get_count() > 0) {
        work_train.pop_front(&wagon);
        if (steps) {
            steps->push_back({StationAction::PopFromTrain, wagon, -1});
        }

        int type = wagon.first();
        
        if (type == direct_type) {
            result.push_back(wagon);

            if (steps) {
                steps->push_back({StationAction::PushToResult, wagon, -1});\
            }
        } else {
            int siding_index = find_type_index(types, type);
            sidings[siding_index].push_back(wagon);

            if (steps) {
                steps->push_back({StationAction::PushToSiding, wagon, siding_index});
            }
        }
    }

    for (int idx = 0; idx < siding_count; idx++) {
        if (types.get(idx) == direct_type) {
            continue;
        }

        while (sidings[idx].get_count() > 0) {
            sidings[idx].pop_front(&wagon);
            result.push_back(wagon);
            if (steps) {
                steps->push_back({StationAction::MoveSidingToResult, wagon, idx});
            }
        }
    }

    delete[] sidings;

    return result;
}
