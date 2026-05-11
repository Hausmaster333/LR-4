#include "deque/segment_deque.h"
#include "types/complex.h"
#include "types/person_types.h"
#include "sorting_station/sorting_station.h"
#include <gtest/gtest.h>
#include <string>

int double_val(const int& x) { return x * 2; }
int square(const int& x) { return x * x; }
int add(const int& a, const int& b) { return a + b; }
bool is_even(const int& x) { return x % 2 == 0; }
bool is_positive(const int& x) { return x > 0; }
bool less_than(const int& a, const int& b) { return a < b; }
bool greater_than(const int& a, const int& b) { return a > b; }

// =================== Конструкторы

TEST(SegmentedDequeTest, DefaultConstructor) {
    MutableSegmentedDeque<int> deque;
    EXPECT_EQ(deque.get_count(), 0);
}

TEST(SegmentedDequeTest, ArrayConstructor) {
    int items[] = {10, 20, 30, 40, 50};
    MutableSegmentedDeque<int> deque(items, 5);

    EXPECT_EQ(deque.get_count(), 5);
    EXPECT_EQ(deque.get(0), 10);
    EXPECT_EQ(deque.get(1), 20);
    EXPECT_EQ(deque.get(2), 30);
    EXPECT_EQ(deque.get(3), 40);
    EXPECT_EQ(deque.get(4), 50);
}

TEST(SegmentedDequeTest, ArrayConstructorEmpty) {
    MutableSegmentedDeque<int> deque(nullptr, 0);
    EXPECT_EQ(deque.get_count(), 0);
}

TEST(SegmentedDequeTest, CopyConstructor) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> original(items, 3);
    MutableSegmentedDeque<int> copy(original);

    EXPECT_EQ(copy.get_count(), 3);
    EXPECT_EQ(copy.get(0), 1);
    EXPECT_EQ(copy.get(1), 2);
    EXPECT_EQ(copy.get(2), 3);

    // Проверяем независимость копии от оригинала
    original.push_back(99);
    EXPECT_EQ(original.get_count(), 4);
    EXPECT_EQ(copy.get_count(), 3);
}

TEST(SegmentedDequeTest, AssignmentOperator) {
    int items1[] = {1, 2, 3};
    int items2[] = {10, 20};
    MutableSegmentedDeque<int> a(items1, 3);
    MutableSegmentedDeque<int> b(items2, 2);

    b = a;
    EXPECT_EQ(b.get_count(), 3);
    EXPECT_EQ(b.get(0), 1);
    EXPECT_EQ(b.get(1), 2);
    EXPECT_EQ(b.get(2), 3);

    // Проверяем независимость
    a.push_back(99);
    EXPECT_EQ(b.get_count(), 3);
}

TEST(SegmentedDequeTest, SelfAssignment) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> a(items, 3);
    a = a;
    EXPECT_EQ(a.get_count(), 3);
    EXPECT_EQ(a.get(0), 1);
}

// =================== push & pop

TEST(SegmentedDequeTest, PushBackSingle) {
    MutableSegmentedDeque<int> deque;
    deque.push_back(42);

    EXPECT_EQ(deque.get_count(), 1);
    EXPECT_EQ(deque.get(0), 42);
}

TEST(SegmentedDequeTest, PushFrontSingle) {
    MutableSegmentedDeque<int> deque;
    deque.push_front(42);

    EXPECT_EQ(deque.get_count(), 1);
    EXPECT_EQ(deque.get(0), 42);
}

TEST(SegmentedDequeTest, PushBackMultiple) {
    MutableSegmentedDeque<int> deque;
    for (int idx = 0; idx < 20; idx++) {
        deque.push_back(idx);
    }

    EXPECT_EQ(deque.get_count(), 20);
    for (int idx = 0; idx < 20; idx++) {
        EXPECT_EQ(deque.get(idx), idx);
    }
}

TEST(SegmentedDequeTest, PushFrontMultiple) {
    MutableSegmentedDeque<int> deque;
    for (int idx = 0; idx < 20; idx++) {
        deque.push_front(idx);
    }

    EXPECT_EQ(deque.get_count(), 20);
    for (int idx = 0; idx < 20; idx++) {
        EXPECT_EQ(deque.get(idx), 19 - idx);
    }
}

TEST(SegmentedDequeTest, PushBothEnds) {
    MutableSegmentedDeque<int> deque;
    deque.push_back(2);
    deque.push_back(3);
    deque.push_front(1);
    deque.push_front(0);

    EXPECT_EQ(deque.get_count(), 4);
    EXPECT_EQ(deque.get(0), 0);
    EXPECT_EQ(deque.get(1), 1);
    EXPECT_EQ(deque.get(2), 2);
    EXPECT_EQ(deque.get(3), 3);
}

TEST(SegmentedDequeTest, PopBackSingle) {
    MutableSegmentedDeque<int> deque;
    deque.push_back(42);

    int result;
    deque.pop_back(&result);
    EXPECT_EQ(result, 42);
    EXPECT_EQ(deque.get_count(), 0);
}

TEST(SegmentedDequeTest, PopFrontSingle) {
    MutableSegmentedDeque<int> deque;
    deque.push_back(42);

    int result;
    deque.pop_front(&result);
    EXPECT_EQ(result, 42);
    EXPECT_EQ(deque.get_count(), 0);
}

