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
    // 1, 1, 2, 3, 5, 8, ... (без ведущего 0 - тесты FibAsInsertedSequence так и ожидают)
    int init[2] = { 1, 1 };
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
    Ordinal L = res->get_length();
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

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);  // 5 - 2 = 3 элемента в хвосте

    EXPECT_EQ(res->get(Ordinal(1, 0)), 3);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 4);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 5);
    EXPECT_THROW(res->get(Ordinal(1, 3)), std::out_of_range);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, Length_InfiniteIntoFinite_AtEnd) {
    // insert(inf) на самый конец: хвоста нет -> длина = ω, finite_part=0
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* ins = make_inf(100);
    LazySequence<int>* res = base->insert_at(ins, 3);

    Ordinal L = res->get_length();
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

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);

    EXPECT_EQ(res->get(0), 0);   // injected[0]
    EXPECT_EQ(res->get(5), 5);   // injected[5]

    EXPECT_EQ(res->get(Ordinal(1, 0)), 10);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 20);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 30);

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
    EXPECT_EQ(res->get(Ordinal(1, 0)), 10);
    EXPECT_EQ(res->get(Ordinal(1, 100)), 110);
    EXPECT_EQ(res->get(Ordinal(1, 9999)), 10009);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, RepeatedAccessIsConsistent) {
    // Многократный доступ к одному и тому же ординальному индексу даёт один и тот же результат
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    for (int trial = 0; trial < 3; trial++) {
        EXPECT_EQ(res->get(Ordinal(1, 0)), 5);
        EXPECT_EQ(res->get(Ordinal(1, 7)), 12);
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

    EXPECT_EQ(res->get(Ordinal(1, 0)), -1);
    EXPECT_EQ(res->get(Ordinal(1, 1)), -2);

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
        int v = res->get(Ordinal(1, k));
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
    EXPECT_THROW(res->get(Ordinal(1, 0)), std::out_of_range);

    delete res; delete base; delete ins;
}

TEST(InsertExtra, OrdinalZeroEquivalentToInt) {
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* ins = make_inf(1000);
    LazySequence<int>* res = base->insert_at(ins, 5);

    // {0, k} должен быть идентичен get(k)
    for (int k = 0; k < 30; k++) {
        EXPECT_EQ(res->get(Ordinal(0, k)), res->get(k)) << "k=" << k;
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
    EXPECT_EQ(combined->get(Ordinal(1, 0)), 3);

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

// ============================================================
// Новые тесты для проверки трансфинитной арифметики (P5, P7, P8, P10)
// ============================================================

TEST(Concat, ChainedInfInfInf_LengthOmegaThree) {
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* c = make_inf(1000);
    LazySequence<int>* ab = a->concat(b);
    LazySequence<int>* abc = ab->concat(c);

    Ordinal L = abc->get_length();
    EXPECT_EQ(L.get_omega_count(), 3u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    delete abc; delete ab; delete a; delete b; delete c;
}

TEST(Concat, ChainedInfInfInf_OrdinalAccess) {
    LazySequence<int>* a = make_inf(0);     // 0, 1, 2, ...
    LazySequence<int>* b = make_inf(100);   // 100, 101, ...
    LazySequence<int>* c = make_inf(1000);  // 1000, 1001, ...
    LazySequence<int>* ab = a->concat(b);
    LazySequence<int>* abc = ab->concat(c);

    // Левый ω-блок - a
    EXPECT_EQ(abc->get(Ordinal(0, 5)), 5);
    EXPECT_EQ(abc->get(Ordinal(0, 99)), 99);
    // Средний ω-блок - b
    EXPECT_EQ(abc->get(Ordinal(1, 0)), 100);
    EXPECT_EQ(abc->get(Ordinal(1, 50)), 150);
    // Правый ω-блок - c
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

    // Note: cleanup is messy because concat() returns new LazySequence but doesn't own operands.
    // Чтобы не утечь memory, удалим только конечные результаты и оригинальные операнды.
    delete left_assoc; delete right_assoc;
    delete a1; delete b1; delete c1;
    delete a2; delete b2; delete c2;
}

TEST(Concat, InfWithTail_ConcatPreservesTail) {
    // make_inf(0).append(99).concat(make_inf(1000))
    // Структура: [base ω] + [99] + [other ω] = ω + 1 + ω = ω·2
    // Tail '99' видится между блоками через Ordinal.
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* with_tail = base->append(99);
    LazySequence<int>* other = make_inf(1000);
    LazySequence<int>* res = with_tail->concat(other);

    // Длина: ω·1 (base) + 1 (tail) + ω·1 (other) = ω·2 (1 абсорбируется между ω-блоками)
    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 2u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    // get(Ordinal(0, k)) - в base
    EXPECT_EQ(res->get(Ordinal(0, 5)), 5);
    // get(Ordinal(1, 0)) - сразу после base = первый элемент tail-прослойки = 99
    EXPECT_EQ(res->get(Ordinal(1, 0)), 99);
    // get(Ordinal(1, 1)) - после tail-прослойки = первый элемент other
    EXPECT_EQ(res->get(Ordinal(1, 1)), 1000);

    delete res; delete with_tail; delete base; delete other;
}

TEST(Take, OrdinalLimitOmegaPlusN) {
    // На ω·2 берём take(Ordinal(1, 5)): длина результата = ω+5
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);       // ω·2

    LazySequence<int>* taken = ab->take(Ordinal(1, 5));

    Ordinal L = taken->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 5u);

    // Доступ внутри лимита
    EXPECT_EQ(taken->get(Ordinal(0, 5)), 5);
    EXPECT_EQ(taken->get(Ordinal(1, 0)), 100);
    EXPECT_EQ(taken->get(Ordinal(1, 4)), 104);

    // За лимитом - throw
    EXPECT_THROW(taken->get(Ordinal(1, 5)), std::out_of_range);
    EXPECT_THROW(taken->get(Ordinal(2, 0)), std::out_of_range);

    delete taken; delete ab; delete a; delete b;
}

TEST(Map, OrdinalAccessThroughMapAfterConcat) {
    // concat(inf, inf).map(*10).get(Ordinal(1, k)) должен работать
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);

    std::function<int(const int&)> times10 = [](const int& x) { return x * 10; };
    LazySequence<int>* mapped = ab->map<int>(times10);

    EXPECT_EQ(mapped->get(Ordinal(0, 5)), 50);
    EXPECT_EQ(mapped->get(Ordinal(1, 0)), 1000);
    EXPECT_EQ(mapped->get(Ordinal(1, 3)), 1030);

    delete mapped; delete ab; delete a; delete b;
}

TEST(Where, NoOrdinalAccess) {
    // where ломает ординальный доступ - должно бросить logic_error
    LazySequence<int>* a = make_inf(0);
    LazySequence<int>* b = make_inf(100);
    LazySequence<int>* ab = a->concat(b);

    std::function<bool(const int&)> all = [](const int&) { return true; };
    LazySequence<int>* filtered = ab->where(all);

    EXPECT_THROW(filtered->get(Ordinal(1, 0)), std::logic_error);

    delete filtered; delete ab; delete a; delete b;
}

TEST(OrdinalArithmetic, PrependOnInfiniteKeepsLengthOmega) {
    // 1 + ω = ω (левая абсорбция)
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* prepended = base->prepend(-1);

    Ordinal L = prepended->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);  // НЕ omega+1!

    EXPECT_EQ(prepended->get(0), -1);
    EXPECT_EQ(prepended->get(1), 0);

    delete prepended; delete base;
}

