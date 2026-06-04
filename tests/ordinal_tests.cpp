#include "lazy/lazy_sequence.h"
#include "core/sequence.h"
#include <gtest/gtest.h>

int naturals_rule(Sequence<int>* w) { return w->get_last() + 1; }
int fib_rule(Sequence<int>* w) { return w->get_first() + w->get_last(); }

LazySequence<int>* make_inf(int start) {
    int init[1] = { start };
    MutableArraySequence<int> initial(init, 1);

    return new LazySequence<int>(naturals_rule, &initial);
}

LazySequence<int>* make_fib() {
    // 1, 1, 2, 3, 5, 8
    int init[2] = { 1, 1 };
    MutableArraySequence<int> initial(init, 2);

    return new LazySequence<int>(fib_rule, &initial);
}

LazySequence<int>* make_fin(const int* items, int count) {
    return new LazySequence<int>(items, count);
}

// ======== Граничные случаи

TEST(InsertExtra, InsertAtEnd_Finite) {
    // Эквивалентно конкатенации
    int base_data[] = {1, 2, 3};
    int ins_data[] = {99, 100};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_fin(ins_data, 2);
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
    int base_data[] = {1, 2, 3};
    int ins_data[] = {99};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_fin(ins_data, 1);
    EXPECT_THROW(base->insert_at(ins, 4), std::out_of_range);

    delete base;
    delete ins;
}

TEST(InsertExtra, NegativeIndex_Throws) {
    int base_data[] = {1, 2, 3};
    int ins_data[] = {99};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_fin(ins_data, 1);
    EXPECT_THROW(base->insert_at(ins, -1), std::out_of_range);

    delete base;
    delete ins;
}

TEST(InsertExtra, NullOther_Throws) {
    int base_data[] = {1, 2, 3};
    LazySequence<int>* base = make_fin(base_data, 3);
    EXPECT_THROW(base->insert_at(static_cast<LazySequence<int>*>(nullptr), 0), std::invalid_argument);
    delete base;
}

TEST(InsertExtra, EmptyOther_IsIdentity) {
    // Вставка пустой последовательности должна давать копию this
    int base_data[] = {1, 2, 3};
    LazySequence<int>* base = make_fin(base_data, 3);
    MutableArraySequence<int> empty;
    LazySequence<int>* ins = new LazySequence<int>(&empty);
    LazySequence<int>* res = base->insert_at(ins, 1);

    EXPECT_EQ(res->get_count(), 3);
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 2);
    EXPECT_EQ(res->get(2), 3);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, FiniteIntoFinite_NoOmegaAccess) {
    int base_data[] = {1, 2, 3};
    int ins_data[] = {99};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_fin(ins_data, 1);
    LazySequence<int>* res = base->insert_at(ins, 1);

    // omega_part = 1 на полностью финитной последовательности - выход за границы
    EXPECT_THROW(res->get(Ordinal(1, 0)), std::out_of_range);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, OrdinalZeroEquivalentToInt) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    // {0, k} должен быть идентичен get(k)
    for (int k = 0; k < 30; k++) {
        EXPECT_EQ(res->get(Ordinal(0, k)), res->get(k)) << "k=" << k;
    }

    delete res;
    delete base;
    delete ins;
}

// ======== Ординальная арифметика

TEST(InsertExtra, Length_FiniteIntoInfinite) {
    LazySequence<int>* base = make_inf(0);
    int ins_data[] = {100, 200};
    LazySequence<int>* ins = make_fin(ins_data, 2);
    LazySequence<int>* res = base->insert_at(ins, 5);

    Ordinal L = res->get_length();
    EXPECT_TRUE(L.is_infinite());
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite) {
    // base = [a, b, c, d, e], insert(inf) на позицию 2 -> w + 3
    int base_data[] = {1, 2, 3, 4, 5};
    LazySequence<int>* base = make_fin(base_data, 5);
    LazySequence<int>* ins = make_inf(100);
    LazySequence<int>* res = base->insert_at(ins, 2);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 3);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 4);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 5);
    EXPECT_THROW(res->get(Ordinal(1, 3)), std::out_of_range);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite_AtEnd) {
    // insert(inf) на самый конец: хвоста нет -> длина = w, finite_part=0
    int base_data[] = {1, 2, 3};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_inf(100);
    LazySequence<int>* res = base->insert_at(ins, 3);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(2), 3);
    EXPECT_EQ(res->get(3), 100);
    EXPECT_EQ(res->get(10), 107);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite_AtBeginning) {
    // insert(inf) в начало финитной: хвост = весь base
    int base_data[] = {10, 20, 30};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* ins = make_inf(0);
    LazySequence<int>* res = base->insert_at(ins, 0);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);

    EXPECT_EQ(res->get(0), 0);   // injected[0]
    EXPECT_EQ(res->get(5), 5);   // injected[5]

    EXPECT_EQ(res->get(Ordinal(1, 0)), 10);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 20);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 30);

    delete res;
    delete base;
    delete ins;
}