TEST(SegmentedDequeTest, PopBackMultiple) {
    int items[] = {1, 2, 3, 4, 5};
    MutableSegmentedDeque<int> deque(items, 5);

    int result;
    deque.pop_back(&result);
    EXPECT_EQ(result, 5);
    deque.pop_back(&result);
    EXPECT_EQ(result, 4);
    EXPECT_EQ(deque.get_count(), 3);
}

TEST(SegmentedDequeTest, PopFrontMultiple) {
    int items[] = {1, 2, 3, 4, 5};
    MutableSegmentedDeque<int> deque(items, 5);

    int result;
    deque.pop_front(&result);
    EXPECT_EQ(result, 1);
    deque.pop_front(&result);
    EXPECT_EQ(result, 2);
    EXPECT_EQ(deque.get_count(), 3);
    EXPECT_EQ(deque.get(0), 3);
}

TEST(SegmentedDequeTest, PushPopInterleaved) {
    MutableSegmentedDeque<int> deque;
    deque.push_back(1);
    deque.push_back(2);
    deque.push_front(0);

    int result;
    deque.pop_back(&result);
    EXPECT_EQ(result, 2);

    deque.push_back(3);
    deque.pop_front(&result);
    EXPECT_EQ(result, 0);

    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 3);
}

// Тест на переход через границу сегмента (segment_size = 8)
TEST(SegmentedDequeTest, PushAcrossSegmentBoundary) {
    MutableSegmentedDeque<int> deque;
    for (int idx = 0; idx < 25; idx++) {
        deque.push_back(idx * 10);
    }

    EXPECT_EQ(deque.get_count(), 25);
    for (int idx = 0; idx < 25; idx++) {
        EXPECT_EQ(deque.get(idx), idx * 10);
    }
}

TEST(SegmentedDequeTest, PushFrontAcrossSegmentBoundary) {
    MutableSegmentedDeque<int> deque;
    for (int idx = 0; idx < 25; idx++) {
        deque.push_front(idx);
    }

    EXPECT_EQ(deque.get_count(), 25);
    for (int idx = 0; idx < 25; idx++) {
        EXPECT_EQ(deque.get(idx), 24 - idx);
    }
}

// =================== Геттеры

TEST(SegmentedDequeTest, GetFirstLast) {
    int items[] = {10, 20, 30};
    MutableSegmentedDeque<int> deque(items, 3);

    EXPECT_EQ(deque.get_first(), 10);
    EXPECT_EQ(deque.get_last(), 30);
}

TEST(SegmentedDequeTest, GetByIndex) {
    int items[] = {5, 10, 15, 20, 25};
    MutableSegmentedDeque<int> deque(items, 5);

    for (int idx = 0; idx < 5; idx++) {
        EXPECT_EQ(deque.get(idx), (idx + 1) * 5);
    }
}

TEST(SegmentedDequeTest, TryGetValid) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);

    Option<int> first = deque.try_get_first();
    Option<int> last = deque.try_get_last();
    Option<int> mid = deque.try_get(1);

    EXPECT_TRUE(first.has_value());
    EXPECT_EQ(first.get_value(), 1);
    EXPECT_TRUE(last.has_value());
    EXPECT_EQ(last.get_value(), 3);
    EXPECT_TRUE(mid.has_value());
    EXPECT_EQ(mid.get_value(), 2);
}

TEST(SegmentedDequeTest, TryGetEmpty) {
    MutableSegmentedDeque<int> deque;

    Option<int> first = deque.try_get_first();
    Option<int> last = deque.try_get_last();
    Option<int> any = deque.try_get(0);

    EXPECT_FALSE(first.has_value());
    EXPECT_FALSE(last.has_value());
    EXPECT_FALSE(any.has_value());
}

// =================== get_sub_sequence

TEST(SegmentedDequeTest, SubSequence) {
    int items[] = {10, 20, 30, 40, 50};
    MutableSegmentedDeque<int> deque(items, 5);

    MutableSegmentedDeque<int>* sub = deque.get_sub_sequence_defined(1, 3);
    EXPECT_EQ(sub->get_count(), 3);
    EXPECT_EQ(sub->get(0), 20);
    EXPECT_EQ(sub->get(1), 30);
    EXPECT_EQ(sub->get(2), 40);
    delete sub;
}

TEST(SegmentedDequeTest, SubSequenceFullRange) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);

    MutableSegmentedDeque<int>* sub = deque.get_sub_sequence_defined(0, 2);
    EXPECT_EQ(sub->get_count(), 3);
    EXPECT_EQ(sub->get(0), 1);
    EXPECT_EQ(sub->get(2), 3);
    delete sub;
}

TEST(SegmentedDequeTest, SubSequenceSingleElement) {
    int items[] = {10, 20, 30};
    MutableSegmentedDeque<int> deque(items, 3);

    MutableSegmentedDeque<int>* sub = deque.get_sub_sequence_defined(1, 1);
    EXPECT_EQ(sub->get_count(), 1);
    EXPECT_EQ(sub->get(0), 20);
    delete sub;
}

// =================== append, prepend, insert

