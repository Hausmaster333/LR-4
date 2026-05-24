#include "lazy/lazy_sequence.h"
#include "lazy/sliding_cache.h"
#include <gtest/gtest.h>

TEST(Test, Test) {
    LazySequence<int> c(6);
    c.get_first();
}
// ================ SlidingCache tests

TEST(SlidingCacheTest, ZeroCapacityThrows) {
    EXPECT_THROW(SlidingCache<int>(0), std::invalid_argument);
}

TEST(SlidingCacheTest, EmptyAfterConstruction) {
    SlidingCache<int> c(4);

    EXPECT_TRUE(c.is_empty());
    EXPECT_EQ(c.get_count(), 0);
    EXPECT_EQ(c.get_capacity(), 4);
    EXPECT_THROW(c.get_first_index(), std::logic_error);
    EXPECT_THROW(c.get_last_index(), std::logic_error);
    EXPECT_FALSE(c.contains(0));
}

TEST(SlidingCacheTest, PushFillsWindow) {
    SlidingCache<int> c(4);

    c.push(10);
    c.push(20);
    c.push(30);

    EXPECT_FALSE(c.is_empty());
    EXPECT_EQ(c.get_count(), 3);
    EXPECT_EQ(c.get_first_index(), 0u);
    EXPECT_EQ(c.get_last_index(), 2u);
    EXPECT_EQ(c.get(0), 10);
    EXPECT_EQ(c.get(1), 20);
    EXPECT_EQ(c.get(2), 30);
    EXPECT_TRUE(c.contains(0));
    EXPECT_TRUE(c.contains(2));
    EXPECT_FALSE(c.contains(3));
}

TEST(SlidingCacheTest, PushExactlyCapacity) {
    SlidingCache<int> c(3);
    c.push(1); c.push(2); c.push(3);

    EXPECT_EQ(c.get_count(), 3);
    EXPECT_EQ(c.get_first_index(), 0u);
    EXPECT_EQ(c.get_last_index(), 2u);
    EXPECT_EQ(c.get(0), 1);
    EXPECT_EQ(c.get(2), 3);
}

TEST(SlidingCacheTest, EvictsOldestWhenOverflow) {
    SlidingCache<int> c(3);
    // push 0-5 и должно остаться окно [3, 4, 5]
    for (int i = 0; i < 6; ++i) c.push(i * 10);

    EXPECT_EQ(c.get_count(), 3);
    EXPECT_EQ(c.get_first_index(), 3u);
    EXPECT_EQ(c.get_last_index(), 5u);
    EXPECT_EQ(c.get(3), 30);
    EXPECT_EQ(c.get(4), 40);
    EXPECT_EQ(c.get(5), 50);
}

TEST(SlidingCacheTest, AtThrowsOnEvictedIndex) {
    SlidingCache<int> c(2);
    c.push(1); c.push(2); c.push(3);  // окно [1, 2]

    EXPECT_THROW(c.get(0), std::out_of_range);   // вытеснен
    EXPECT_FALSE(c.contains(0));
    EXPECT_TRUE(c.contains(1));
    EXPECT_TRUE(c.contains(2));
}

TEST(SlidingCacheTest, AtThrowsOnFutureIndex) {
    SlidingCache<int> c(4);
    c.push(1); c.push(2);

    EXPECT_THROW(c.get(2), std::out_of_range);
    EXPECT_THROW(c.get(100), std::out_of_range);
}

TEST(SlidingCacheTest, ClearResetsState) {
    SlidingCache<int> c(3);
    c.push(1); c.push(2);

    c.clear();

    EXPECT_TRUE(c.is_empty());
    EXPECT_EQ(c.get_count(), 0);
    EXPECT_THROW(c.get(0), std::out_of_range);

    // после clear можно начинать заново с индекса 0
    c.push(99);
    EXPECT_EQ(c.get_first_index(), 0u);
    EXPECT_EQ(c.get_last_index(), 0u);
    EXPECT_EQ(c.get(0), 99);
}

TEST(SlidingCacheTest, RingWrapsCorrectly) {
    SlidingCache<int> c(3);
    // Заполняем 0-9, окно постоянно сдвигается через границу физического кольца
    for (int i = 0; i < 10; ++i) c.push(i);

    EXPECT_EQ(c.get_first_index(), 7u);
    EXPECT_EQ(c.get_last_index(), 9u);
    EXPECT_EQ(c.get(7), 7);
    EXPECT_EQ(c.get(8), 8);
    EXPECT_EQ(c.get(9), 9);
}

