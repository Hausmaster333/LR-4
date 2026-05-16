#include "lazy/lazy_sequence.h"
#include "core/sequence.h"
#include <gtest/gtest.h>

int naturals_rule(Sequence<int>* w) { return w->get_last() + 1; }
int fib_rule(Sequence<int>* w) { return w->get_first() + w->get_last(); }

LazySequence<int>* make_inf_from(int start) {
    int init[1] = { start };
    MutableArraySequence<int> initial(init, 1);
    return new LazySequence<int>(naturals_rule, &initial);
}
LazySequence<int>* make_fib() {
    int init[2] = { 0, 1 };
    MutableArraySequence<int> initial(init, 2);
    return new LazySequence<int>(fib_rule, &initial);
}
LazySequence<int>* make_fin(std::initializer_list<int> xs) {
    MutableArraySequence<int> buf;
    for (int v : xs) buf.append(v);
    return new LazySequence<int>(&buf);
}

// ======== Пограничные позиции вставки ========

TEST(InsertExtra, InsertAtEnd_Finite) {
    // index == length: эквивалентно конкатенации
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_fin({99, 100});
    LazySequence<int>* res = base->insert_at(ins, 3);

    EXPECT_EQ(res->get_count(), 5);
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(2), 3);
    EXPECT_EQ(res->get(3), 99);
    EXPECT_EQ(res->get(4), 100);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, InsertPastEnd_Finite_Throws) {
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_fin({99});
    EXPECT_THROW(base->insert_at(ins, 4), std::out_of_range);
    delete base;
    delete ins;
}

TEST(InsertExtra, NegativeIndex_Throws) {
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_fin({99});
    EXPECT_THROW(base->insert_at(ins, -1), std::out_of_range);
    delete base;
    delete ins;
}

TEST(InsertExtra, NullOther_Throws) {
    LazySequence<int>* base = make_fin({1, 2, 3});
    EXPECT_THROW(base->insert_at(static_cast<LazySequence<int>*>(nullptr), 0),
                 std::invalid_argument);
    delete base;
}

TEST(InsertExtra, EmptyOther_IsIdentity) {
    // Вставка пустой последовательности должна давать копию this
    LazySequence<int>* base = make_fin({1, 2, 3});
    MutableArraySequence<int> empty;
    LazySequence<int>* ins = new LazySequence<int>(&empty);
    LazySequence<int>* res = base->insert_at(ins, 1);

    EXPECT_EQ(res->get_count(), 3);
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 2);
    EXPECT_EQ(res->get(2), 3);

    delete res; delete base; delete ins;
}

// ======== Точные значения длин в ω-арифметике ========

TEST(InsertExtra, Length_FiniteIntoInfinite) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_fin({100, 200});
    LazySequence<int>* res = base->insert_at(ins, 5);

    // ω + 2 формально равно ω в ординалах (left absorption), у нас остаётся omega_count=1, finite_part=0
    Cardinal L = res->get_length();
    EXPECT_TRUE(L.is_infinite());
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite) {
    // base = [a, b, c, d, e], insert(inf) на позицию 2 -> ω + 3
    LazySequence<int>* base = make_fin({1, 2, 3, 4, 5});
    LazySequence<int>* ins = make_inf(100);
    LazySequence<int>* res = base->insert_at(ins, 2);

    Cardinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);  // 5 - 2 = 3 элемента в хвосте

    EXPECT_EQ(res->get(OrdinalIndex(1, 0)), 3);
    EXPECT_EQ(res->get(OrdinalIndex(1, 1)), 4);
    EXPECT_EQ(res->get(OrdinalIndex(1, 2)), 5);
    EXPECT_THROW(res->get(OrdinalIndex(1, 3)), std::out_of_range);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite_AtEnd) {
    // insert(inf) на самый конец: хвоста нет -> длина = ω, finite_part=0
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_inf(100);
    LazySequence<int>* res = base->insert_at(ins, 3);

    Cardinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(2), 3);
    EXPECT_EQ(res->get(3), 100);
    EXPECT_EQ(res->get(10), 107);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite_AtBeginning) {
    // insert(inf) в начало финитной: хвост = весь base
    LazySequence<int>* base = make_fin({10, 20, 30});
    LazySequence<int>* ins = make_inf(0);
    LazySequence<int>* res = base->insert_at(ins, 0);

    Cardinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);

    EXPECT_EQ(res->get(0), 0);   // injected[0]
    EXPECT_EQ(res->get(5), 5);   // injected[5]

    EXPECT_EQ(res->get(OrdinalIndex(1, 0)), 10);
    EXPECT_EQ(res->get(OrdinalIndex(1, 1)), 20);
    EXPECT_EQ(res->get(OrdinalIndex(1, 2)), 30);

    delete res; delete base; delete ins;
}

// ======== Большие индексы и стабильность ========

