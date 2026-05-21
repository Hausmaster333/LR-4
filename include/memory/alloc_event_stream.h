#ifndef ALLOC_EVENT_STREAM_H
#define ALLOC_EVENT_STREAM_H

#include <cstdint>
#include "lazy/lazy_sequence.h"
#include "lazy/generator.h"
#include "memory/alloc_event.h"

// Бесконечный генератор, который через LCG (Linear Congruential Generator) выдаёт псевдослучайные события Alloc(size) и Free(id)
class AllocEventGenerator : public Generator<AllocEvent> {
    private:
        uint64_t initial_seed;
        uint64_t state;
        int existing_blocks_cap;
        int max_alloc_size;
        int alloc_prob_percent;
        size_t pos;

        AllocEvent step_lcg() {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            uint64_t r1 = state >> 33;
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            uint64_t r2 = state >> 33;

            int pick = static_cast<int>(r1 % 100);
            if (pick < alloc_prob_percent) {
                int size = 1 + static_cast<int>(r2 % static_cast<uint64_t>(max_alloc_size));
                return AllocEvent::make_alloc(size);
            }

            int block_id = static_cast<int>(r2 % static_cast<uint64_t>(existing_blocks_cap));
            return AllocEvent::make_free(block_id);
        }
    public:
        AllocEventGenerator(uint64_t seed, int existing_blocks_cap, int max_alloc_size, int alloc_prob_percent)
            : initial_seed(seed), state(seed),
              existing_blocks_cap(existing_blocks_cap),
              max_alloc_size(max_alloc_size),
              alloc_prob_percent(alloc_prob_percent),
              pos(0) {}

        size_t position() const override { return pos; }
        bool has_next() const override { return true; }

        AllocEvent get_next() override {
            AllocEvent event = step_lcg();
            pos++;
            return event;
        }

        Option<AllocEvent> try_get_next() override {
            return Option<AllocEvent>::Some(get_next());
        }

        Ordinal estimate_remaining() const override { return Ordinal::infinity(); }

        Generator<AllocEvent>* clone() const override {
            return new AllocEventGenerator(initial_seed, existing_blocks_cap, max_alloc_size, alloc_prob_percent);
        }
};
// Параметры:
// seed - начальное состояние LCG
// existing_blocks_cap - диапазон id для Free-событий: [0-existing_blocks_cap)
// max_alloc_size - Alloc даст size из [1-max_alloc_size]
// alloc_prob_percent - доля Alloc от 0 до 100
inline LazySequence<AllocEvent>* make_alloc_event_stream(uint64_t seed, 
                                                         int existing_blocks_cap = 32, 
                                                         int max_alloc_size = 5,
                                                         int alloc_prob_percent = 70) {
    if (existing_blocks_cap <= 0) existing_blocks_cap = 1;
    if (max_alloc_size <= 0) max_alloc_size = 1;
    if (alloc_prob_percent < 0) alloc_prob_percent = 0;
    if (alloc_prob_percent > 100) alloc_prob_percent = 100;

    auto* generator = new AllocEventGenerator(seed, existing_blocks_cap, max_alloc_size, alloc_prob_percent);
    return new LazySequence<AllocEvent>(generator, Ordinal::infinity(), LazySequence<AllocEvent>::DEFAULT_CACHE_CAPACITY);
}

#endif