// ================ Ordinal tests

TEST(OrdinalTest, FiniteOrdinal) {
    Ordinal c = Ordinal::finite(5);

    EXPECT_TRUE(c.is_finite());
    EXPECT_FALSE(c.is_infinite());
    EXPECT_EQ(c.get_value(), 5u);
}

TEST(OrdinalTest, InfiniteOrdinal) {
    Ordinal c = Ordinal::infinity();

    EXPECT_TRUE(c.is_infinite());
    EXPECT_FALSE(c.is_finite());
    EXPECT_THROW(c.get_value(), std::logic_error);
}

TEST(OrdinalTest, Equality) {
    EXPECT_EQ(Ordinal::finite(3), Ordinal::finite(3));
    EXPECT_NE(Ordinal::finite(3), Ordinal::finite(4));
    EXPECT_EQ(Ordinal::infinity(), Ordinal::infinity());
    EXPECT_NE(Ordinal::finite(1000000), Ordinal::infinity());
}

// ================ LazySequence tests

TEST(LazySequenceTest, EmptyConstructor) {
    LazySequence<int> seq;

    EXPECT_EQ(seq.get_length(), Ordinal::finite(0));
    EXPECT_EQ(seq.get_materialized_count(), 0);
}

TEST(LazySequenceTest, ArrayConstructor) {
    int items[] = {1, 2, 3};

    LazySequence<int> seq(items, 3);

    EXPECT_EQ(seq.get_length(), Ordinal::finite(3));
    // Крконструктор не материализует элементы сразу и кэш изначально пуст, материализация при get().
    EXPECT_EQ(seq.get_materialized_count(), 0);

    seq.get(2);

    EXPECT_EQ(seq.get_materialized_count(), 3);
}

TEST(LazySequenceTest, SequenceConstructor) {
    int items[] = {10, 20, 30};
    MutableArraySequence<int> source(items, 3);

    LazySequence<int> seq(&source);

    EXPECT_EQ(seq.get_length(), Ordinal::finite(3));
    EXPECT_EQ(seq.get_materialized_count(), 0);

    seq.get(2);

    EXPECT_EQ(seq.get_materialized_count(), 3);
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

    EXPECT_EQ(seq.get_length(), Ordinal::finite(3));
    EXPECT_EQ(seq.get_last(), 3);

    EXPECT_EQ(result->get_length(), Ordinal::finite(4));
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

    EXPECT_EQ(seq.get_length(), Ordinal::finite(3));
    EXPECT_EQ(seq.get_first(), 2);

    EXPECT_EQ(result->get_length(), Ordinal::finite(4));
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

    EXPECT_EQ(seq.get_length(), Ordinal::finite(3));
    EXPECT_EQ(seq.get(2), 4);

    EXPECT_EQ(result->get_length(), Ordinal::finite(4));
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

    EXPECT_EQ(result->get_length(), Ordinal::finite(3));
    EXPECT_EQ(result->get(0), 20);
    EXPECT_EQ(result->get(1), 30);
    EXPECT_EQ(result->get(2), 40);

    delete result;
}

TEST(LazySequenceTest, GetSubSequenceSingleElement) {
    int items[] = {10, 20, 30};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.get_sub_sequence(1, 1);

    EXPECT_EQ(result->get_length(), Ordinal::finite(1));
    EXPECT_EQ(result->get(0), 20);

    delete result;
}

TEST(LazySequenceTest, GetSubSequenceFullRange) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    LazySequence<int>* result = seq.get_sub_sequence(0, 2);

    EXPECT_EQ(result->get_length(), Ordinal::finite(3));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);

    delete result;
}

TEST(LazySequenceTest, InfiniteFibonacci_Basic) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto fib_rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(fib_rule, &initial, /*cache_capacity*/ 16);

    EXPECT_TRUE(fib.get_length().is_infinite());
    EXPECT_EQ(fib.get(0), 0);
    EXPECT_EQ(fib.get(1), 1);
    EXPECT_EQ(fib.get(2), 1);
    EXPECT_EQ(fib.get(3), 2);
    EXPECT_EQ(fib.get(4), 3);
    EXPECT_EQ(fib.get(5), 5);
    EXPECT_EQ(fib.get(9), 34);
    EXPECT_EQ(fib.get(15), 610);
}

TEST(LazySequenceTest, InfiniteFibonacci_GetCountThrows) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    EXPECT_THROW(fib.get_count(), std::logic_error);
    EXPECT_THROW(fib.get_last(), std::logic_error);
    EXPECT_FALSE(fib.try_get_last().has_value());
}