TEST(SegmentedDequeTest, Append) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);

    deque.append(4);
    EXPECT_EQ(deque.get_count(), 4);
    EXPECT_EQ(deque.get(3), 4);
}

TEST(SegmentedDequeTest, Prepend) {
    int items[] = {2, 3, 4};
    MutableSegmentedDeque<int> deque(items, 3);

    deque.prepend(1);
    EXPECT_EQ(deque.get_count(), 4);
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 2);
}

TEST(SegmentedDequeTest, InsertAtBeginning) {
    int items[] = {2, 3, 4};
    MutableSegmentedDeque<int> deque(items, 3);

    deque.insert_at(1, 0);
    EXPECT_EQ(deque.get_count(), 4);
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 2);
}

TEST(SegmentedDequeTest, InsertAtMiddle) {
    int items[] = {1, 2, 4, 5};
    MutableSegmentedDeque<int> deque(items, 4);

    deque.insert_at(3, 2);
    EXPECT_EQ(deque.get_count(), 5);
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 2);
    EXPECT_EQ(deque.get(2), 3);
    EXPECT_EQ(deque.get(3), 4);
    EXPECT_EQ(deque.get(4), 5);
}

TEST(SegmentedDequeTest, InsertAtEnd) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);

    deque.insert_at(4, 3);
    EXPECT_EQ(deque.get_count(), 4);
    EXPECT_EQ(deque.get(3), 4);
}

// =================== concat

TEST(SegmentedDequeTest, Concat) {
    int items1[] = {1, 2, 3};
    int items2[] = {4, 5, 6};
    MutableSegmentedDeque<int> deque1(items1, 3);
    MutableSegmentedDeque<int> deque2(items2, 3);

    MutableSegmentedDeque<int>* result = deque1.concat_defined(&deque2);
    EXPECT_EQ(result->get_count(), 6);
    for (int idx = 0; idx < 6; idx++) {
        EXPECT_EQ(result->get(idx), idx + 1);
    }
    delete result;
}

TEST(SegmentedDequeTest, ConcatWithEmpty) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);
    MutableSegmentedDeque<int> empty;

    Sequence<int>* result = deque.concat(&empty);
    EXPECT_EQ(result->get_count(), 3);
    delete result;
}

// =================== mar, where, reduce

TEST(SegmentedDequeTest, Map) {
    int items[] = {1, 2, 3, 4};
    MutableSegmentedDeque<int> deque(items, 4);

    MutableSegmentedDeque<int>* result = deque.map_defined(double_val);
    EXPECT_EQ(result->get_count(), 4);
    EXPECT_EQ(result->get(0), 2);
    EXPECT_EQ(result->get(1), 4);
    EXPECT_EQ(result->get(2), 6);
    EXPECT_EQ(result->get(3), 8);
    delete result;
}

TEST(SegmentedDequeTest, MapSquare) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);

    MutableSegmentedDeque<int>* result = deque.map_defined(square);
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 4);
    EXPECT_EQ(result->get(2), 9);
    delete result;
}

TEST(SegmentedDequeTest, Where) {
    int items[] = {1, 2, 3, 4, 5, 6};
    MutableSegmentedDeque<int> deque(items, 6);

    MutableSegmentedDeque<int>* result = deque.where_defined(is_even);
    EXPECT_EQ(result->get_count(), 3);
    EXPECT_EQ(result->get(0), 2);
    EXPECT_EQ(result->get(1), 4);
    EXPECT_EQ(result->get(2), 6);
    delete result;
}

TEST(SegmentedDequeTest, WhereNoneMatch) {
    int items[] = {-1, -2, -3};
    MutableSegmentedDeque<int> deque(items, 3);

    Sequence<int>* result = deque.where(is_positive);
    EXPECT_EQ(result->get_count(), 0);
    delete result;
}

TEST(SegmentedDequeTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    MutableSegmentedDeque<int> deque(items, 5);

    int result = deque.reduce(add, 0);
    EXPECT_EQ(result, 15);
}

TEST(SegmentedDequeTest, ReduceSingleElement) {
    int items[] = {42};
    MutableSegmentedDeque<int> deque(items, 1);

    int result = deque.reduce(add, 0);
    EXPECT_EQ(result, 42);
}

TEST(SegmentedDequeTest, ReduceEmpty) {
    MutableSegmentedDeque<int> deque;

    int result = deque.reduce(add, 100);
    EXPECT_EQ(result, 100);
}

// =================== sort

TEST(SegmentedDequeTest, SortAscending) {
    int items[] = {5, 2, 8, 1, 9, 3};
    MutableSegmentedDeque<int> deque(items, 6);

    deque.sort();
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 2);
    EXPECT_EQ(deque.get(2), 3);
    EXPECT_EQ(deque.get(3), 5);
    EXPECT_EQ(deque.get(4), 8);
    EXPECT_EQ(deque.get(5), 9);
}

TEST(SegmentedDequeTest, SortDescending) {
    int items[] = {5, 2, 8, 1, 9, 3};
    MutableSegmentedDeque<int> deque(items, 6);

    deque.sort(greater_than);
    EXPECT_EQ(deque.get(0), 9);
    EXPECT_EQ(deque.get(1), 8);
    EXPECT_EQ(deque.get(2), 5);
    EXPECT_EQ(deque.get(3), 3);
    EXPECT_EQ(deque.get(4), 2);
    EXPECT_EQ(deque.get(5), 1);
}

