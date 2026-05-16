#ifndef ALLOC_EVENT_STREAM_H
#define ALLOC_EVENT_STREAM_H

#include <cstdint>
#include "lazy/lazy_sequence.h"
#include "lazy/generator.h"
#include "memory/alloc_event.h"

// AllocEventGenerator - бесконечный Generator, который через LCG (Linear
// Congruential Generator) выдаёт псевдослучайные события Alloc(size) и Free(id)
// Хранит state в обычном поле, без shared_ptr. clone() начинает свежую копию
// с тем же initial_seed - оригинал и клон полностью независимы
class AllocEventGenerator : public Generator<AllocEvent> {
    private:
        uint64_t initial_seed; // Запоминаем для clone (нужно начать заново с того же seed)
        uint64_t state; // Текущее состояние LCG, меняется на каждом get_next
        int existing_blocks_cap; // Диапазон id для Free-событий: [0..cap)
        int max_alloc_size; // Диапазон size для Alloc-событий: [1..max_alloc_size]
        int alloc_prob_percent; // 0..100, вероятность Alloc-события (иначе Free)
        size_t pos;

        // Один шаг LCG: меняет state и возвращает следующее событие
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

        Cardinal estimate_remaining() const override { return Cardinal::infinity(); }

        // Свежая копия с pos=0 и тем же initial_seed
        // Каждый клон выдаёт идентичную последовательность с самого начала
        Generator<AllocEvent>* clone() const override {
            return new AllocEventGenerator(initial_seed, existing_blocks_cap, max_alloc_size, alloc_prob_percent);
        }
};

// Template-internal: имеет friend-доступ к приватному ctor LazySequence
// Реально работает только с T = AllocEvent (проверяется через static_assert)
// Caller не должен звать напрямую - есть удобный non-template wrapper ниже
template <class T>
LazySequence<T>* make_alloc_event_stream_impl(uint64_t seed, int existing_blocks_cap, int max_alloc_size, int alloc_prob_percent);

// Фабрика бесконечного потока событий. Эргономичная обёртка над template-impl
//
// Параметры:
//   seed                - начальное состояние LCG
//   existing_blocks_cap - диапазон id для Free-событий: [0..existing_blocks_cap)
//   max_alloc_size      - Alloc-событие даст size из [1..max_alloc_size]
//   alloc_prob_percent  - 0..100, доля Alloc-событий
inline LazySequence<AllocEvent>* make_alloc_event_stream(uint64_t seed,
                                                          int existing_blocks_cap = 32,
                                                          int max_alloc_size = 5,
                                                          int alloc_prob_percent = 70) {
    return make_alloc_event_stream_impl<AllocEvent>(seed, existing_blocks_cap, max_alloc_size, alloc_prob_percent);
}

#include "alloc_event_stream.tpp"

#endif