TEST(LazySequenceTest, InfiniteFibonacci_GetFirstWorks) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    EXPECT_EQ(fib.get_first(), 0);
    EXPECT_TRUE(fib.try_get_first().has_value());
    EXPECT_EQ(fib.try_get_first().get_value(), 0);
}

TEST(LazySequenceTest, InfiniteFibonacci_EvictionAfterLargeIndex) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial, /*cache_capacity*/ 4);

    EXPECT_EQ(fib.get(9), 34);                     // окно [6..9]
    EXPECT_THROW(fib.get(0), std::out_of_range);   // 0 вытеснен
    EXPECT_THROW(fib.get(5), std::out_of_range);   // 5 вытеснен
    EXPECT_EQ(fib.get(8), 21);                     // 8 ещё в окне
    EXPECT_EQ(fib.get(9), 34);
}

TEST(LazySequenceTest, AppendOnInfinite_TailHangs) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    // append на бесконечной — отложенная операция, длина остаётся бесконечной
    LazySequence<int>* fib_plus = fib.append(999);
    EXPECT_TRUE(fib_plus->get_length().is_infinite());

    // base работает как раньше
    EXPECT_EQ(fib_plus->get(0), 0);
    EXPECT_EQ(fib_plus->get(9), 34);

    delete fib_plus;
}

TEST(LazySequenceTest, AppendOnInfinite_OrdinallyAccessible) {
    // Унифицированная модель: append(item) на ω даёт ω+1.
    // Element доступен через get(Ordinal(1, 0)), а не через "magic take with tail".
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazySequence<int>* once = fib.append(999);
    LazySequence<int>* extended = once->append(1000);
    delete once;

    // Длина теперь честная: ω + 2 (два append-а после ω-блока)
    Ordinal L = extended->get_length();
    EXPECT_TRUE(L.is_infinite());
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 2u);

    // Линейно видим только fib (ω-блок)
    EXPECT_EQ(extended->get(0), 0);
    EXPECT_EQ(extended->get(9), 34);

    // Appended элементы доступны через ординальный индекс
    EXPECT_EQ(extended->get(Ordinal(1, 0)), 999);
    EXPECT_EQ(extended->get(Ordinal(1, 1)), 1000);

    // take(5) теперь даёт ТОЛЬКО первые 5 (без auto-append).
    // Для "first 5 + appended" — собрать вручную через take + concat.
    LazySequence<int>* prefix = extended->take(5);
    EXPECT_EQ(prefix->get_count(), 5);
    EXPECT_EQ(prefix->get(0), 0);
    EXPECT_EQ(prefix->get(4), 3);

    delete prefix;
    delete extended;
}

TEST(LazySequenceTest, ConcatFiniteWithFinite) {
    int a[] = {1, 2, 3};
    int b[] = {10, 20};
    LazySequence<int> la(a, 3);
    LazySequence<int> lb(b, 2);

    LazySequence<int>* result = la.concat(&lb);

    EXPECT_EQ(result->get_length(), Ordinal::finite(5));
    EXPECT_EQ(result->get(0), 1);
    EXPECT_EQ(result->get(1), 2);
    EXPECT_EQ(result->get(2), 3);
    EXPECT_EQ(result->get(3), 10);
    EXPECT_EQ(result->get(4), 20);

    delete result;
}

TEST(LazySequenceTest, PrependOnInfinite) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazySequence<int>* shifted = fib.prepend(-1);
    EXPECT_TRUE(shifted->get_length().is_infinite());

    EXPECT_EQ(shifted->get(0), -1);
    EXPECT_EQ(shifted->get(1), 0);
    EXPECT_EQ(shifted->get(2), 1);
    EXPECT_EQ(shifted->get(3), 1);
    EXPECT_EQ(shifted->get(4), 2);
    EXPECT_EQ(shifted->get(10), 34);   // fib(9) = 34, теперь на индексе 10

    delete shifted;
}

TEST(LazySequenceTest, InsertAtMiddleOfInfinite) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    // вставляем 99 на позицию 3: было [0,1,1,2,3,...], станет [0,1,1,99,2,3,...]
    LazySequence<int>* modified = fib.insert_at(99, 3);
    EXPECT_TRUE(modified->get_length().is_infinite());

    EXPECT_EQ(modified->get(0), 0);
    EXPECT_EQ(modified->get(1), 1);
    EXPECT_EQ(modified->get(2), 1);
    EXPECT_EQ(modified->get(3), 99);
    EXPECT_EQ(modified->get(4), 2);
    EXPECT_EQ(modified->get(5), 3);

    delete modified;
}

