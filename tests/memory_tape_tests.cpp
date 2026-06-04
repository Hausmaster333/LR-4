#include "memory/memory_tape.h"
#include "memory/alloc_strategy.h"
#include "memory/alloc_event_stream.h"
#include <gtest/gtest.h>

static FirstFitStrategy first_fit;
static BestFitStrategy best_fit;
static WorstFitStrategy worst_fit;

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
    EXPECT_EQ(t.alloc(2, first_fit), 0);
    EXPECT_EQ(t.alloc(3, first_fit), 1);
    EXPECT_EQ(t.alloc(1, first_fit), 2);
    EXPECT_EQ(t.get_used_count(), 6);
    EXPECT_EQ(t.get_blocks_count(), 3);
}

TEST(MemoryTapeBasicTest, AllocTooBigReturnsMinusOne) {
    MemoryTape t(5);
    EXPECT_EQ(t.alloc(0, first_fit), -1);
    EXPECT_EQ(t.alloc(6, first_fit), -1);
    EXPECT_EQ(t.alloc(5, first_fit), 0);
    EXPECT_EQ(t.alloc(1, first_fit), -1);   // нет места
}

TEST(MemoryTapeBasicTest, FreeReleasesCells) {
    MemoryTape t(6);
    int id = t.alloc(3, first_fit);
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
    t.alloc(2, first_fit);
    EXPECT_FALSE(t.free(42));
}

TEST(MemoryTapeBasicTest, ResetEmptiesTape) {
    MemoryTape t(5);
    t.alloc(2, first_fit);
    t.alloc(2, first_fit);

    t.reset();
    EXPECT_EQ(t.get_used_count(), 0);
    EXPECT_EQ(t.get_blocks_count(), 0);
    EXPECT_EQ(t.alloc(1, first_fit), 0);
}

namespace {
    MemoryTape make_layout() {
        MemoryTape t(10);
        t.alloc(2, first_fit);   // [0-1] id=0
        t.alloc(2, first_fit);   // [2-3] id=1
        t.alloc(4, first_fit);   // [4-7] id=2
        t.alloc(2, first_fit);   // [8-9] id=3
        t.free(0);
        t.free(2);

        return t;
    }
}

TEST(MemoryTapeStrategyTest, FirstFitTakesEarliest) {
    MemoryTape t = make_layout();
    int id = t.alloc(2, first_fit);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(t.get_cell(0).used);
    EXPECT_TRUE(t.get_cell(1).used);
    EXPECT_FALSE(t.get_cell(4).used);
}

TEST(MemoryTapeStrategyTest, BestFitTakesSmallestRun) {
    MemoryTape t = make_layout();
    int id = t.alloc(2, best_fit);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(t.get_cell(0).used);
    EXPECT_TRUE(t.get_cell(1).used);
    EXPECT_FALSE(t.get_cell(4).used);
}

TEST(MemoryTapeStrategyTest, WorstFitTakesLargestRun) {
    MemoryTape t = make_layout();
    int id = t.alloc(2, worst_fit);
    EXPECT_GE(id, 0);
    EXPECT_FALSE(t.get_cell(0).used);
    EXPECT_FALSE(t.get_cell(1).used);
    EXPECT_TRUE(t.get_cell(4).used);
    EXPECT_TRUE(t.get_cell(5).used);
}

// ===================

TEST(MemoryTapeFragmentationTest, EmptyTapeZeroFrag) {
    MemoryTape t(8);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

TEST(MemoryTapeFragmentationTest, FullTapeZeroFrag) {
    MemoryTape t(4);
    t.alloc(4, first_fit);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

TEST(MemoryTapeFragmentationTest, SingleRunZeroFrag) {
    MemoryTape t(8);
    t.alloc(3, first_fit);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

// =================== AllocEventStreamTest

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

        if (e.kind == AllocEventKind::Alloc) {
            EXPECT_GE(e.payload, 1);
            EXPECT_LE(e.payload, 5);
        } else {
            EXPECT_GE(e.payload, 0);
            EXPECT_LT(e.payload, 32);
        }
    }
    EXPECT_GE(alloc_count, 600);
    EXPECT_LE(alloc_count, 800);

    delete s;
}

TEST(MemoryTapeFragmentationTest, TwoEqualRunsHalfFrag) {
    MemoryTape t(4);

    t.reset();
    int x = t.alloc(1, first_fit);
    t.alloc(1, first_fit);
    int y = t.alloc(1, first_fit);
    t.alloc(1, first_fit);
    t.free(x);
    t.free(y);

    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.5);
}

// =================== NextFit

TEST(MemoryTapeStrategyTest, NextFitRemembersPosition) {
    MemoryTape t(8);
    NextFitStrategy next_fit;

    EXPECT_EQ(t.alloc(2, next_fit), 0);
    EXPECT_EQ(t.alloc(2, next_fit), 1);
    t.free(0);

    int id = t.alloc(2, next_fit);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(t.get_cell(4).used);
    EXPECT_TRUE(t.get_cell(5).used);
    EXPECT_FALSE(t.get_cell(0).used);
}

TEST(MemoryTapeStrategyTest, NextFitWrapsAround) {
    MemoryTape t(6);
    NextFitStrategy next_fit;

    t.alloc(2, next_fit);
    t.alloc(2, next_fit);
    t.alloc(2, next_fit);
    t.free(0);

    int id = t.alloc(2, next_fit);
    EXPECT_GE(id, 0);
    EXPECT_TRUE(t.get_cell(0).used);
}

// =================== Compact

TEST(MemoryTapeCompactTest, NewestFirstFreeAtEnd) {
    MemoryTape t(10);
    t.alloc(1, first_fit);
    t.alloc(2, first_fit);
    t.alloc(1, first_fit);
    t.alloc(2, first_fit);
    t.alloc(2, first_fit);
    t.free(0);
    t.free(2);

    t.compact();

    EXPECT_EQ(t.get_cell(0).block_id, 4);
    EXPECT_EQ(t.get_cell(1).block_id, 4);
    EXPECT_EQ(t.get_cell(2).block_id, 3);
    EXPECT_EQ(t.get_cell(3).block_id, 3);
    EXPECT_EQ(t.get_cell(4).block_id, 1);
    EXPECT_EQ(t.get_cell(5).block_id, 1);
    for (int index = 6; index < 10; index++) {
        EXPECT_FALSE(t.get_cell(index).used);
    }

    EXPECT_EQ(t.get_used_count(), 6);
    EXPECT_EQ(t.get_blocks_count(), 3);
    EXPECT_DOUBLE_EQ(t.fragmentation(), 0.0);
}

TEST(MemoryTapeCompactTest, EmptyTapeNoop) {
    MemoryTape t(5);
    t.compact();
    EXPECT_EQ(t.get_used_count(), 0);

    for (int index = 0; index < 5; index++) {
        EXPECT_FALSE(t.get_cell(index).used);
    }
}
