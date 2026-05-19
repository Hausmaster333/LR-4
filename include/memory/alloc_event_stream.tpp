#ifndef ALLOC_EVENT_STREAM_TPP
#define ALLOC_EVENT_STREAM_TPP

#include "memory/alloc_event_stream.h"
#include <type_traits>

template <class T>
LazySequence<T>* make_alloc_event_stream_impl(uint64_t seed, int existing_blocks_cap, int max_alloc_size, int alloc_prob_percent) {
    static_assert(std::is_same<T, AllocEvent>::value, "make_alloc_event_stream_impl is only implemented for T = AllocEvent");

    if (existing_blocks_cap <= 0) existing_blocks_cap = 1;
    if (max_alloc_size <= 0) max_alloc_size = 1;
    if (alloc_prob_percent < 0) alloc_prob_percent = 0;
    if (alloc_prob_percent > 100) alloc_prob_percent = 100;

    AllocEventGenerator* generator = new AllocEventGenerator(seed, existing_blocks_cap, max_alloc_size, alloc_prob_percent);
    return new LazySequence<T>(generator, Ordinal::infinity(),
                                LazySequence<T>::DEFAULT_CACHE_CAPACITY);
}

#endif