TEST(SegmentedDequeTest, SortAlreadySorted) {
    int items[] = {1, 2, 3, 4, 5};
    MutableSegmentedDeque<int> deque(items, 5);

    deque.sort();
    for (int idx = 0; idx < 5; idx++) {
        EXPECT_EQ(deque.get(idx), idx + 1);
    }
}

TEST(SegmentedDequeTest, SortSingleElement) {
    int items[] = {42};
    MutableSegmentedDeque<int> deque(items, 1);

    deque.sort();
    EXPECT_EQ(deque.get(0), 42);
}

TEST(SegmentedDequeTest, SortWithDuplicates) {
    int items[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    MutableSegmentedDeque<int> deque(items, 10);

    deque.sort();
    EXPECT_EQ(deque.get(0), 1);
    EXPECT_EQ(deque.get(1), 1);
    EXPECT_EQ(deque.get(2), 2);
    EXPECT_EQ(deque.get(3), 3);
    EXPECT_EQ(deque.get(4), 3);
    EXPECT_EQ(deque.get(5), 4);
    EXPECT_EQ(deque.get(6), 5);
    EXPECT_EQ(deque.get(7), 5);
    EXPECT_EQ(deque.get(8), 6);
    EXPECT_EQ(deque.get(9), 9);
}

// =================== merge

TEST(SegmentedDequeTest, MergeSorted) {
    int items1[] = {1, 3, 5, 7};
    int items2[] = {2, 4, 6, 8};
    MutableSegmentedDeque<int> deque1(items1, 4);
    MutableSegmentedDeque<int> deque2(items2, 4);

    SegmentedDeque<int>* merged = deque1.merge(&deque2);
    EXPECT_EQ(merged->get_count(), 8);
    for (int idx = 0; idx < 8; idx++) {
        EXPECT_EQ(merged->get(idx), idx + 1);
    }
    delete merged;
}

TEST(SegmentedDequeTest, MergeDifferentSizes) {
    int items1[] = {1, 5};
    int items2[] = {2, 3, 4, 6, 7};
    MutableSegmentedDeque<int> deque1(items1, 2);
    MutableSegmentedDeque<int> deque2(items2, 5);

    SegmentedDeque<int>* merged = deque1.merge(&deque2);
    EXPECT_EQ(merged->get_count(), 7);
    for (int idx = 0; idx < 7; idx++) {
        EXPECT_EQ(merged->get(idx), idx + 1);
    }
    delete merged;
}

TEST(SegmentedDequeTest, MergeWithEmpty) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);
    MutableSegmentedDeque<int> empty;

    SegmentedDeque<int>* merged = deque.merge(&empty);
    EXPECT_EQ(merged->get_count(), 3);
    EXPECT_EQ(merged->get(0), 1);
    delete merged;
}

// =================== Первое вхождение подпоследовательности

TEST(SegmentedDequeTest, FindSubSequenceFound) {
    int items[] = {1, 2, 3, 4, 5, 6};
    int sub_items[] = {3, 4, 5};
    MutableSegmentedDeque<int> deque(items, 6);
    MutableSegmentedDeque<int> sub(sub_items, 3);

    EXPECT_EQ(deque.find_sub_sequence(&sub), 2);
}

TEST(SegmentedDequeTest, FindSubSequenceAtStart) {
    int items[] = {1, 2, 3, 4, 5};
    int sub_items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 5);
    MutableSegmentedDeque<int> sub(sub_items, 3);

    EXPECT_EQ(deque.find_sub_sequence(&sub), 0);
}

TEST(SegmentedDequeTest, FindSubSequenceAtEnd) {
    int items[] = {1, 2, 3, 4, 5};
    int sub_items[] = {4, 5};
    MutableSegmentedDeque<int> deque(items, 5);
    MutableSegmentedDeque<int> sub(sub_items, 2);

    EXPECT_EQ(deque.find_sub_sequence(&sub), 3);
}

TEST(SegmentedDequeTest, FindSubSequenceNotFound) {
    int items[] = {1, 2, 3, 4, 5};
    int sub_items[] = {2, 4};
    MutableSegmentedDeque<int> deque(items, 5);
    MutableSegmentedDeque<int> sub(sub_items, 2);

    EXPECT_EQ(deque.find_sub_sequence(&sub), -1);
}

TEST(SegmentedDequeTest, FindSubSequenceSingleElement) {
    int items[] = {1, 2, 3};
    int sub_items[] = {2};
    MutableSegmentedDeque<int> deque(items, 3);
    MutableSegmentedDeque<int> sub(sub_items, 1);

    EXPECT_EQ(deque.find_sub_sequence(&sub), 1);
}

// =================== Исключения

TEST(SegmentedDequeTest, GetFromEmptyThrows) {
    MutableSegmentedDeque<int> deque;
    EXPECT_THROW(deque.get_first(), std::out_of_range);
    EXPECT_THROW(deque.get_last(), std::out_of_range);
    EXPECT_THROW(deque.get(0), std::out_of_range);
}