TEST(OrdinalArithmetic, InsertSingleItemIntoInfiniteKeepsLengthOmega) {
    // n + 1 + ω = ω
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* inserted = base->insert_at(99, 5);

    Ordinal L = inserted->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    EXPECT_EQ(inserted->get(5), 99);
    EXPECT_EQ(inserted->get(6), 5);   // base[5] сдвинут на 1

    delete inserted; delete base;
}

TEST(OrdinalArithmetic, AppendOnInfiniteGivesOmegaPlusOne) {
    // ω + 1 = ω + 1 (НЕ абсорбируется справа)
    LazySequence<int>* base = make_inf(0);
    LazySequence<int>* appended = base->append(99);

    Ordinal L = appended->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 1u);  // ω+1

    delete appended; delete base;
}

TEST(WhereOnFinite, LengthIsUpperBound) {
    // where на финитной должен иметь длину = base_length как верхнюю границу
    LazySequence<int>* base = make_fin({1, 2, 3, 4, 5});
    std::function<bool(const int&)> even = [](const int& x) { return x % 2 == 0; };
    LazySequence<int>* w = base->where(even);

    Ordinal L = w->get_length();
    EXPECT_TRUE(L.is_finite());
    EXPECT_EQ(L.get_value(), 5u);  // upper bound = base_length

    delete w; delete base;
}

// ============================================================
// Регрессионные тесты на найденные в повторном ревью баги
// ============================================================

TEST(InsertSeq, OtherWithTail_AllElementsAccessible) {
    // Bug A: other = base[10,20] + tail[99]. insert_at(other, 1) должен дать
    // [1, 10, 20, 99, 2, 3], а не throw "injected exhausted".
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* other_base = make_fin({10, 20});
    LazySequence<int>* other = other_base->append(99);
    LazySequence<int>* res = base->insert_at(other, 1);

    EXPECT_EQ(res->get_count(), 6);
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 10);
    EXPECT_EQ(res->get(2), 20);
    EXPECT_EQ(res->get(3), 99);
    EXPECT_EQ(res->get(4), 2);
    EXPECT_EQ(res->get(5), 3);

    delete res; delete other; delete other_base; delete base;
}