TEST(LazySequenceTest, TakeOnFiniteEqualsCopy) {
    int items[] = {10, 20, 30, 40, 50};
    LazySequence<int> seq(items, 5);

    LazySequence<int>* taken = seq.take(3);
    EXPECT_EQ(taken->get_length(), Ordinal::finite(3));
    EXPECT_EQ(taken->get(0), 10);
    EXPECT_EQ(taken->get(1), 20);
    EXPECT_EQ(taken->get(2), 30);

    delete taken;
}

TEST(LazySequenceTest, TakeMoreThanLengthThrows) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    EXPECT_THROW(seq.take(5), std::out_of_range);
}

// ================ Комбинаторы

TEST(LazySequenceTest, MapOfInfinite_TakeWorks) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazySequence<int>* squared = fib.map<int>([](const int& x) { return x * x; });
    EXPECT_TRUE(squared->get_length().is_infinite());

    LazySequence<int>* first5 = squared->take(5);
    EXPECT_EQ(first5->get_length(), Ordinal::finite(5));
    EXPECT_EQ(first5->get(0), 0);
    EXPECT_EQ(first5->get(1), 1);
    EXPECT_EQ(first5->get(2), 1);
    EXPECT_EQ(first5->get(3), 4);     // 2*2
    EXPECT_EQ(first5->get(4), 9);     // 3*3

    delete first5;
    delete squared;
}

TEST(LazySequenceTest, WhereOfInfinite_TakeFirstEvens) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazySequence<int>* evens = fib.where([](const int& x) { return x % 2 == 0; });
    EXPECT_TRUE(evens->get_length().is_infinite());

    // Первые чётные числа Фибоначчи: 0, 2, 8, 34, 144, ...
    LazySequence<int>* first3 = evens->take(3);
    EXPECT_EQ(first3->get(0), 0);
    EXPECT_EQ(first3->get(1), 2);
    EXPECT_EQ(first3->get(2), 8);

    delete first3;
    delete evens;
}

TEST(LazySequenceTest, ZipInfiniteWithFinite_LengthEqualsMin) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    int idxs[] = {100, 200, 300};
    LazySequence<int> finite_seq(idxs, 3);

    LazySequence<int>* zipped = fib.zip<int, int>(&finite_seq, [](const int& a, const int& b) { return a + b; });

    EXPECT_EQ(zipped->get_length(), Ordinal::finite(3));
    EXPECT_EQ(zipped->get(0), 100);   // 0 + 100
    EXPECT_EQ(zipped->get(1), 201);   // 1 + 200
    EXPECT_EQ(zipped->get(2), 301);   // 1 + 300

    delete zipped;
}

TEST(LazySequenceTest, ConcatFiniteWithInfinite_LengthInfinite) {
    int prefix[] = {100, 200};
    LazySequence<int> head(prefix, 2);

    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    LazySequence<int>* combined = head.concat(&fib);
    EXPECT_TRUE(combined->get_length().is_infinite());

    EXPECT_EQ(combined->get(0), 100);
    EXPECT_EQ(combined->get(1), 200);
    EXPECT_EQ(combined->get(2), 0);     // fib[0]
    EXPECT_EQ(combined->get(3), 1);     // fib[1]
    EXPECT_EQ(combined->get(4), 1);     // fib[2]
    EXPECT_EQ(combined->get(6), 3);     // fib[4]

    delete combined;
}

TEST(LazySequenceTest, ReduceOnFinite) {
    int items[] = {1, 2, 3, 4, 5};
    LazySequence<int> seq(items, 5);

    int sum = seq.reduce([](const int& a, const int& b) { return a + b; }, 0);
    EXPECT_EQ(sum, 15);

    int product = seq.reduce([](const int& a, const int& b) { return a * b; }, 1);
    EXPECT_EQ(product, 120);
}

TEST(LazySequenceTest, ReduceOnInfiniteThrows) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);

    EXPECT_THROW(fib.reduce([](const int& a, const int& b) { return a + b; }, 0), std::logic_error);
}

// ================ LazyEnumerator

TEST(LazySequenceTest, EnumeratorOverFinite_VisitsAll) {
    int items[] = {10, 20, 30, 40};
    LazySequence<int> seq(items, 4);

    EnumeratorWrapper<int> iter(seq.get_enumerator());
    int expected[] = {10, 20, 30, 40};
    int idx = 0;
    while (iter.move_next()) {
        EXPECT_EQ(iter.get_current(), expected[idx]);
        idx++;
    }
    EXPECT_EQ(idx, 4);
}

