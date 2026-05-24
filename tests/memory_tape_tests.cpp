#include "memory/memory_tape.h"
#include "memory/alloc_event.h"
#include "memory/alloc_event_stream.h"
#include <gtest/gtest.h>

// =================== MemoryTapeBasicTest ===================

TEST(MemoryTapeBasicTest, ZeroCapacityThrows) {
    EXPECT_THROW(MemoryTape(0), std::invalid_argument);
    EXPECT_THROW(MemoryTape(-1), std::invalid_argument);
}

TEST(MemoryTapeBasicTest, AfterConstructionAllFree) {
    MemoryTape t(8);
    EXPECT_EQ(t.get_capacity(), 8);
    EXPECT_EQ(t.get_used_count(), 0);
    EXPECT_EQ(t.get_free_count(), 8);
    EXPECT_EQ(t.get_blocks_count(), 0);
    for (int idx = 0; idx < 8; ++idx) {
        EXPECT_FALSE(t.get_cell(idx).used);
    }
}

TEST(MemoryTapeBasicTest, AllocReturnsIncrementingIds) {
    MemoryTape t(10);
    EXPECT_EQ(t.alloc(2, AllocStrategy::FirstFit), 0);
    EXPECT_EQ(t.alloc(3, AllocStrategy::FirstFit), 1);
    EXPECT_EQ(t.alloc(1, AllocStrategy::FirstFit), 2);
    EXPECT_EQ(t.get_used_count(), 6);
    EXPECT_EQ(t.get_blocks_count(), 3);
}

TEST(MemoryTapeBasicTest, AllocTooBigReturnsMinusOne) {
    MemoryTape t(5);
    EXPECT_EQ(t.alloc(0, AllocStrategy::FirstFit), -1);
    EXPECT_EQ(t.alloc(6, AllocStrategy::FirstFit), -1);
    EXPECT_EQ(t.alloc(5, AllocStrategy::FirstFit), 0);
    EXPECT_EQ(t.alloc(1, AllocStrategy::FirstFit), -1);   // нет места
}

TEST(MemoryTapeBasicTest, FreeReleasesCells) {
    MemoryTape t(6);
    int id = t.alloc(3, AllocStrategy::FirstFit);
    EXPECT_EQ(t.get_used_count(), 3);

    EXPECT_TRUE(t.free(id));
    EXPECT_EQ(t.get_used_count(), 0);
    EXPECT_EQ(t.get_blocks_count(), 0);
    for (int idx = 0; idx < 6; ++idx) {
        EXPECT_FALSE(t.get_cell(idx).used);
    }
}

TEST(MemoryTapeBasicTest, FreeUnknownReturnsFalse) {
    MemoryTape t(4);
    EXPECT_FALSE(t.free(999));
    t.alloc(2, AllocStrategy::FirstFit);
    EXPECT_FALSE(t.free(42));
}

TEST(MemoryTapeBasicTest, ResetEmptiesTape) {
    MemoryTape t(5);
    t.alloc(2, AllocStrategy::FirstFit);
    t.alloc(2, AllocStrategy::FirstFit);

    t.reset();
    EXPECT_EQ(t.get_used_count(), 0);
    EXPECT_EQ(t.get_blocks_count(), 0);
    // next_block_id тоже сброшен
    EXPECT_EQ(t.alloc(1, AllocStrategy::FirstFit), 0);
}

// =================== MemoryTapeStrategyTest ===================
// Раскладка [F F U U F F F F U U]: cap=10, два блока по 2.

namespace {
    MemoryTape make_layout_10() {
        MemoryTape t(10);
        // alloc 2 - start 0 (id=0)
        // alloc 2 - start 2 (id=1)
        // alloc 4 - start 4 (id=2)
        // alloc 2 - start 8 (id=3)
        // free id=0 - освободит [0-1]
        // free id=2 - освободит [4-7]
        // итог: [F F U U F F F F U U]
        t.alloc(2, AllocStrategy::FirstFit);   // [0-1] id=0
        t.alloc(2, AllocStrategy::FirstFit);   // [2-3] id=1
        t.alloc(4, AllocStrategy::FirstFit);   // [4-7] id=2
        t.alloc(2, AllocStrategy::FirstFit);   // [8-9] id=3
        t.free(0);
        t.free(2);
        return t;
    }
}

TEST(MemoryTapeStrategyTest, FirstFitTakesEarliest) {
    MemoryTape t = make_layout_10();
    int id = t.alloc(2, AllocStrategy::FirstFit);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(t.get_cell(0).used);
    EXPECT_TRUE(t.get_cell(1).used);
    EXPECT_FALSE(t.get_cell(4).used);
}