// ======== Большие индексы

TEST(InsertExtra, LargeIndicesInOmegaPart) {
    LazySequence<int>* base = make_inf(0);   // 0, 1, 2
    LazySequence<int>* ins = make_inf(1000); // 1000, 1001
    LazySequence<int>* res = base->insert_at(ins, 10);

    EXPECT_EQ(res->get(9), 9);          // left tail
    EXPECT_EQ(res->get(10), 1000);      // первый injected
    EXPECT_EQ(res->get(1000), 1990);    // injected[990]

    // Хвост base (10, 11, 12, ...) в первом w блоке
    EXPECT_EQ(res->get(Ordinal(1, 0)), 10);
    EXPECT_EQ(res->get(Ordinal(1, 100)), 110);
    EXPECT_EQ(res->get(Ordinal(1, 9999)), 10009);

    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, RepeatedAccessIsConsistent) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    for (int trial = 0; trial < 3; trial++) {
        EXPECT_EQ(res->get(Ordinal(1, 0)), 5);
        EXPECT_EQ(res->get(Ordinal(1, 7)), 12);
        EXPECT_EQ(res->get(100), 1095);
    }

    delete res;
    delete base;
    delete ins;
}

// ========

TEST(InsertExtra, NestedInsert_TwoFiniteInsertsIntoFinite) {
    int base_data[] = {1, 2, 3, 4, 5};
    int ins1_data[] = {10, 20};
    LazySequence<int>* base = make_fin(base_data, 5);
    LazySequence<int>* ins1 = make_fin(ins1_data, 2);
    LazySequence<int>* res1 = base->insert_at(ins1, 2);
    // res1 = [1, 2, 10, 20, 3, 4, 5]

    int ins2_data[] = {99};
    LazySequence<int>* ins2 = make_fin(ins2_data, 1);
    LazySequence<int>* res2 = res1->insert_at(ins2, 5);
    // res2 = [1, 2, 10, 20, 3, 99, 4, 5]

    EXPECT_EQ(res2->get_count(), 8);
    EXPECT_EQ(res2->get(0), 1);
    EXPECT_EQ(res2->get(2), 10);
    EXPECT_EQ(res2->get(4), 3);
    EXPECT_EQ(res2->get(5), 99);
    EXPECT_EQ(res2->get(6), 4);
    EXPECT_EQ(res2->get(7), 5);

    delete res2;
    delete res1;
    delete base;
    delete ins1;
    delete ins2;
}

TEST(InsertExtra, FibAsInsertedSequence) {
    // Вставляем первые из фибоначчи (бесконечно) в конечный base
    int base_data[] = {0, -1, -2};
    LazySequence<int>* base = make_fin(base_data, 3);
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

    EXPECT_EQ(res->get(Ordinal(1, 0)), -1);
    EXPECT_EQ(res->get(Ordinal(1, 1)), -2);

    delete res;
    delete base;
    delete fib;
}

// ======== 

