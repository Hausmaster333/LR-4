#include "lazy/cardinal.h"
#include "lazy/lazy_sequence.h"
#include <gtest/gtest.h>

// ================ Cardinal tests

TEST(CardinalTest, FiniteCardinal) {
    Cardinal c = Cardinal::finite(5);

    EXPECT_TRUE(c.is_finite());
    EXPECT_FALSE(c.is_infinite());
    EXPECT_EQ(c.get_value(), 5u);
}

TEST(CardinalTest, InfiniteCardinal) {
    Cardinal c = Cardinal::infinity();

    EXPECT_TRUE(c.is_infinite());
    EXPECT_FALSE(c.is_finite());
    EXPECT_THROW(c.get_value(), std::logic_error);
}

TEST(CardinalTest, Equality) {
    EXPECT_EQ(Cardinal::finite(3), Cardinal::finite(3));
    EXPECT_NE(Cardinal::finite(3), Cardinal::finite(4));
    EXPECT_EQ(Cardinal::infinity(), Cardinal::infinity());
    EXPECT_NE(Cardinal::finite(1000000), Cardinal::infinity());
}

// ================ LazySequence tests

TEST(LazySequenceTest, EmptyConstructor) {
    LazySequence<int> seq;

    EXPECT_EQ(seq.get_length(), Cardinal::finite(0));
    EXPECT_EQ(seq.get_materialized_count(), 0u);
}

TEST(LazySequenceTest, ArrayConstructor) {
    int items[] = {1, 2, 3};

    LazySequence<int> seq(items, 3);

    EXPECT_EQ(seq.get_length(), Cardinal::finite(3));
    EXPECT_EQ(seq.get_materialized_count(), 3u);
}

TEST(LazySequenceTest, SequenceConstructor) {
    int items[] = {10, 20, 30};
    MutableArraySequence<int> source(items, 3);

    LazySequence<int> seq(&source);

    EXPECT_EQ(seq.get_length(), Cardinal::finite(3));
    EXPECT_EQ(seq.get_materialized_count(), 3u);
}

TEST(LazySequenceTest, GetFromArray) {
    int items[] = {10, 20, 30};

    LazySequence<int> seq(items, 3);

    EXPECT_EQ(seq.get_first(), 10);
    EXPECT_EQ(seq.get_last(), 30);
    EXPECT_EQ(seq.get(0), 10);
    EXPECT_EQ(seq.get(1), 20);
    EXPECT_EQ(seq.get(2), 30);
}

TEST(LazySequenceTest, GetFromEmptyThrows) {
    LazySequence<int> seq;

    EXPECT_THROW(seq.get_first(), std::out_of_range);
    EXPECT_THROW(seq.get_last(), std::out_of_range);
    EXPECT_THROW(seq.get(0), std::out_of_range);
}

TEST(LazySequenceTest, GetOutOfRangeThrows) {
    int items[] = {1, 2, 3};

    LazySequence<int> seq(items, 3);

    EXPECT_THROW(seq.get(-1), std::out_of_range);
    EXPECT_THROW(seq.get(3), std::out_of_range);
}

TEST(LazySequenceTest, AppendReturnsNewSequence) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.append(4);

    EXPECT_EQ(seq.get_length(), Cardinal::finite(3));
    EXPECT_EQ(seq.get_last(), 3);

    EXPECT_EQ(result->get_length(), Cardinal::finite(4));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 4);

    delete result;
}

TEST(LazySequenceTest, PrependReturnsNewSequence) {
    int items[] = {2, 3, 4};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.prepend(1);

    EXPECT_EQ(seq.get_length(), Cardinal::finite(3));
    EXPECT_EQ(seq.get_first(), 2);

    EXPECT_EQ(result->get_length(), Cardinal::finite(4));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 4);

    delete result;
}

TEST(LazySequenceTest, InsertAtMiddleReturnsNewSequence) {
    int items[] = {1, 2, 4};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.insert_at(3, 2);

    EXPECT_EQ(seq.get_length(), Cardinal::finite(3));
    EXPECT_EQ(seq.get(2), 4);

    EXPECT_EQ(result->get_length(), Cardinal::finite(4));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 4);

    delete result;
}

TEST(LazySequenceTest, InsertAtBeginning) {
    int items[] = {2, 3};
    LazySequence<int> seq(items, 2);

    LazySequence<int>* result = seq.insert_at(1, 0);

    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);

    delete result;
}

TEST(LazySequenceTest, InsertAtEnd) {
    int items[] = {1, 2};
    LazySequence<int> seq(items, 2);

    LazySequence<int>* result = seq.insert_at(3, 2);

    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);

    delete result;
}

TEST(LazySequenceTest, InsertAtInvalidIndexThrows) {
    int items[] = {1, 2};
    LazySequence<int> seq(items, 2);

    EXPECT_THROW(seq.insert_at(0, -1), std::out_of_range);
    EXPECT_THROW(seq.insert_at(0, 3), std::out_of_range);
}

TEST(LazySequenceTest, GetSubSequenceMiddle) {
    int items[] = {10, 20, 30, 40, 50};
    LazySequence<int> seq(items, 5);

    LazySequence<int>* result = seq.get_sub_sequence(1, 3);

    EXPECT_EQ(result->get_length(), Cardinal::finite(3));
    EXPECT_EQ(result->get(0), 20);
    EXPECT_EQ(result->get(1), 30);
    EXPECT_EQ(result->get(2), 40);

    delete result;
}

TEST(LazySequenceTest, GetSubSequenceSingleElement) {
    int items[] = {10, 20, 30};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.get_sub_sequence(1, 1);

    EXPECT_EQ(result->get_length(), Cardinal::finite(1));
    EXPECT_EQ(result->get(0), 20);

    delete result;
}

TEST(LazySequenceTest, GetSubSequenceFullRange) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.get_sub_sequence(0, 2);

    EXPECT_EQ(result->get_length(), Cardinal::finite(3));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);

    delete result;
}

TEST(LazySequenceTest, GetSubSequenceInvalidRangeThrows) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    EXPECT_THROW(seq.get_sub_sequence(-1, 1), std::out_of_range);
    EXPECT_THROW(seq.get_sub_sequence(0, 3), std::out_of_range);
    EXPECT_THROW(seq.get_sub_sequence(2, 1), std::out_of_range);
}