TEST(MemoryTapeStrategyTest, BestFitTakesSmallestRun) {
    MemoryTape t = make_layout_10();
    int id = t.alloc(2, AllocStrategy::BestFit);
    EXPECT_GE(id, 0);
    // Run длины 2 в [0-1] = best match, не должны занять [4-7] длины 4.
    EXPECT_TRUE(t.get_cell(0).used);
    EXPECT_TRUE(t.get_cell(1).used);
    EXPECT_FALSE(t.get_cell(4).used);
}

TEST(MemoryTapeStrategyTest, WorstFitTakesLargestRun) {
    MemoryTape t = make_layout_10();
    int id = t.alloc(2, AllocStrategy::WorstFit);
    EXPECT_GE(id, 0);
    // Run длины 4 в [4..7] = worst match (самый большой).
    EXPECT_FALSE(t.get_cell(0).used);
    EXPECT_FALSE(t.get_cell(1).used);
    EXPECT_TRUE(t.get_cell(4).used);
    EXPECT_TRUE(t.get_cell(5).used);
}

// =================== MemoryTapeFragmentationTest ===================

TEST(MemoryTapeFragmentationTest, EmptyTapeZeroFrag) {
    MemoryTape t(8);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

TEST(MemoryTapeFragmentationTest, FullTapeZeroFrag) {
    MemoryTape t(4);
    t.alloc(4, AllocStrategy::FirstFit);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

TEST(MemoryTapeFragmentationTest, SingleRunZeroFrag) {
    MemoryTape t(8);
    t.alloc(3, AllocStrategy::FirstFit);
    // Свободные [3..7] = один run длины 5, free=5, max_run=5 → frag=0.
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

// =================== AllocEventStreamTest ===================

TEST(AllocEventStreamTest, ConsistentForSameSeed) {
    LazySequence<AllocEvent>* a = make_alloc_event_stream(12345ULL, 32, 5, 70);
    LazySequence<AllocEvent>* b = make_alloc_event_stream(12345ULL, 32, 5, 70);

    for (int i = 0; i < 100; ++i) {
        AllocEvent ea = a->get(i);
        AllocEvent eb = b->get(i);
        EXPECT_EQ(static_cast<int>(ea.kind), static_cast<int>(eb.kind));
        EXPECT_EQ(ea.payload, eb.payload);
    }
    delete a;
    delete b;
}

TEST(AllocEventStreamTest, ProportionRoughly70Alloc) {
    LazySequence<AllocEvent>* s = make_alloc_event_stream(99ULL, 32, 5, 70);

    int alloc_count = 0;
    const int N = 1000;
    for (int i = 0; i < N; ++i) {
        AllocEvent e = s->get(i);
        if (e.kind == AllocEventKind::Alloc) alloc_count++;
        // sanity
        if (e.kind == AllocEventKind::Alloc) {
            EXPECT_GE(e.payload, 1);
            EXPECT_LE(e.payload, 5);
        } else {
            EXPECT_GE(e.payload, 0);
            EXPECT_LT(e.payload, 32);
        }
    }
    // Допуск ±10% от 700.
    EXPECT_GE(alloc_count, 600);
    EXPECT_LE(alloc_count, 800);

    delete s;
}

TEST(MemoryTapeFragmentationTest, TwoEqualRunsHalfFrag) {
    // cap=4, состояние [F U F U]: free=2, max_run=1, frag = 1 - 1/2 = 0.5
    MemoryTape t(4);
    int a = t.alloc(1, AllocStrategy::FirstFit);  // [0] id=0
    int b = t.alloc(1, AllocStrategy::FirstFit);  // [1] id=1
    t.alloc(1, AllocStrategy::FirstFit);          // [2] id=2
    t.alloc(1, AllocStrategy::FirstFit);          // [3] id=3
    t.free(a);  // free [0]
    t.free(b);  // free [1] — теперь run [0..1] длины 2, не подходит для теста.
    // Перестрою:
    t.reset();
    int x = t.alloc(1, AllocStrategy::FirstFit);  // [0]
    t.alloc(1, AllocStrategy::FirstFit);          // [1]
    int y = t.alloc(1, AllocStrategy::FirstFit);  // [2]
    t.alloc(1, AllocStrategy::FirstFit);          // [3]
    t.free(x);
    t.free(y);
    // Состояние [F U F U]: 2 свободные ячейки в разных run-ах длины 1.
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.5);
}