TEST(InsertExtra, NoCrossContamination_InfIntoInf) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 7);

    for (int i = 7; i < 100; i++) {
        EXPECT_GE(res->get(i), 1000) << "index=" << i;
        EXPECT_LE(res->get(i), 1100) << "index=" << i;
    }

    for (int i = 0; i < 7; i++) {
        EXPECT_LT(res->get(i), 7) << "index=" << i;
    }

    for (size_t k = 0; k < 20; k++) {
        int v = res->get(Ordinal(1, k));
        EXPECT_GE(v, 7) << "omega_part=1, k=" << k;
        EXPECT_LT(v, 1000) << "omega_part=1, k=" << k;
    }

    delete res;
    delete base;
    delete ins;
}



// ======== insert_at(item, idx)

TEST(InsertExtra, LegacySingleInsert_AtEnd_Finite) {
    int base_data[] = {1, 2, 3};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* res = base->insert_at(99, 3);

    EXPECT_EQ(res->get_count(), 4);
    EXPECT_EQ(res->get(3), 99);

    delete res;
    delete base;
}

TEST(InsertExtra, LegacySingleInsert_AtBeginning_Inf) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* res = base->insert_at(-1, 0);

    EXPECT_EQ(res->get(0), -1);
    EXPECT_EQ(res->get(1), 0);
    EXPECT_EQ(res->get(2), 1);

    delete res;
    delete base;
}

// ======== Взаимодействие с map / take

TEST(InsertExtra, MapAfterInsert) {
    LazySequence<int>* base = make_inf(0);
    int ins_data[] = {100, 200};
    LazySequence<int>* ins = make_fin(ins_data, 2);
    LazySequence<int>* res = base->insert_at(ins, 3);

    // map x -> x * 10
    LazySequence<int>* mapped = res->map(std::function<int(const int&)>([](const int& x) { return x * 10; }));

    EXPECT_EQ(mapped->get(0), 0);
    EXPECT_EQ(mapped->get(2), 20);
    EXPECT_EQ(mapped->get(3), 1000);  // 100 * 10
    EXPECT_EQ(mapped->get(4), 2000);
    EXPECT_EQ(mapped->get(5), 30);    // base[3]*10 (base продолжается)

    delete mapped;
    delete res;
    delete base;
    delete ins;
}

TEST(InsertExtra, TakeAfterInsert_TurnsInfiniteIntoFinite) {
    LazySequence<int>* base = make_inf(0);
    int ins_data[] = {100, 200};
    LazySequence<int>* ins = make_fin(ins_data, 2);
    LazySequence<int>* res = base->insert_at(ins, 2);
    // res = 0, 1, 100, 200, 2, 3, 4, ...

    LazySequence<int>* taken = res->take(6);
    // taken = [0, 1, 100, 200, 2, 3]
    EXPECT_EQ(taken->get_count(), 6);
    EXPECT_EQ(taken->get(0), 0);
    EXPECT_EQ(taken->get(2), 100);
    EXPECT_EQ(taken->get(3), 200);
    EXPECT_EQ(taken->get(5), 3);

    delete taken;
    delete res;
    delete base;
    delete ins;
}

// ======== concat + insert

TEST(InsertExtra, ConcatAfterInsert) {
    int base_data[] = {1, 2};
    int ins_data[] = {10, 20};
    int extra_data[] = {99, 100};
    LazySequence<int>* base = make_fin(base_data, 2);
    LazySequence<int>* ins  = make_fin(ins_data, 2);
    LazySequence<int>* res1 = base->insert_at(ins, 1);  // [1, 10, 20, 2]
    LazySequence<int>* extra = make_fin(extra_data, 2);
    LazySequence<int>* res2 = res1->concat(extra);      // [1, 10, 20, 2, 99, 100]

    EXPECT_EQ(res2->get_count(), 6);
    EXPECT_EQ(res2->get(0), 1);
    EXPECT_EQ(res2->get(1), 10);
    EXPECT_EQ(res2->get(3), 2);
    EXPECT_EQ(res2->get(4), 99);
    EXPECT_EQ(res2->get(5), 100);

    delete res2;
    delete res1;
    delete base;
    delete ins;
    delete extra;
}

TEST(InsertExtra, InsertWithBothOperandsInfinite_ConcatWithFinite) {
    // Сначала insert(inf, inf), потом concat(...)
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* combined = a->insert_at(b, 3);  // ω * 2

    // Проверяем, что длина ω * 2
    EXPECT_EQ(combined->get_length().get_omega_count(), 2u);
    EXPECT_EQ(combined->get(2), 2);
    EXPECT_EQ(combined->get(3), 100);
    EXPECT_EQ(combined->get(Ordinal(1, 0)), 3);

    delete combined;
    delete a;
    delete b;
}