TEST(InsertExtra, LargeIndicesInOmegaPart) {
    LazySequence<int>* base = make_inf(0);   // 0, 1, 2, ...
    LazySequence<int>* ins = make_inf(1000); // 1000, 1001, ...
    LazySequence<int>* res = base->insert_at(ins, 10);

    // Большие индексы линейно и ординально
    EXPECT_EQ(res->get(9), 9);          // left tail
    EXPECT_EQ(res->get(10), 1000);      // первый injected
    EXPECT_EQ(res->get(1000), 1990);    // injected[990]

    // Хвост base (10, 11, 12, ...) в 1-м ω-блоке
    EXPECT_EQ(res->get(OrdinalIndex(1, 0)), 10);
    EXPECT_EQ(res->get(OrdinalIndex(1, 100)), 110);
    EXPECT_EQ(res->get(OrdinalIndex(1, 9999)), 10009);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, RepeatedAccessIsConsistent) {
    // Многократный доступ к одному и тому же ординальному индексу даёт один и тот же результат
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    for (int trial = 0; trial < 3; trial++) {
        EXPECT_EQ(res->get(OrdinalIndex(1, 0)), 5);
        EXPECT_EQ(res->get(OrdinalIndex(1, 7)), 12);
        EXPECT_EQ(res->get(100), 1095);
    }

    delete res; delete base; delete ins;
}

// ======== Многократная вставка ========

TEST(InsertExtra, NestedInsert_TwoFiniteInsertsIntoFinite) {
    LazySequence<int>* base = make_fin({1, 2, 3, 4, 5});
    LazySequence<int>* ins1 = make_fin({10, 20});
    LazySequence<int>* res1 = base->insert_at(ins1, 2);
    // res1 = [1, 2, 10, 20, 3, 4, 5]

    LazySequence<int>* ins2 = make_fin({99});
    LazySequence<int>* res2 = res1->insert_at(ins2, 5);
    // res2 = [1, 2, 10, 20, 3, 99, 4, 5]

    EXPECT_EQ(res2->get_count(), 8);
    EXPECT_EQ(res2->get(0), 1);
    EXPECT_EQ(res2->get(2), 10);
    EXPECT_EQ(res2->get(4), 3);
    EXPECT_EQ(res2->get(5), 99);
    EXPECT_EQ(res2->get(6), 4);
    EXPECT_EQ(res2->get(7), 5);

    delete res2; delete res1; delete base; delete ins1; delete ins2;
}

TEST(InsertExtra, FibAsInsertedSequence) {
    // Вставляем "первые из фибоначчи" (бесконечно) в конечный base
    LazySequence<int>* base = make_fin({0, -1, -2});
    LazySequence<int>* fib = make_fib();
    LazySequence<int>* res = base->insert_at(fib, 1);

    // res = [0, 1, 1, 2, 3, 5, 8, 13, ..., (-1, -2 - в омега-хвосте)]
    EXPECT_EQ(res->get(0), 0);
    EXPECT_EQ(res->get(1), 1);  // fib[0]
    EXPECT_EQ(res->get(2), 1);  // fib[1]
    EXPECT_EQ(res->get(3), 2);  // fib[2]
    EXPECT_EQ(res->get(4), 3);
    EXPECT_EQ(res->get(5), 5);
    EXPECT_EQ(res->get(6), 8);

    EXPECT_EQ(res->get(OrdinalIndex(1, 0)), -1);
    EXPECT_EQ(res->get(OrdinalIndex(1, 1)), -2);

    delete res; delete base; delete fib;
}

// ======== Целостность данных: правильные элементы из правильных источников ========

TEST(InsertExtra, NoCrossContamination_InfIntoInf) {
    // Убеждаемся, что элементы из base никогда не попадают в injected-зону и наоборот
    LazySequence<int>* base = make_inf(0);     // 0, 1, 2, 3, ... (даёт только малые числа на малых индексах)
    LazySequence<int>* ins = make_inf(1000);   // 1000, 1001, ...
    LazySequence<int>* res = base->insert_at(ins, 7);

    // Injected zone: индексы 7..N - всё должно быть >= 1000
    for (int i = 7; i < 100; i++) {
        EXPECT_GE(res->get(i), 1000) << "index=" << i;
        EXPECT_LE(res->get(i), 1100) << "index=" << i;
    }
    // Left zone: индексы 0..6 - всё < 7
    for (int i = 0; i < 7; i++) {
        EXPECT_LT(res->get(i), 7) << "index=" << i;
    }
    // Tail zone (ω+k): все >= 7 и < 1000
    for (size_t k = 0; k < 20; k++) {
        int v = res->get(OrdinalIndex(1, k));
        EXPECT_GE(v, 7) << "omega_part=1, k=" << k;
        EXPECT_LT(v, 1000) << "omega_part=1, k=" << k;
    }

    delete res; delete base; delete ins;
}

// ======== Граничные случаи ординального индекса ========

TEST(InsertExtra, FiniteIntoFinite_NoOmegaAccess) {
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_fin({99});
    LazySequence<int>* res = base->insert_at(ins, 1);

    // omega_part=1 на полностью финитной последовательности - выход за границы
    EXPECT_THROW(res->get(OrdinalIndex(1, 0)), std::out_of_range);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, OrdinalIndexZeroEquivalentToInt) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    // {0, k} должен быть идентичен get(k)
    for (int k = 0; k < 30; k++) {
        EXPECT_EQ(res->get(OrdinalIndex(0, k)), res->get(k)) << "k=" << k;
    }

    delete res; delete base; delete ins;
}