TEST(SegmentedDequeTest, PopFromEmptyThrows) {
    MutableSegmentedDeque<int> deque;
    int result;
    EXPECT_THROW(deque.pop_back(&result), std::out_of_range);
    EXPECT_THROW(deque.pop_front(&result), std::out_of_range);
}

TEST(SegmentedDequeTest, GetOutOfRangeThrows) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);
    EXPECT_THROW(deque.get(-1), std::out_of_range);
    EXPECT_THROW(deque.get(3), std::out_of_range);
    EXPECT_THROW(deque.get(100), std::out_of_range);
}

TEST(SegmentedDequeTest, SubSequenceInvalidRangeThrows) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);
    EXPECT_THROW(deque.get_sub_sequence(-1, 2), std::out_of_range);
    EXPECT_THROW(deque.get_sub_sequence(0, 5), std::out_of_range);
    EXPECT_THROW(deque.get_sub_sequence(2, 1), std::out_of_range);
}

TEST(SegmentedDequeTest, InsertAtInvalidIndexThrows) {
    int items[] = {1, 2, 3};
    MutableSegmentedDeque<int> deque(items, 3);
    EXPECT_THROW(deque.insert_at(0, -1), std::out_of_range);
    EXPECT_THROW(deque.insert_at(0, 5), std::out_of_range);
}

// =================== Граничные случаи

TEST(SegmentedDequeTest, PushPopUntilEmpty) {
    MutableSegmentedDeque<int> deque;
    for (int idx = 0; idx < 10; idx++) {
        deque.push_back(idx);
    }

    int result;
    for (int idx = 0; idx < 10; idx++) {
        deque.pop_front(&result);
        EXPECT_EQ(result, idx);
    }
    EXPECT_EQ(deque.get_count(), 0);
}

TEST(SegmentedDequeTest, PushPopAlternating) {
    MutableSegmentedDeque<int> deque;
    int result;

    for (int idx = 0; idx < 50; idx++) {
        deque.push_back(idx);
        deque.push_front(-idx);
    }
    // Дек: [-49, -48, ..., -1, 0, 0, 1, 2, ..., 49]
    EXPECT_EQ(deque.get_count(), 100);

    deque.pop_front(&result);
    EXPECT_EQ(result, -49);
    deque.pop_back(&result);
    EXPECT_EQ(result, 49);
}

TEST(SegmentedDequeTest, LargeDeque) {
    MutableSegmentedDeque<int> deque;
    int n = 1000;

    for (int idx = 0; idx < n; idx++) {
        deque.push_back(idx);
    }

    EXPECT_EQ(deque.get_count(), n);
    EXPECT_EQ(deque.get_first(), 0);
    EXPECT_EQ(deque.get_last(), n - 1);

    for (int idx = 0; idx < n; idx++) {
        EXPECT_EQ(deque.get(idx), idx);
    }
}

// =================== Immutable

TEST(SegmentedDequeTest, ImmutablePushBackReturnsNew) {
    int items[] = {1, 2, 3};
    ImmutableSegmentedDeque<int> deque(items, 3);

    SegmentedDeque<int>* result = deque.push_back(4);
    EXPECT_EQ(deque.get_count(), 3);     // Оригинал не изменился
    EXPECT_EQ(result->get_count(), 4);   // Новый дек с 4 элементами
    EXPECT_EQ(result->get(3), 4);
    delete result;
}

TEST(SegmentedDequeTest, ImmutablePopFrontReturnsNew) {
    int items[] = {1, 2, 3};
    ImmutableSegmentedDeque<int> deque(items, 3);

    int val;
    SegmentedDeque<int>* result = deque.pop_front(&val);
    EXPECT_EQ(val, 1);
    EXPECT_EQ(deque.get_count(), 3);     // Оригинал не изменился
    EXPECT_EQ(result->get_count(), 2);
    delete result;
}

TEST(SegmentedDequeTest, ImmutableAppendReturnsNew) {
    int items[] = {1, 2};
    ImmutableSegmentedDeque<int> deque(items, 2);

    Sequence<int>* result = deque.append(3);
    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_EQ(result->get_count(), 3);
    delete result;
}

// =================== Работа с Complex

TEST(SegmentedDequeTest, ComplexType) {
    Complex items[] = {Complex(1, 2), Complex(3, 4), Complex(5, 6)};
    MutableSegmentedDeque<Complex> deque(items, 3);

    EXPECT_EQ(deque.get_count(), 3);
    EXPECT_EQ(deque.get(0), Complex(1, 2));
    EXPECT_EQ(deque.get(1), Complex(3, 4));
    EXPECT_EQ(deque.get(2), Complex(5, 6));
}

TEST(SegmentedDequeTest, ComplexSort) {
    // Модули: sqrt(9+16)=5, sqrt(1+1)≈1.41, sqrt(9+1)≈3.16
    Complex items[] = {Complex(3, 4), Complex(1, 1), Complex(3, 1)};
    MutableSegmentedDeque<Complex> deque(items, 3);

    deque.sort();
    EXPECT_EQ(deque.get(0), Complex(1, 1));   // модуль ~1.41
    EXPECT_EQ(deque.get(1), Complex(3, 1));   // модуль ~3.16
    EXPECT_EQ(deque.get(2), Complex(3, 4));   // модуль 5
}