// ======== insert в insert

TEST(InsertExtra, InsertIntoInsertedResult_StaysCorrect) {
    int a_data[] = {1, 2, 3, 4, 5};
    int b_data[] = {10, 11};
    int c_data[] = {99};
    LazySequence<int>* a = make_fin(a_data, 5);
    LazySequence<int>* b = make_fin(b_data, 2);
    LazySequence<int>* mid = a->insert_at(b, 2);  // [1, 2, 10, 11, 3, 4, 5]

    LazySequence<int>* c = make_fin(c_data, 1);
    LazySequence<int>* final_seq = mid->insert_at(c, 0);  // [99, 1, 2, 10, 11, 3, 4, 5]

    EXPECT_EQ(final_seq->get_count(), 8);
    EXPECT_EQ(final_seq->get(0), 99);
    EXPECT_EQ(final_seq->get(1), 1);
    EXPECT_EQ(final_seq->get(3), 10);
    EXPECT_EQ(final_seq->get(5), 3);
    EXPECT_EQ(final_seq->get(7), 5);

    delete final_seq;
    delete mid;
    delete a;
    delete b;
    delete c;
}

// ======== Тесты для проверки трансфинитной арифметики

TEST(Concat, ChainedInfInfInf_LengthOmegaThree) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* c = make_inf(1000);
    LazySequence<int>* ab = a->concat(b);
    LazySequence<int>* abc = ab->concat(c);

    Ordinal L = abc->get_length();
    EXPECT_EQ(L.get_omega_count(), 3u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    delete abc;
    delete ab;
    delete a;
    delete b;
    delete c;
}

TEST(Concat, ChainedInfInfInf_OrdinalAccess) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* c = make_inf(1000);
    LazySequence<int>* ab = a->concat(b);
    LazySequence<int>* abc = ab->concat(c);

    // Левый блок a
    EXPECT_EQ(abc->get(Ordinal(0, 5)), 5);
    EXPECT_EQ(abc->get(Ordinal(0, 99)), 99);
    // Средний блок b
    EXPECT_EQ(abc->get(Ordinal(1, 0)), 100);
    EXPECT_EQ(abc->get(Ordinal(1, 50)), 150);
    // Правый блок c
    EXPECT_EQ(abc->get(Ordinal(2, 0)), 1000);
    EXPECT_EQ(abc->get(Ordinal(2, 99)), 1099);

    delete abc; delete ab; delete a; delete b; delete c;
}

TEST(Concat, ChainedAssociativityOfAccess) {
    LazySequence<int>* a1 = make_inf(0);
    LazySequence<int>* b1 = make_inf(100);
    LazySequence<int>* c1 = make_inf(1000);

    LazySequence<int>* a2 = make_inf(0);
    LazySequence<int>* b2 = make_inf(100);
    LazySequence<int>* c2 = make_inf(1000);

    // (a + b) + c
    LazySequence<int>* left_assoc = a1->concat(b1)->concat(c1);
    // a + (b + c)
    LazySequence<int>* right_assoc = a2->concat(b2->concat(c2));

    for (size_t k = 0; k < 10; k++) {
        EXPECT_EQ(left_assoc->get(Ordinal(0, k)), right_assoc->get(Ordinal(0, k)));
        EXPECT_EQ(left_assoc->get(Ordinal(1, k)), right_assoc->get(Ordinal(1, k)));
        EXPECT_EQ(left_assoc->get(Ordinal(2, k)), right_assoc->get(Ordinal(2, k)));
    }

    delete left_assoc;
    delete right_assoc;
    delete a1;
    delete b1;
    delete c1;
    delete a2;
    delete b2;
    delete c2;
}