TEST(InsertSeq, ThisWithTail_TailPreserved) {
    // Bug B: this = base[1,2,3] + tail[99]. insert_at(other_fin, 0) должен
    // дать [10, 20, 1, 2, 3, 99], а не терять 99.
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* this_seq = base->append(99);
    LazySequence<int>* other = make_fin({10, 20});
    LazySequence<int>* res = this_seq->insert_at(other, 0);

    EXPECT_EQ(res->get_count(), 6);
    EXPECT_EQ(res->get(0), 10);
    EXPECT_EQ(res->get(1), 20);
    EXPECT_EQ(res->get(2), 1);
    EXPECT_EQ(res->get(3), 2);
    EXPECT_EQ(res->get(4), 3);
    EXPECT_EQ(res->get(5), 99);  // tail сохранён

    delete res; delete this_seq; delete other; delete base;
}

TEST(InsertSeq, InfiniteOtherIntoSeqAfterAppend_Works) {
    // В унифицированной модели append встроен в generator, поэтому insert(inf, 1)
    // в this = [1,2,3,99] (где 99 — бывший tail) — это обычный случай finite + inf.
    // Длина: 1 + ω + 3 = ω + 3.
    LazySequence<int>* base = make_fin({1, 2, 3});
    LazySequence<int>* this_seq = base->append(99);
    LazySequence<int>* other = make_inf(100);

    LazySequence<int>* res = this_seq->insert_at(other, 1);

    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 1u);
    EXPECT_EQ(L.get_finite_part(), 3u);  // [2, 3, 99] хвост за ω

    // Линейно: this[0] = 1, then injected inf = 100, 101, ...
    EXPECT_EQ(res->get(0), 1);
    EXPECT_EQ(res->get(1), 100);
    EXPECT_EQ(res->get(2), 101);

    // Ординально: хвост this за ω-блоком
    EXPECT_EQ(res->get(Ordinal(1, 0)), 2);
    EXPECT_EQ(res->get(Ordinal(1, 1)), 3);
    EXPECT_EQ(res->get(Ordinal(1, 2)), 99);

    delete res; delete this_seq; delete other; delete base;
}

TEST(WhereAfterAppend, LengthIsUpperBoundOfWholeSequence) {
    // В унифицированной модели append встроен в generator. base.append(6).append(7).append(8)
    // даёт length=8 (всё внутри). where(even) - upper bound = 8 (все элементы могут пройти).
    // Реальное число фильтрованных проверяется через материализацию.
    LazySequence<int>* base = make_fin({1, 2, 3, 4, 5});
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

    delete w; delete with_extra; delete s2; delete s1; delete base;
}

TEST(InsertSeq, InfiniteIntoOmegaTwoUpstream_OrdinalAccessAllBlocks) {
    // Регрессия: раньше для omega_part > 1 в InsertAtGenerator::get_at
    // код передавал idx в upstream без сдвига координат, и сложные случаи
    // (вставка ω в upstream с omega_count >= 2) ломались.
    // После исправления зон-формула работает универсально.
    LazySequence<int>* a = make_inf(0);          // 0, 1, 2, ...           (ω)
    LazySequence<int>* b = make_inf(1000);       // 1000, 1001, ...        (ω)
    LazySequence<int>* ab = a->concat(b);        // ω·2
    LazySequence<int>* inj = make_inf(100);      // 100, 101, ...          (ω)
    LazySequence<int>* res = ab->insert_at(inj, 5);

    // Длина: 5 + ω + ω·2 = ω·3
    Ordinal L = res->get_length();
    EXPECT_EQ(L.get_omega_count(), 3u);
    EXPECT_EQ(L.get_finite_part(), 0u);

    // omega_part=0, финитная часть: zone 1 (первые 5 ab) + zone 2 (inj)
    EXPECT_EQ(res->get(0), 0);            // ab[0]
    EXPECT_EQ(res->get(4), 4);            // ab[4]
    EXPECT_EQ(res->get(5), 100);          // inj[0]
    EXPECT_EQ(res->get(105), 200);        // inj[100]

    // omega_part=1: хвост первого ω-блока ab (= a) после позиции 5
    EXPECT_EQ(res->get(Ordinal(1, 0)), 5);       // a[5]
    EXPECT_EQ(res->get(Ordinal(1, 100)), 105);   // a[105]

    // omega_part=2: второй ω-блок ab (= b) - ИМЕННО ЗДЕСЬ был баг
    EXPECT_EQ(res->get(Ordinal(2, 0)), 1000);    // b[0]
    EXPECT_EQ(res->get(Ordinal(2, 5)), 1005);    // b[5]

    delete res; delete ab; delete a; delete b; delete inj;
}