TEST(SegmentedDequeTest, ComplexFindSubSequence) {
    Complex items[] = {Complex(1, 0), Complex(2, 0), Complex(3, 0), Complex(4, 0)};
    Complex sub_items[] = {Complex(2, 0), Complex(3, 0)};
    MutableSegmentedDeque<Complex> deque(items, 4);
    MutableSegmentedDeque<Complex> sub(sub_items, 2);

    EXPECT_EQ(deque.find_sub_sequence(&sub), 1);
}

// =================== Работа со string

TEST(SegmentedDequeTest, StringType) {
    std::string items[] = {"hello", "world", "foo"};
    MutableSegmentedDeque<std::string> deque(items, 3);

    EXPECT_EQ(deque.get_count(), 3);
    EXPECT_EQ(deque.get(0), "hello");
    EXPECT_EQ(deque.get(1), "world");
    EXPECT_EQ(deque.get(2), "foo");

    deque.push_front("start");
    deque.push_back("end");
    EXPECT_EQ(deque.get_count(), 5);
    EXPECT_EQ(deque.get_first(), "start");
    EXPECT_EQ(deque.get_last(), "end");
}

TEST(SegmentedDequeTest, StringSort) {
    std::string items[] = {"banana", "apple", "cherry"};
    MutableSegmentedDeque<std::string> deque(items, 3);

    deque.sort();
    EXPECT_EQ(deque.get(0), "apple");
    EXPECT_EQ(deque.get(1), "banana");
    EXPECT_EQ(deque.get(2), "cherry");
}

// =================== Работа с Person, Teacher, Student

TEST(SegmentedDequeTest, PersonDeque) {
    Person p1(Person_ID{1, 100}, (char*)"Ivan", (char*)"Ivanovich", (char*)"Ivanov");
    Person p2(Person_ID{2, 200}, (char*)"Petr", (char*)"Petrovich", (char*)"Petrov");
    Person p3(Person_ID{1, 100}, (char*)"Ivan", (char*)"Ivanovich", (char*)"Ivanov");
 
    MutableSegmentedDeque<Person> deque;
    deque.push_back(p1);
    deque.push_back(p2);
 
    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_TRUE(deque.get(0) == p1);
    EXPECT_TRUE(deque.get(1) == p2);
    EXPECT_TRUE(deque.get(0) == p3); // одинаковые id
}
 
TEST(SegmentedDequeTest, PersonSort) {
    Person p1(Person_ID{3, 100}, (char*)"C", (char*)"C", (char*)"C");
    Person p2(Person_ID{1, 200}, (char*)"A", (char*)"A", (char*)"A");
    Person p3(Person_ID{2, 300}, (char*)"B", (char*)"B", (char*)"B");
 
    MutableSegmentedDeque<Person> deque;
    deque.push_back(p1);
    deque.push_back(p2);
    deque.push_back(p3);
 
    deque.sort();
    EXPECT_TRUE(deque.get(0) == p2); // id{1, 200}
    EXPECT_TRUE(deque.get(1) == p3); // id{2, 300}
    EXPECT_TRUE(deque.get(2) == p1); // id{3, 100}
}
 
TEST(SegmentedDequeTest, PersonCopyIndependence) {
    Person p1(Person_ID{1, 100}, (char*)"Ivan", (char*)"Ivanovich", (char*)"Ivanov");
 
    MutableSegmentedDeque<Person> deque;
    deque.push_back(p1);
 
    MutableSegmentedDeque<Person> copy(deque);
    EXPECT_EQ(copy.get_count(), 1);
    EXPECT_TRUE(copy.get(0) == p1);
 
    Person p2(Person_ID{2, 200}, (char*)"Petr", (char*)"Petrovich", (char*)"Petrov");
    deque.push_back(p2);
    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_EQ(copy.get_count(), 1); // Копия не изменилась
}
 
TEST(SegmentedDequeTest, StudentDeque) {
    Student s1(Person_ID{1, 100}, (char*)"Ivan", (char*)"I", (char*)"Ivanov", 12345, (char*)"M3100");
    Student s2(Person_ID{2, 200}, (char*)"Petr", (char*)"P", (char*)"Petrov", 67890, (char*)"M3101");
 
    MutableSegmentedDeque<Student> deque;
    deque.push_back(s1);
    deque.push_back(s2);
 
    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_EQ(deque.get(0).get_grade_book_num(), 12345);
    EXPECT_EQ(deque.get(1).get_grade_book_num(), 67890);
}
 
TEST(SegmentedDequeTest, StudentPopFront) {
    Student s1(Person_ID{1, 100}, (char*)"Ivan", (char*)"I", (char*)"Ivanov", 111, (char*)"G1");
    Student s2(Person_ID{2, 200}, (char*)"Petr", (char*)"P", (char*)"Petrov", 222, (char*)"G2");
 
    MutableSegmentedDeque<Student> deque;
    deque.push_back(s1);
    deque.push_back(s2);
 
    Student result;
    deque.pop_front(&result);
    EXPECT_EQ(result.get_grade_book_num(), 111);
    EXPECT_EQ(deque.get_count(), 1);
}
 