TEST(Concat, InfWithTail_ConcatPreservesTail) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* with_tail = base->append(99);
    LazySequence<int>* other = make_inf(1000);
    LazySequence<int>* res = with_tail->concat(other);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 2u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    // get(Ordinal(0, k))в base
    EXPECT_EQ(res->get(Ordinal(0, 5)), 5);
    // get(Ordinal(1, 0)) сразу после base = первый элемент tail = 99
    EXPECT_EQ(res->get(Ordinal(1, 0)), 99);
    // get(Ordinal(1, 1)) после tail = первый элемент other
    EXPECT_EQ(res->get(Ordinal(1, 1)), 1000);

    delete res;
    delete with_tail;
    delete base;
    delete other;
}

TEST(Take, OrdinalLimitOmegaPlusN) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);

    LazySequence<int>* taken = ab->take(Ordinal(1, 5));

    Ordinal L = taken->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 5u);

    EXPECT_EQ(taken->get(Ordinal(0, 5)), 5);
    EXPECT_EQ(taken->get(Ordinal(1, 0)), 100);
    EXPECT_EQ(taken->get(Ordinal(1, 4)), 104);

    EXPECT_THROW(taken->get(Ordinal(1, 5)), std::out_of_range);
    EXPECT_THROW(taken->get(Ordinal(2, 0)), std::out_of_range);

    delete taken;
    delete ab;
    delete a;
    delete b;
}

TEST(Map, OrdinalAccessThroughMapAfterConcat) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);
    std::function<int(const int&)> times10 = [](const int& x) { return x * 10; };
    LazySequence<int>* mapped = ab->map<int>(times10);

    EXPECT_EQ(mapped->get(Ordinal(0, 5)), 50);
    EXPECT_EQ(mapped->get(Ordinal(1, 0)), 1000);
    EXPECT_EQ(mapped->get(Ordinal(1, 3)), 1030);

    delete mapped;
    delete ab; 
    delete a;
    delete b;
}

TEST(Where, NoOrdinalAccess) {
    // where ломает ординальный доступ - должно бросить logic_error
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);

    std::function<bool(const int&)> all = [](const int&) { return true; };
    LazySequence<int>* filtered = ab->where(all);

    EXPECT_THROW(filtered->get(Ordinal(1, 0)), std::logic_error);

    delete filtered;
    delete ab;
    delete a;
    delete b;
}

TEST(OrdinalArithmetic, PrependOnInfiniteKeepsLengthOmega) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* prepended = base->prepend(-1);

    Ordinal L = prepended->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);  // Проверка что не omega+1

    EXPECT_EQ(prepended->get(0), -1);
    EXPECT_EQ(prepended->get(1), 0);

    delete prepended;
    delete base;
}

TEST(OrdinalArithmetic, InsertSingleItemIntoInfiniteKeepsLengthOmega) {
    // n + 1 + w = w
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* appended = base->append(99);

    Ordinal L = appended->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 1u);

    delete appended; delete base;
}

TEST(WhereOnFinite, LengthIsUpperBound) {
    // where на финитной должен иметь длину = base_length как верхнюю границу
    int base_data[] = {1, 2, 3, 4, 5};
    LazySequence<int>* base = make_fin(base_data, 5);
    std::function<bool(const int&)> even = [](const int& x) { return x % 2 == 0; };
    LazySequence<int>* w = base->where(even);

    Ordinal L = w->get_length();
    EXPECT_TRUE(L.is_finite());
    EXPECT_EQ(L.get_value(), 5u);

    delete w;
    delete base;
}

// ============================================================

TEST(InsertSeq, OtherWithTail_AllElementsAccessible) {
    // other = base[10,20] + tail[99] и insert_at(other, 1) должен дать [1, 10, 20, 99, 2, 3]
    int base_data[] = {1, 2, 3};
    int other_data[] = {10, 20};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* other_base = make_fin(other_data, 2);
    LazySequence<int>* other = other_base->append(99);
    LazySequence<int>* res = base->insert_at(other, 1);

    EXPECT_EQ(res->get_count(), 6);
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 10);
    EXPECT_EQ(res->get(2), 20);
    EXPECT_EQ(res->get(3), 99);
    EXPECT_EQ(res->get(4), 2);
    EXPECT_EQ(res->get(5), 3);

    delete res;
    delete other;
    delete other_base;
    delete base;
}