TEST(LazySequenceTest, EnumeratorOverTakeFromInfinite) {
    int init[] = {0, 1};
    MutableArraySequence<int> initial(init, 2);

    auto rule = [](Sequence<int>* window) -> int {
        return window->get_first() + window->get_last();
    };

    LazySequence<int> fib(rule, &initial);
    LazySequence<int>* taken = fib.take(7);

    EnumeratorWrapper<int> iter(taken->get_enumerator());
    int expected[] = {0, 1, 1, 2, 3, 5, 8};
    int idx = 0;
    while (iter.move_next()) {
        EXPECT_EQ(iter.get_current(), expected[idx]);
        idx++;
    }
    EXPECT_EQ(idx, 7);

    delete taken;
}

TEST(LazySequenceTest, EnumeratorOverEmpty_NoElements) {
    LazySequence<int> seq;

    EnumeratorWrapper<int> iter(seq.get_enumerator());
    EXPECT_FALSE(iter.move_next());
}

// ================ Проверка throws-overrides через Sequence<T>*

TEST(LazySequenceTest, ThrowOverridesAccessibleViaSequencePointer) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);
    Sequence<int>* base = &seq;

    EXPECT_THROW(base->slice(0, 1), std::logic_error);
    EXPECT_THROW(base->map(nullptr), std::logic_error);
    EXPECT_THROW(base->where(nullptr), std::logic_error);
}

TEST(LazySequenceTest, EvictionFromCache_BackwardJumpThrows) {
    int items[] = {10, 20, 30, 40, 50, 60, 70, 80};
    LazySequence<int> seq(items, 8, 3); // cache_capacity 3

    EXPECT_EQ(seq.get_cache_capacity(), 3);

    // Материализуем 0 - 2, окно [0-2]
    EXPECT_EQ(seq.get(0), 10);
    EXPECT_EQ(seq.get(1), 20);
    EXPECT_EQ(seq.get(2), 30);
    EXPECT_EQ(seq.get_materialized_count(), 3);

    // Двигаемся дальше и окно сдвигается, нижние индексы вытесняются
    EXPECT_EQ(seq.get(5), 60);
    EXPECT_EQ(seq.get_materialized_count(), 3); // cap остался 3
    EXPECT_EQ(seq.get(5), 60); // 5 ещё в окне
    EXPECT_EQ(seq.get(3), 40); // 3 ещё в окне (last=5, first=3)
}

TEST(LazySequenceTest, BackwardJumpAfterEvictionWorksViaOrdinalIndexable) {
    // SourceGenerator реализует OrdinalIndexable, поэтому backward jump
    // через cache-eviction fallback работает корректно.
    int items[] = {1, 2, 3, 4, 5};
    LazySequence<int> seq(items, 5, 2); // cache_capacity 2

    EXPECT_EQ(seq.get(4), 5); // окно [3, 4], индексы 0..2 вытеснены

    // Backward jump через OrdinalIndexable
    EXPECT_EQ(seq.get(0), 1);
    EXPECT_EQ(seq.get(1), 2);
    EXPECT_EQ(seq.get(2), 3);
    EXPECT_EQ(seq.get(3), 4);
    EXPECT_EQ(seq.get(4), 5);
}

TEST(LazySequenceTest, BackwardJumpAfterEvictionThrowsForRecurrence) {
    // RecurrenceGenerator не реализует OrdinalIndexable, backward jump
    // после eviction должен бросать.
    MutableArraySequence<int> initial;
    initial.append(0);
    auto rule = [](Sequence<int>* window) -> int { return window->get_last() + 1; };
    LazySequence<int> seq(rule, &initial, 2); // cache_capacity 2

    EXPECT_EQ(seq.get(10), 10); // окно сдвинулось
    EXPECT_THROW(seq.get(0), std::out_of_range);
}

TEST(LazySequenceTest, GetSubSequenceInvalidRangeThrows) {
    int items[] = {1, 2, 3};
    LazySequence<int> seq(items, 3);

    EXPECT_THROW(seq.get_sub_sequence(-1, 1), std::out_of_range);
    EXPECT_THROW(seq.get_sub_sequence(0, 3), std::out_of_range);
    EXPECT_THROW(seq.get_sub_sequence(2, 1), std::out_of_range);
}