TEST(SegmentedDequeTest, TeacherDeque) {
    Teacher t1(Person_ID{1, 100}, (char*)"Anna", (char*)"A", (char*)"Sidorova", 10, (char*)"Professor");
    Teacher t2(Person_ID{2, 200}, (char*)"Boris", (char*)"B", (char*)"Borisov", 20, (char*)"Docent");
 
    MutableSegmentedDeque<Teacher> deque;
    deque.push_back(t1);
    deque.push_back(t2);
 
    EXPECT_EQ(deque.get_count(), 2);
    EXPECT_EQ(deque.get(0).get_depart_num(), 10);
    EXPECT_EQ(deque.get(1).get_depart_num(), 20);
}
 
TEST(SegmentedDequeTest, TeacherFindSubSequence) {
    Teacher t1(Person_ID{1, 100}, (char*)"A", (char*)"A", (char*)"A", 10, (char*)"Prof");
    Teacher t2(Person_ID{2, 200}, (char*)"B", (char*)"B", (char*)"B", 20, (char*)"Doc");
    Teacher t3(Person_ID{3, 300}, (char*)"C", (char*)"C", (char*)"C", 30, (char*)"Asst");
 
    MutableSegmentedDeque<Teacher> deque;
    deque.push_back(t1);
    deque.push_back(t2);
    deque.push_back(t3);
 
    MutableSegmentedDeque<Teacher> sub;
    sub.push_back(t2);
    sub.push_back(t3);
 
    EXPECT_EQ(deque.find_sub_sequence(&sub), 1);
}

// =================== Работа с указателями на функции
 
int inc1(int x) { return x + 1; }
int inc2(int x) { return x + 2; }
int inc3(int x) { return x + 3; }
int mul2(int x) { return x * 2; }
 
TEST(SegmentedDequeTest, FunctionPointerDeque) {
    MutableSegmentedDeque<int(*)(int)> deque;
    deque.push_back(&inc1);
    deque.push_back(&inc2);
    deque.push_back(&inc3);
 
    EXPECT_EQ(deque.get_count(), 3);
 
    // Извлекаем функцию и вызываем
    int (*f)(int) = deque.get(0);
    EXPECT_EQ(f(0), 1);
 
    f = deque.get(1);
    EXPECT_EQ(f(0), 2);
 
    f = deque.get(2);
    EXPECT_EQ(f(0), 3);
}
 
TEST(SegmentedDequeTest, FunctionPointerApplyAll) {
    MutableSegmentedDeque<int(*)(int)> deque;
    deque.push_back(&inc1);
    deque.push_back(&mul2);
    deque.push_back(&inc3);
 
    // Применяем все функции к значению 10
    int value = 10;
    int results[] = {11, 20, 13}; // inc1(10)=11, mul2(10)=20, inc3(10)=13
 
    for (int idx = 0; idx < deque.get_count(); idx++) {
        int (*f)(int) = deque.get(idx);
        EXPECT_EQ(f(value), results[idx]);
    }
}
 
TEST(SegmentedDequeTest, FunctionPointerPushPopFront) {
    MutableSegmentedDeque<int(*)(int)> deque;
    deque.push_back(&inc1);
    deque.push_back(&inc2);
    deque.push_front(&inc3);
 
    // Порядок: inc3, inc1, inc2
    EXPECT_EQ(deque.get(0)(0), 3); // inc3(0) = 3
    EXPECT_EQ(deque.get(1)(0), 1); // inc1(0) = 1
    EXPECT_EQ(deque.get(2)(0), 2); // inc2(0) = 2
 
    int (*result)(int);
    deque.pop_front(&result);
    EXPECT_EQ(result(5), 8); // inc3(5) = 8
    EXPECT_EQ(deque.get_count(), 2);
}
 
TEST(SegmentedDequeTest, FunctionPointerFindSubSequence) {
    MutableSegmentedDeque<int(*)(int)> deque;
    deque.push_back(&inc1);
    deque.push_back(&inc2);
    deque.push_back(&inc3);
    deque.push_back(&mul2);
 
    MutableSegmentedDeque<int(*)(int)> sub;
    sub.push_back(&inc2);
    sub.push_back(&inc3);
 
    EXPECT_EQ(deque.find_sub_sequence(&sub), 1);
}
 
TEST(SegmentedDequeTest, FunctionPointerEquality) {
    MutableSegmentedDeque<int(*)(int)> deque;
    deque.push_back(&inc1);
    deque.push_back(&inc1); // дубликат
 
    // Одинаковые указатели на функции должны быть равны
    EXPECT_EQ(deque.get(0), deque.get(1));
}
 
TEST(SegmentedDequeTest, FunctionPointerConcat) {
    MutableSegmentedDeque<int(*)(int)> deque1;
    deque1.push_back(&inc1);
    deque1.push_back(&inc2);
 
    MutableSegmentedDeque<int(*)(int)> deque2;
    deque2.push_back(&inc3);
    deque2.push_back(&mul2);
 
    MutableSegmentedDeque<int(*)(int)>* result = deque1.concat_defined(&deque2);
    EXPECT_EQ(result->get_count(), 4);
    EXPECT_EQ(result->get(0)(0), 1); // inc1
    EXPECT_EQ(result->get(1)(0), 2); // inc2
    EXPECT_EQ(result->get(2)(0), 3); // inc3
    EXPECT_EQ(result->get(3)(0), 0); // mul2(0) = 0
    delete result;
}