TEST(InsertSeq, ThisWithTail_TailPreserved) {
    int base_data[] = {1, 2, 3};
    int other_data[] = {10, 20};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* this_seq = base->append(99);
    LazySequence<int>* other = make_fin(other_data, 2);
    LazySequence<int>* res = this_seq->insert_at(other, 0);

    EXPECT_EQ(res->get_count(), 6);
    EXPECT_EQ(res->get(0), 10);
    EXPECT_EQ(res->get(1), 20);
    EXPECT_EQ(res->get(2), 1);
    EXPECT_EQ(res->get(3), 2);
    EXPECT_EQ(res->get(4), 3);
    EXPECT_EQ(res->get(5), 99);  // tail сохранён

    delete res;
    delete this_seq;
    delete other;
    delete base;
}

TEST(InsertSeq, InfiniteOtherIntoSeqAfterAppend_Works) {
    // В модели append встроен в generator, поэтому insert(inf, 1) в this = [1,2,3,99] (где 99 прошлый tail) — это finite + inf.
    // Длина: 1 + w + 3 = w + 3.
    int base_data[] = {1, 2, 3};
    LazySequence<int>* base = make_fin(base_data, 3);
    LazySequence<int>* this_seq = base->append(99);
    LazySequence<int>* other = make_inf(100);

    LazySequence<int>* res = this_seq->insert_at(other, 1);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);  // [2, 3, 99] хвост за w

    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 100);
    EXPECT_EQ(res->get(2), 101);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 2);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 3);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 99);

    delete res;
    delete this_seq;
    delete other;
    delete base;
}

TEST(WhereAfterAppend, LengthIsUpperBoundOfWholeSequence) {
    int base_data[] = {1, 2, 3, 4, 5};
    LazySequence<int>* base = make_fin(base_data, 5);
    LazySequence<int>* s1 = base->append(6);
    LazySequence<int>* s2 = s1->append(7);
    LazySequence<int>* with_extra = s2->append(8);

    Ordinal full_len = with_extra->get_length();
    EXPECT_EQ(full_len.get_value(), 8u);

    std::function<bool(const int&)> even = [](const int& x) { return x % 2 == 0; };
    LazySequence<int>* w = with_extra->where(even);

    Ordinal L = w->get_length();
    EXPECT_TRUE(L.is_finite());
    EXPECT_EQ(L.get_value(), 8u);   // upper bound = вся длина source

    // Реально отфильтровано 4: 2, 4, 6, 8
    EXPECT_EQ(w->get(0), 2);
    EXPECT_EQ(w->get(1), 4);
    EXPECT_EQ(w->get(2), 6);
    EXPECT_EQ(w->get(3), 8);
    EXPECT_THROW(w->get(4), std::out_of_range);

    delete w;
    delete with_extra;
    delete s2;
    delete s1;
    delete base;
}

TEST(InsertSeq, InfiniteIntoOmegaTwoUpstream_OrdinalAccessAllBlocks) {
    LazySequence<int>* a = make_inf(0);          // 0, 1, 2, ...
    LazySequence<int>* b = make_inf(1000);       // 1000, 1001, ...
    LazySequence<int>* ab = a->concat(b);        // w * 2
    LazySequence<int>* inj = make_inf(100);      // 100, 101, ...
    LazySequence<int>* res = ab->insert_at(inj, 5);

    // Длина 5 + w + w * 2 = w * 3
    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 3u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    // omega_part = 0, финитная часть: zone 1 (первые 5 ab) + zone 2 (inj)
    EXPECT_EQ(res->get(0), 0);            // ab[0]
    EXPECT_EQ(res->get(4), 4);            // ab[4]
    EXPECT_EQ(res->get(5), 100);          // inj[0]
    EXPECT_EQ(res->get(105), 200);        // inj[100]

    // omega_part = 1: хвост первого w блока ab (= a) после позиции 5
    EXPECT_EQ(res->get(Ordinal(1, 0)), 5);       // a[5]
    EXPECT_EQ(res->get(Ordinal(1, 100)), 105);   // a[105]

    // omega_part = 2: второй w блок ab (= b)
    EXPECT_EQ(res->get(Ordinal(2, 0)), 1000);    // b[0]
    EXPECT_EQ(res->get(Ordinal(2, 5)), 1005);    // b[5]

    delete res;
    delete ab;
    delete a;
    delete b;
    delete inj;
}