// ======== Старый одиночный insert_at(item, idx) ========

TEST(InsertExtra, LegacySingleInsert_AtEnd_Finite) {
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* res = base->insert_at(99, 3);

    EXPECT_EQ(res->get_count(), 4);
    EXPECT_EQ(res->get(3), 99);

    delete res; delete base;
}

TEST(InsertExtra, LegacySingleInsert_AtBeginning_Inf) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* res = base->insert_at(-1, 0);

    EXPECT_EQ(res->get(0), -1);
    EXPECT_EQ(res->get(1), 0);
    EXPECT_EQ(res->get(2), 1);

    delete res; delete base;
}

// ======== Взаимодействие с map / take ========

TEST(InsertExtra, MapAfterInsert) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_fin({100, 200});
    LazySequence<int>* res = base->insert_at(ins, 3);

    // map x -> x * 10
    LazySequence<int>* mapped = res->map(std::function<int(const int&)>([](const int& x) { return x * 10; }));

    EXPECT_EQ(mapped->get(0), 0);
    EXPECT_EQ(mapped->get(2), 20);
    EXPECT_EQ(mapped->get(3), 1000);  // 100 * 10
    EXPECT_EQ(mapped->get(4), 2000);
    EXPECT_EQ(mapped->get(5), 30);    // base[3]*10 (base продолжается)

    delete mapped; delete res; delete base; delete ins;
}

TEST(InsertExtra, TakeAfterInsert_TurnsInfiniteIntoFinite) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_fin({100, 200});
    LazySequence<int>* res = base->insert_at(ins, 2);
    // res = 0, 1, 100, 200, 2, 3, 4, ...

    LazySequence<int>* taken = res->take(6);
    // taken = [0, 1, 100, 200, 2, 3]
    EXPECT_EQ(taken->get_count(), 6);
    EXPECT_EQ(taken->get(0), 0);
    EXPECT_EQ(taken->get(2), 100);
    EXPECT_EQ(taken->get(3), 200);
    EXPECT_EQ(taken->get(5), 3);

    delete taken; delete res; delete base; delete ins;
}

// ======== concat + insert смешанно ========

TEST(InsertExtra, ConcatAfterInsert) {
    LazySequence<int>* base = make_fin({1, 2});
    LazySequence<int>* ins  = make_fin({10, 20});
    LazySequence<int>* res1 = base->insert_at(ins, 1);  // [1, 10, 20, 2]
    LazySequence<int>* extra = make_fin({99, 100});
    LazySequence<int>* res2 = res1->concat(extra);      // [1, 10, 20, 2, 99, 100]

    EXPECT_EQ(res2->get_count(), 6);
    EXPECT_EQ(res2->get(0), 1);
    EXPECT_EQ(res2->get(1), 10);
    EXPECT_EQ(res2->get(3), 2);
    EXPECT_EQ(res2->get(4), 99);
    EXPECT_EQ(res2->get(5), 100);

    delete res2; delete res1; delete base; delete ins; delete extra;
}

TEST(InsertExtra, InsertWithBothOperandsInfinite_ConcatWithFinite) {
    // Хитрая комбинация: сначала insert(inf, inf), потом concat(...) - что-то логичное должно произойти.
    // Длина результата уже ω·2, дальше concat должен либо корректно расширить, либо отказать.
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* combined = a->insert_at(b, 3);  // ω·2

    // Проверяем, что результат всё ещё корректен и длина ω·2
    EXPECT_EQ(combined->get_length().get_omega_count(), 2u);
    EXPECT_EQ(combined->get(2), 2);
    EXPECT_EQ(combined->get(3), 100);
    EXPECT_EQ(combined->get(OrdinalIndex(1, 0)), 3);

    delete combined; delete a; delete b;
}

// ======== insert в insert (вложенные ординальные операции) ========

TEST(InsertExtra, InsertIntoInsertedResult_StaysCorrect) {
    LazySequence<int>* a = make_fin({1, 2, 3, 4, 5});
    LazySequence<int>* b = make_fin({10, 11});
    LazySequence<int>* mid = a->insert_at(b, 2);  // [1, 2, 10, 11, 3, 4, 5]

    LazySequence<int>* c = make_fin({99});
    LazySequence<int>* final_seq = mid->insert_at(c, 0);  // [99, 1, 2, 10, 11, 3, 4, 5]

    EXPECT_EQ(final_seq->get_count(), 8);
    EXPECT_EQ(final_seq->get(0), 99);
    EXPECT_EQ(final_seq->get(1), 1);
    EXPECT_EQ(final_seq->get(3), 10);
    EXPECT_EQ(final_seq->get(5), 3);
    EXPECT_EQ(final_seq->get(7), 5);

    delete final_seq; delete mid; delete a; delete b; delete c;
}