//  =================== Enumerator

TEST(SegmentedDequeTest, Enumerator) {
    int items[] = {10, 20, 30};
    MutableSegmentedDeque<int> deque(items, 3);

    IEnumerator<int>* iter = deque.get_enumerator();
    
    EXPECT_TRUE(iter->move_next());
    EXPECT_EQ(iter->get_current(), 10);
    EXPECT_TRUE(iter->move_next());
    EXPECT_EQ(iter->get_current(), 20);
    EXPECT_TRUE(iter->move_next());
    EXPECT_EQ(iter->get_current(), 30);
    EXPECT_FALSE(iter->move_next());

    delete iter;
}

TEST(SegmentedDequeTest, EnumeratorReset) {
    int items[] = {1, 2};
    MutableSegmentedDeque<int> deque(items, 2);

    IEnumerator<int>* iter = deque.get_enumerator();
    iter->move_next();
    iter->move_next();
    iter->reset();
    EXPECT_TRUE(iter->move_next());
    EXPECT_EQ(iter->get_current(), 1);

    delete iter;
}

// =================== Sorting station

void expect_wagon(const MutableSegmentedDeque<Wagon>& train, int index, int type, int number) {
    EXPECT_EQ(train.get(index).first(), type);
    EXPECT_EQ(train.get(index).second(), number);
}

TEST(SortingStationTest, ExampleFromTask) {
    MutableSegmentedDeque<Wagon> train;

    train.push_back(Wagon(2, 1));
    train.push_back(Wagon(3, 2));
    train.push_back(Wagon(3, 3));
    train.push_back(Wagon(1, 4));
    train.push_back(Wagon(2, 5));
    train.push_back(Wagon(3, 6));
    train.push_back(Wagon(2, 7));
    train.push_back(Wagon(1, 8));

    MutableSegmentedDeque<Wagon> result = sort_train_by_type(train);

    EXPECT_EQ(result.get_count(), 8);
    expect_wagon(result, 0, 2, 1);
    expect_wagon(result, 1, 2, 5);
    expect_wagon(result, 2, 2, 7);
    expect_wagon(result, 3, 3, 2);
    expect_wagon(result, 4, 3, 3);
    expect_wagon(result, 5, 3, 6);
    expect_wagon(result, 6, 1, 4);
    expect_wagon(result, 7, 1, 8);

    EXPECT_EQ(count_wagon_types(train), 3);
}

TEST(SortingStationTest, OneTypeKeepsOriginalOrder) {
    MutableSegmentedDeque<Wagon> train;

    train.push_back(Wagon(1, 1));
    train.push_back(Wagon(1, 2));
    train.push_back(Wagon(1, 3));

    MutableSegmentedDeque<Wagon> result = sort_train_by_type(train);

    EXPECT_EQ(result.get_count(), 3);
    expect_wagon(result, 0, 1, 1);
    expect_wagon(result, 1, 1, 2);
    expect_wagon(result, 2, 1, 3);

    EXPECT_EQ(count_wagon_types(train), 1);
}

TEST(SortingStationTest, AlreadyGroupedTrainDoesNotChange) {
    MutableSegmentedDeque<Wagon> train;

    train.push_back(Wagon(2, 1));
    train.push_back(Wagon(2, 2));
    train.push_back(Wagon(3, 3));
    train.push_back(Wagon(3, 4));
    train.push_back(Wagon(1, 5));

    MutableSegmentedDeque<Wagon> result = sort_train_by_type(train);

    EXPECT_EQ(result.get_count(), 5);
    expect_wagon(result, 0, 2, 1);
    expect_wagon(result, 1, 2, 2);
    expect_wagon(result, 2, 3, 3);
    expect_wagon(result, 3, 3, 4);
    expect_wagon(result, 4, 1, 5);

    EXPECT_EQ(count_wagon_types(train), 3);
}

TEST(SortingStationTest, AlternatingTypesKeepNumbersInsideGroups) {
    MutableSegmentedDeque<Wagon> train;

    train.push_back(Wagon(1, 1));
    train.push_back(Wagon(2, 2));
    train.push_back(Wagon(1, 3));
    train.push_back(Wagon(2, 4));
    train.push_back(Wagon(1, 5));
    train.push_back(Wagon(2, 6));

    MutableSegmentedDeque<Wagon> result = sort_train_by_type(train);

    EXPECT_EQ(result.get_count(), 6);
    expect_wagon(result, 0, 1, 1);
    expect_wagon(result, 1, 1, 3);
    expect_wagon(result, 2, 1, 5);
    expect_wagon(result, 3, 2, 2);
    expect_wagon(result, 4, 2, 4);
    expect_wagon(result, 5, 2, 6);

    EXPECT_EQ(count_wagon_types(train), 2);
}

TEST(SortingStationTest, EmptyTrain) {
    MutableSegmentedDeque<Wagon> train;

    MutableSegmentedDeque<Wagon> result = sort_train_by_type(train);

    EXPECT_EQ(result.get_count(), 0);
    EXPECT_EQ(count_wagon_types(train), 0);
}