// ======== Ординальный insert_at

TEST(OrdinalInsert, FiniteItemAtOmegaBoundary) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* chain = a->concat(b);

    LazySequence<int>* res = chain->insert_at(999, Ordinal(1, 0));

    // Длина w + 1 + w = ω * 2
    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 2u);

    // Линейно — первый w блок не изменился
    EXPECT_EQ(res->get(0), 0);
    EXPECT_EQ(res->get(99), 99);

    // Ординально — вставленный элемент на позиции (1, 0)
    EXPECT_EQ(res->get(Ordinal(1, 0)), 999);

    // Второй блок (b) сдвинулся на 1 в финитной части
    EXPECT_EQ(res->get(Ordinal(1, 1)), 100);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 101);
    EXPECT_EQ(res->get(Ordinal(1, 10)), 109);

    delete res;
    delete chain;
    delete a;
    delete b;
}

TEST(OrdinalInsert, FiniteSeqAtOmegaBoundary) {
    // concat(inf, inf) длиной w * 2, вставляем [99, 100] на позицию w
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(1000);
    LazySequence<int>* chain = a->concat(b);

    int ins_data[] = {99, 100};
    LazySequence<int>* ins = make_fin(ins_data, 2);
    LazySequence<int>* res = chain->insert_at(ins, Ordinal(1, 0));

    EXPECT_EQ(res->get_length().get_omega_count(), 2u);

    EXPECT_EQ(res->get(5), 5);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 99);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 100);

    EXPECT_EQ(res->get(Ordinal(1, 2)), 1000);
    EXPECT_EQ(res->get(Ordinal(1, 3)), 1001);

    delete res;
    delete ins;
    delete chain;
    delete a;
    delete b;
}

TEST(OrdinalInsert, InfiniteSeqAtOmegaBoundary) {
    // concat(inf_a, inf_b) длиной w * 2, вставляем inf_c на позицию w -> w * 3
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(1000);
    LazySequence<int>* chain = a->concat(b);

    LazySequence<int>* c = make_inf(500);
    LazySequence<int>* res = chain->insert_at(c, Ordinal(1, 0));

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 3u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    EXPECT_EQ(res->get(0), 0);
    EXPECT_EQ(res->get(50), 50);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 500);
    EXPECT_EQ(res->get(Ordinal(1, 10)), 510);

    EXPECT_EQ(res->get(Ordinal(2, 0)), 1000);
    EXPECT_EQ(res->get(Ordinal(2, 5)), 1005);

    delete res;
    delete c;
    delete chain;
    delete a;
    delete b;
}

TEST(OrdinalInsert, AtOmegaPlusFive) {
    // concat(inf_a, inf_b), вставляем элемент на позицию w + 5
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(1000);
    LazySequence<int>* chain = a->concat(b);

    LazySequence<int>* res = chain->insert_at(777, Ordinal(1, 5));

    EXPECT_EQ(res->get(0), 0);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 1000);
    EXPECT_EQ(res->get(Ordinal(1, 4)), 1004);

    EXPECT_EQ(res->get(Ordinal(1, 5)), 777);

    EXPECT_EQ(res->get(Ordinal(1, 6)), 1005);
    EXPECT_EQ(res->get(Ordinal(1, 7)), 1006);

    delete res;
    delete chain;
    delete a;
    delete b;
}

TEST(OrdinalInsert, PastLengthThrows) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* chain = a->concat(b);

    EXPECT_THROW(chain->insert_at(999, Ordinal(3, 0)), std::out_of_range);

    delete chain;
    delete a;
    delete b;
}

TEST(OrdinalInsert, AtEndEqualsAppend) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* res = a->insert_at(999, Ordinal::infinity());

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 1u);

    EXPECT_EQ(res->get(0), 0);
    EXPECT_EQ(res->get(100), 100);

    EXPECT_EQ(res->get(Ordinal(1, 0)), 999);

    delete res;
    delete a;
}