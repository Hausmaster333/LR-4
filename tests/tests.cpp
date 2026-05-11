#include "utils.h"
#include "core/sequence.h"
#include "types/bit_sequence.h"
#include <gtest/gtest.h>

template <class T>
void check_sequence(const Sequence<T>* seq, const T* expected, int expected_count) {
    EXPECT_EQ(seq->get_count(), expected_count);
    
    EnumeratorWrapper<T> iter(seq->get_enumerator());
    int i = 0;
    while (iter.move_next()) {
        EXPECT_EQ(iter.get_current(), expected[i]);
        i++;
    }
}

template <class T>
void check_linked_list(const LinkedList<T>& list, const T* expected, int expected_count) {
    EXPECT_EQ(list.get_length(), expected_count);

    EnumeratorWrapper<T> iter(list.get_enumerator());
    int i = 0;
    while (iter.move_next()) {
        EXPECT_EQ(iter.get_current(), expected[i]);
        i++;
    }
}

int square(const int& x) {
    return x * x;
}

bool is_positive(const int& x) {
    return x > 0;
}

int sum(const int& a, const int& b) {
    return a + b;
}

bool is_zero(const int& x) {
    return x == 0;
}

bool is_zero_bit(const Bit& b) {
    return !b.get();
}

TEST(DynamicArrayTest, Constructors) {
    DynamicArray<int> empty_array; // DynamicArray()
    EXPECT_EQ(empty_array.get_size(), 0);

    DynamicArray<int> array(5); // DynamicArray(int size)
    EXPECT_EQ(array.get_size(), 5);

    int items[] = {1, 2, 3};
    DynamicArray<int> arr_from_items(items, 3); // DynamicArray(const T* items, int count)
    EXPECT_EQ(arr_from_items.get(0), 1);
    EXPECT_EQ(arr_from_items.get(1), 2);
    EXPECT_EQ(arr_from_items.get(2), 3);
    EXPECT_EQ(arr_from_items.get_size(), 3);

    DynamicArray<int> other(3);
    other.set(0, 1);
    other.set(1, 2);
    DynamicArray<int> copy_from_other(other); // DynamicArray(const DynamicArray<T>& other)
    EXPECT_EQ(copy_from_other.get(0), 1);
    EXPECT_EQ(copy_from_other.get(1), 2);
    EXPECT_EQ(copy_from_other.get_size(), 3);
}

TEST(DynamicArrayTest, GetAndSet) {
    DynamicArray<int> arr(5);
    arr.set(0, 42); // set(index, value)
    EXPECT_EQ(arr.get(0), 42); // get(index)
    EXPECT_EQ(arr.get_size(), 5); // get_size()

    arr.resize(10); // resize()
    EXPECT_EQ(arr.get_size(), 10);
}

TEST(DynamicArrayTest, OutOfRange) {
    DynamicArray<int> arr(5);
    EXPECT_THROW(arr.get(-1), std::out_of_range); 
    EXPECT_THROW(arr.get(10), std::out_of_range);
}

// =========================================================

TEST(LinkedListTest, Constructors) {
    LinkedList<int> empty_list; // LinkedList()
    EXPECT_EQ(empty_list.get_length(), 0);

    int items[] = {1, 5, 4};
    LinkedList<int> list_from_items(items, 3); // LinkedList(const T* items, int count)
    check_linked_list(list_from_items, items, 3);
    EXPECT_EQ(list_from_items.get_first(), 1);
    EXPECT_EQ(list_from_items.get_last(), 4);

    LinkedList<int> list_from_other(list_from_items); // LinkedList(const LinkedList<T>& other)
    check_linked_list(list_from_other, items, 3);
    EXPECT_EQ(list_from_other.get_first(), 1);
    EXPECT_EQ(list_from_other.get_last(), 4);
}

TEST(LinkedListTest, GetAndSet) {
    LinkedList<int> list;
    list.append(4); // append()
    list.append(5);
    list.prepend(8); // prepend()
    list.prepend(-4);
    list.insert_at(8, 3); // insert_at()

    int expected[] = {-4, 8, 4, 8, 5};
    check_linked_list(list, expected, 5);
    EXPECT_EQ(list.get_first(), -4); // get_first()
    EXPECT_EQ(list.get_last(), 5); // get_last()
}

TEST(LinkedListTest, OutOfRange) {
    LinkedList<int> list;
    EXPECT_THROW(list.get_first(), std::out_of_range);
    EXPECT_THROW(list.get_last(), std::out_of_range);
}

TEST(LinkedListTest, Concat) {
    LinkedList<int> first_list;
    first_list.append(4);
    first_list.append(10);

    LinkedList<int> second_list;
    second_list.append(-3);
    second_list.append(7);

    LinkedList<int>* concat_list = first_list.concat(&second_list); // concat
    int expected[] = {4, 10, -3, 7};
    check_linked_list(*concat_list, expected, 4);
    EXPECT_EQ(concat_list->get_first(), 4);
    EXPECT_EQ(concat_list->get_last(), 7);
    
    delete concat_list;
}

TEST(LinkedListTest, GetSubList) {
    int items[] = {1, 2, 3, 4, 5};
    LinkedList<int> list(items, 5);

    LinkedList<int>* sub_list = list.get_sub_list(1, 3); // get_sub_list
    int expected[] = {2, 3, 4};
    check_linked_list(*sub_list, expected, 3);
    EXPECT_EQ(sub_list->get_first(), 2);
    EXPECT_EQ(sub_list->get_last(), 4);

    delete sub_list;
}

TEST(LinkedListTest, Iterator) {
    int items[] = {1, 2, 3};
    LinkedList<int> list(items, 3);

    int sum = 0;
    EnumeratorWrapper<int> iter(list.get_enumerator());
    while(iter.move_next()) {
        sum += iter.get_current();
    }

    EXPECT_EQ(sum, 6);
}

// =======================

TEST(MutableArraySequenceTest, Constructors) { // Тестируем на Mutable варианте
    MutableArraySequence<int> empty_arr; // ArraySequence();
    EXPECT_EQ(empty_arr.get_count(), 0);
    
    int items[] = {1, 2, 3, 4};
    MutableArraySequence<int> arr(items, 4); // ArraySequence(const T* items, int count)
    check_sequence<int>(&arr, items, 4);
    EXPECT_EQ(arr.get_first(), 1);
    EXPECT_EQ(arr.get_last(), 4);

    DynamicArray<int> dyn_arr(items, 4);
    MutableArraySequence<int> arr_from_dyn_arr(dyn_arr); // ArraySequence(const DynamicArray<T>& other)
    check_sequence<int>(&arr_from_dyn_arr, items, 4);
    EXPECT_EQ(arr_from_dyn_arr.get_first(), 1);
    EXPECT_EQ(arr_from_dyn_arr.get_last(), 4);
    
    MutableArraySequence<int> arr_from_other(arr); // ArraySequence(const ArraySequence<T>& other)
    check_sequence<int>(&arr_from_other, items, 4);
    EXPECT_EQ(arr_from_other.get_first(), 1);
    EXPECT_EQ(arr_from_other.get_last(), 4);
}

TEST(MutableArraySequenceTest, GetAndSet) {
    MutableArraySequence<int> arr;
    arr.append(5);
    arr.append(10);
    arr.prepend(6);
    arr.prepend(-10);
    arr.insert_at(0, 3);

    int expected[] = {-10, 6, 5, 0, 10};
    check_sequence(&arr, expected, 5);
    EXPECT_EQ(arr.get_first(), -10);
    EXPECT_EQ(arr.get_last(), 10);
}

TEST(MutableArraySequenceTest, GetSubSequence) {
    int items[] = {1, 2, 3, 4, 5, 6, 7};
    MutableArraySequence<int> arr(items, 7);

    Sequence<int>* sub_arr = arr.get_sub_sequence(2, 5); // get_sub_sequence
    int expected[] = {3, 4, 5, 6};
    check_sequence<int>(sub_arr, expected, 4);
    EXPECT_EQ(sub_arr->get_first(), 3);
    EXPECT_EQ(sub_arr->get_last(), 6);

    delete sub_arr;
}

TEST(MutableArraySequenceTest, Concat) {
    int pos_items[] = {0, 2, 3, 4, 5};
    int neg_items[] = {-6, -7, -8, -9};

    MutableArraySequence<int> first_arr(pos_items, 5);
    MutableArraySequence<int> second_arr(neg_items, 4);

    Sequence<int>* concat_arr = first_arr.concat(&second_arr); // concat
    int expected[] = {0, 2, 3, 4, 5, -6, -7, -8, -9};
    check_sequence<int>(concat_arr, expected, 9);
    EXPECT_EQ(concat_arr->get_first(), 0);
    EXPECT_EQ(concat_arr->get_last(), -9);

    delete concat_arr;
}

TEST(MutableArraySequenceTest, Map) {
    int items[] = {1, -10, 3, 4, -5};
    MutableArraySequence<int> arr(items, 5);

    Sequence<int>* mapped_arr = arr.map(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence<int>(mapped_arr, expected, 5);
    EXPECT_EQ(mapped_arr->get_first(), 1);
    EXPECT_EQ(mapped_arr->get_last(), 25);

    delete mapped_arr;
}

TEST(MutableArraySequenceTest, Where) {
    int items[] = {1, -10, 3, 4, -5};
    MutableArraySequence<int> arr(items, 5);

    Sequence<int>* where_arr = arr.where(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence<int>(where_arr, expected, 3);
    EXPECT_EQ(where_arr->get_first(), 1);
    EXPECT_EQ(where_arr->get_last(), 4);

    delete where_arr;
}

TEST(MutableArraySequenceTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    MutableArraySequence<int> arr(items, 5);

    int reduced = arr.reduce(sum, 0);
    EXPECT_EQ(reduced, 15);
}

// =======================

TEST(ImmutableArraySequenceTest, Constructors) { // Тестируем на Immutable варианте
    ImmutableArraySequence<int> empty_arr; // ArraySequence();
    EXPECT_EQ(empty_arr.get_count(), 0);
    
    int items[] = {1, 2, 3, 4};
    ImmutableArraySequence<int> arr(items, 4); // ArraySequence(const T* items, int count)
    check_sequence<int>(&arr, items, 4);
    EXPECT_EQ(arr.get_first(), 1);
    EXPECT_EQ(arr.get_last(), 4);

    DynamicArray<int> dyn_arr(items, 4);
    ImmutableArraySequence<int> arr_from_dyn_arr(dyn_arr); // ArraySequence(const DynamicArray<T>& other)
    check_sequence<int>(&arr_from_dyn_arr, items, 4);
    EXPECT_EQ(arr_from_dyn_arr.get_first(), 1);
    EXPECT_EQ(arr_from_dyn_arr.get_last(), 4);
    
    ImmutableArraySequence<int> arr_from_other(arr); // ArraySequence(const ArraySequence<T>& other)
    check_sequence<int>(&arr_from_other, items, 4);
    EXPECT_EQ(arr_from_other.get_first(), 1);
    EXPECT_EQ(arr_from_other.get_last(), 4);
}

TEST(ImmutableArraySequenceTest, GetAndSet) {
    int items[] = {1, 2, 3};
    ImmutableArraySequence<int> arr_1(items, 3);

    Sequence<int>* arr_2 = arr_1.append(4);
    EXPECT_EQ(arr_1.get_count(), 3);
    int expected_2[] = {1, 2, 3, 4};
    check_sequence(arr_2, expected_2, 4);
    EXPECT_EQ(arr_2->get_last(), 4);

    Sequence<int>* arr_3 = arr_1.prepend(10);
    EXPECT_EQ(arr_1.get_count(), 3);
    int expected_3[] = {10, 1, 2, 3};
    check_sequence(arr_3, expected_3, 4);
    EXPECT_EQ(arr_3->get_first(), 10);

    Sequence<int>* arr_4 = arr_1.insert_at(0, 3);
    EXPECT_EQ(arr_1.get_count(), 3);
    int expected_4[] = {1, 2, 3, 0};
    check_sequence(arr_4, expected_4, 4);

    delete arr_2;
    delete arr_3;
    delete arr_4;
}

TEST(ImmutableArraySequenceTest, GetSubSequence) {
    int items[] = {1, 2, 3, 4, 5, 6, 7};
    ImmutableArraySequence<int> arr(items, 7);

    Sequence<int>* sub_arr = arr.get_sub_sequence(2, 5); // get_sub_sequence
    int expected[] = {3, 4, 5, 6};
    check_sequence<int>(sub_arr, expected, 4);
    EXPECT_EQ(sub_arr->get_first(), 3);
    EXPECT_EQ(sub_arr->get_last(), 6);

    delete sub_arr;
}

TEST(ImmutableArraySequenceTest, Concat) {
    int pos_items[] = {0, 2, 3, 4, 5};
    int neg_items[] = {-6, -7, -8, -9};

    ImmutableArraySequence<int> first_arr(pos_items, 5);
    ImmutableArraySequence<int> second_arr(neg_items, 4);

    Sequence<int>* concat_arr = first_arr.concat(&second_arr); // concat
    int expected[] = {0, 2, 3, 4, 5, -6, -7, -8, -9};
    check_sequence<int>(concat_arr, expected, 9);
    EXPECT_EQ(concat_arr->get_first(), 0);
    EXPECT_EQ(concat_arr->get_last(), -9);

    delete concat_arr;
}

TEST(ImmutableArraySequenceTest, Map) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableArraySequence<int> arr(items, 5);

    Sequence<int>* mapped_arr = arr.map(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence<int>(mapped_arr, expected, 5);
    EXPECT_EQ(mapped_arr->get_first(), 1);
    EXPECT_EQ(mapped_arr->get_last(), 25);

    delete mapped_arr;
}

TEST(ImmutableArraySequenceTest, Where) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableArraySequence<int> arr(items, 5);

    Sequence<int>* where_arr = arr.where(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence<int>(where_arr, expected, 3);
    EXPECT_EQ(where_arr->get_first(), 1);
    EXPECT_EQ(where_arr->get_last(), 4);

    delete where_arr;
}

TEST(ImmutableArraySequenceTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    ImmutableArraySequence<int> arr(items, 5);

    int reduced = arr.reduce(sum, 0);
    EXPECT_EQ(reduced, 15);
}

// =======================

TEST(MutableListSequenceTest, Constructors) { // Тестируем на Mutable варианте
    MutableListSequence<int> empty_list; // ListSequence()
    EXPECT_EQ(empty_list.get_count(), 0);
    
    int items[] = {1, 2, 3, 4};
    MutableListSequence<int> list(items, 4); // ListSequence(const T* items, int count)
    check_sequence<int>(&list, items, 4);
    EXPECT_EQ(list.get_first(), 1);
    EXPECT_EQ(list.get_last(), 4);

    LinkedList<int> link_list(items, 4);
    MutableListSequence<int> list_from_link_list(link_list); // ListSequence(const LinkedList<T>& other)
    check_sequence<int>(&list_from_link_list, items, 4);
    EXPECT_EQ(list_from_link_list.get_first(), 1);
    EXPECT_EQ(list_from_link_list.get_last(), 4);
    
    MutableListSequence<int> list_from_other(list_from_link_list); // ListSequence(const ArraySequence<T>& other)
    check_sequence<int>(&list_from_other, items, 4);
    EXPECT_EQ(list_from_other.get_first(), 1);
    EXPECT_EQ(list_from_other.get_last(), 4);
}

TEST(MutableListSequenceTest, GetAndSet) {
    MutableListSequence<int> list;
    list.append(5); // append()
    list.append(10);
    list.prepend(6); // prepend()
    list.prepend(-10);
    list.insert_at(0, 3); // insert_at()

    int expected[] = {-10, 6, 5, 0, 10};
    check_sequence<int>(&list, expected, 5);
    EXPECT_EQ(list.get_first(), -10); // get_first()
    EXPECT_EQ(list.get_last(), 10); // get_last()
}

TEST(MutableListSequenceTest, GetSubSequence) {
    int items[] = {1, 2, 3, 4, 5, 6, 7};
    MutableListSequence<int> list(items, 7);

    Sequence<int>* sub_list = list.get_sub_sequence(2, 5); // get_sub_sequence
    int expected[] = {3, 4, 5, 6};
    check_sequence<int>(sub_list, expected, 4);
    EXPECT_EQ(sub_list->get_first(), 3);
    EXPECT_EQ(sub_list->get_last(), 6);

    delete sub_list;
}

TEST(MutableListSequenceTest, Concat) {
    int pos_items[] = {0, 2, 3, 4, 5};
    int neg_items[] = {-6, -7, -8, -9};

    MutableListSequence<int> first_list(pos_items, 5);
    MutableListSequence<int> second_list(neg_items, 4);

    Sequence<int>* concat_list = first_list.concat(&second_list); // concat
    int expected[] = {0, 2, 3, 4, 5, -6, -7, -8, -9};
    check_sequence<int>(concat_list, expected, 9);
    EXPECT_EQ(concat_list->get_first(), 0);
    EXPECT_EQ(concat_list->get_last(), -9);

    delete concat_list;
}

TEST(MutableListSequenceTest, Map) {
    int items[] = {1, -10, 3, 4, -5};
    MutableListSequence<int> list(items, 5);

    Sequence<int>* mapped_list = list.map(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence<int>(mapped_list, expected, 5);
    EXPECT_EQ(mapped_list->get_first(), 1);
    EXPECT_EQ(mapped_list->get_last(), 25);

    delete mapped_list;
}

TEST(MutableListSequenceTest, Where) {
    int items[] = {1, -10, 3, 4, -5};
    MutableListSequence<int> list(items, 5);

    Sequence<int>* where_list = list.where(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence<int>(where_list, expected, 3);
    EXPECT_EQ(where_list->get_first(), 1);
    EXPECT_EQ(where_list->get_last(), 4);

    delete where_list;
}

TEST(MutableListSequenceTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    MutableListSequence<int> list(items, 5);

    int reduced = list.reduce(sum, 0);
    EXPECT_EQ(reduced, 15);
}

// =======================

TEST(ImmutableListSequenceTest, Constructors) { // Тестируем на Immutable варианте
    ImmutableListSequence<int> empty_list; // ListSequence();
    EXPECT_EQ(empty_list.get_count(), 0);
    
    int items[] = {1, 2, 3, 4};
    ImmutableListSequence<int> list(items, 4); // ListSequence(const T* items, int count)
    check_sequence<int>(&list, items, 4);
    EXPECT_EQ(list.get_first(), 1);
    EXPECT_EQ(list.get_last(), 4);

    LinkedList<int> link_list(items, 4);
    ImmutableListSequence<int> list_from_link_list(link_list); // ListSequence(const LinkedList<T>& other)
    check_sequence<int>(&list_from_link_list, items, 4);
    EXPECT_EQ(list_from_link_list.get_first(), 1);
    EXPECT_EQ(list_from_link_list.get_last(), 4);
    
    ImmutableListSequence<int> list_from_other(list); // ListSequence(const ListSequence<T>& other)
    check_sequence<int>(&list_from_other, items, 4);
    EXPECT_EQ(list_from_other.get_first(), 1);
    EXPECT_EQ(list_from_other.get_last(), 4);
}

TEST(ImmutableListSequenceTest, GetAndSet) {
    int items[] = {1, 2, 3};
    ImmutableListSequence<int> list_1(items, 3);

    Sequence<int>* list_2 = list_1.append(4); // append()
    EXPECT_EQ(list_1.get_count(), 3); // Размер arr не меняется
    int expected_2[] = {1, 2, 3, 4};
    check_sequence<int>(list_2, expected_2, 4); // Добавляется в копию
    EXPECT_EQ(list_2->get_last(), 4);

    Sequence<int>* list_3 = list_1.prepend(10); // prepend()
    EXPECT_EQ(list_1.get_count(), 3);
    int expected_3[] = {10, 1, 2, 3};
    check_sequence<int>(list_3, expected_3, 4);
    EXPECT_EQ(list_3->get_first(), 10);
    
    Sequence<int>* list_4 = list_1.insert_at(0, 3);
    EXPECT_EQ(list_1.get_count(), 3);
    int expected_4[] = {1, 2, 3, 0};
    check_sequence<int>(list_4, expected_4, 4);
    
    delete list_2;
    delete list_3;
    delete list_4;
}

TEST(ImmutableListSequenceTest, GetSubSequence) {
    int items[] = {1, 2, 3, 4, 5, 6, 7};
    ImmutableListSequence<int> list(items, 7);

    Sequence<int>* sub_list = list.get_sub_sequence(2, 5); // get_sub_sequence
    int expected[] = {3, 4, 5, 6};
    check_sequence<int>(sub_list, expected, 4);
    EXPECT_EQ(sub_list->get_first(), 3);
    EXPECT_EQ(sub_list->get_last(), 6);

    delete sub_list;
}

TEST(ImmutableListSequenceTest, Concat) {
    int pos_items[] = {0, 2, 3, 4, 5};
    int neg_items[] = {-6, -7, -8, -9};

    ImmutableListSequence<int> first_list(pos_items, 5);
    ImmutableListSequence<int> second_list(neg_items, 4);

    Sequence<int>* concat_list = first_list.concat(&second_list); // concat
    int expected[] = {0, 2, 3, 4, 5, -6, -7, -8, -9};
    check_sequence<int>(concat_list, expected, 9);
    EXPECT_EQ(concat_list->get_first(), 0);
    EXPECT_EQ(concat_list->get_last(), -9);

    delete concat_list;
}

TEST(ImmutableListSequenceTest, Map) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableListSequence<int> list(items, 5);

    Sequence<int>* mapped_list = list.map(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence<int>(mapped_list, expected, 5);
    EXPECT_EQ(mapped_list->get_first(), 1);
    EXPECT_EQ(mapped_list->get_last(), 25);

    delete mapped_list;
}

TEST(ImmutableListSequenceTest, Where) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableListSequence<int> list(items, 5);

    Sequence<int>* where_list = list.where(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence<int>(where_list, expected, 3);
    EXPECT_EQ(where_list->get_first(), 1);
    EXPECT_EQ(where_list->get_last(), 4);

    delete where_list;
}

TEST(ImmutableListSequenceTest, Reduce) {
    int items[] = {1, 2, 3, 4, 5};
    ImmutableListSequence<int> list(items, 5);

    int reduced = list.reduce(sum, 0);
    EXPECT_EQ(reduced, 15);
}

// =======================

TEST(BitTest, Operations) {
    Bit a(1), b(0), c(42);
    EXPECT_EQ(c.get(), true);
    EXPECT_EQ((a & b).get(), false);
    EXPECT_EQ((a | b).get(), true);
    EXPECT_EQ((a ^ b).get(), true);
    EXPECT_EQ((~a).get(), false);
    EXPECT_EQ((~b).get(), true);
}

TEST(BitSequenceTest, ConstructFromArray) {
    Bit items[] = {Bit(1), Bit(0), Bit(1), Bit(1)};
    BitSequence seq(items, 4);

    check_sequence<Bit>(&seq, items, 4);
}

TEST(BitSequenceTest, BitAnd) {
    BitSequence first;
    BitSequence second;

    first.append(Bit(1));
    first.append(Bit(0));
    first.append(Bit(1));

    second.append(Bit(1));
    second.append(Bit(1));
    second.append(Bit(0));

    BitSequence* seq_and = first.bit_and(second);
    Bit expected[] = {Bit(1), Bit(0), Bit(0)};
    check_sequence<Bit>(seq_and, expected, 3);

    delete seq_and;
}

TEST(BitSequenceTest, BitOr) {
    BitSequence first;
    BitSequence second;

    first.append(Bit(1));
    first.append(Bit(0));
    first.append(Bit(0));

    second.append(Bit(1));
    second.append(Bit(1));
    second.append(Bit(0));

    BitSequence* seq_or = first.bit_or(second);
    Bit expected[] = {Bit(1), Bit(1), Bit(0)};
    check_sequence<Bit>(seq_or, expected, 3);

    delete seq_or;
}

TEST(BitSequenceTest, BitXOR) {
    BitSequence first;
    BitSequence second;

    first.append(Bit(1));
    first.append(Bit(0));
    first.append(Bit(0));

    second.append(Bit(1));
    second.append(Bit(1));
    second.append(Bit(0));

    BitSequence* seq_xor = first.bit_xor(second);
    Bit expected[] = {Bit(0), Bit(1), Bit(0)};
    check_sequence<Bit>(seq_xor, expected, 3);

    delete seq_xor;
}

TEST(BitSequenceTest, BitNot) {
    BitSequence first;

    first.append(Bit(1));
    first.append(Bit(0));
    first.append(Bit(0));

    BitSequence* seq_not = first.bit_not();
    Bit expected[] = {Bit(0), Bit(1), Bit(1)};
    check_sequence<Bit>(seq_not, expected, 3);

    delete seq_not;
}

// =======================

TEST(OptionTest, Some) {
    Option<int> opt = Option<int>::Some(42);
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(opt.get_value(), 42);
}

TEST(OptionTest, None) {
    Option<int> opt = Option<int>::None();
    EXPECT_FALSE(opt.has_value());
    EXPECT_THROW(opt.get_value(), std::runtime_error);
}

TEST(OptionTest, TryGetArraySequence) {
    MutableArraySequence<int> seq;

    EXPECT_FALSE(seq.try_get_first().has_value());
    EXPECT_FALSE(seq.try_get_last().has_value());
    EXPECT_FALSE(seq.try_get(0).has_value());

    seq.append(10);
    seq.append(20);

    Option<int> first = seq.try_get_first();
    EXPECT_TRUE(first.has_value());
    EXPECT_EQ(first.get_value(), 10);

    Option<int> last = seq.try_get_last();
    EXPECT_TRUE(last.has_value());
    EXPECT_EQ(last.get_value(), 20);

    Option<int> elem = seq.try_get(1);
    EXPECT_TRUE(elem.has_value());
    EXPECT_EQ(elem.get_value(), 20);

    EXPECT_FALSE(seq.try_get(-1).has_value());
    EXPECT_FALSE(seq.try_get(5).has_value());
}

TEST(OptionTest, TryGetListSequence) {
    MutableArraySequence<int> seq;

    EXPECT_FALSE(seq.try_get_first().has_value());
    EXPECT_FALSE(seq.try_get_last().has_value());

    seq.append(10);
    seq.append(20);

    EXPECT_TRUE(seq.try_get_first().has_value());
    EXPECT_EQ(seq.try_get_first().get_value(), 10);
    EXPECT_EQ(seq.try_get(1).get_value(), 20);
}

// =======================

TEST(ArrayOfArraysTest, CopyConstructor) {
    DynamicArray<int> row(2);
    row.set(0, 5); row.set(1, 6);

    DynamicArray<DynamicArray<int>> original(1);
    original.set(0, row);

    DynamicArray<DynamicArray<int>> copy(original);
    EXPECT_EQ(copy.get(0).get(0), 5);
    EXPECT_EQ(copy.get(0).get(1), 6);
}

TEST(ArrayOfArraysTest, CreateAndAccess) {
    DynamicArray<int> row_1(3);
    row_1.set(0, 1); 
    row_1.set(1, 2); 
    row_1.set(2, 3);

    DynamicArray<int> row_2(2);
    row_2.set(0, 10); 
    row_2.set(1, 20);

    DynamicArray<DynamicArray<int>> matrix(2);
    matrix.set(0, row_1);
    matrix.set(1, row_2);

    EXPECT_EQ(matrix.get(0).get(0), 1);
    EXPECT_EQ(matrix.get(0).get(2), 3);
    EXPECT_EQ(matrix.get(1).get(0), 10);
    EXPECT_EQ(matrix.get(1).get(1), 20);
    EXPECT_EQ(matrix.get_size(), 2);
}

TEST(ArrayOfArraysTest, IndependentCopy) {
    DynamicArray<int> row(2);
    row.set(0, 1); row.set(1, 2);

    DynamicArray<DynamicArray<int>> original(1);
    original.set(0, row);

    DynamicArray<DynamicArray<int>> copy(original);

    DynamicArray<int> newRow(2); // меняем оригинал
    newRow.set(0, 99); newRow.set(1, 100);
    original.set(0, newRow);
    
    EXPECT_EQ(copy.get(0).get(0), 1); // копия не изменилась
    EXPECT_EQ(copy.get(0).get(1), 2);
}

TEST(ListOfArraysTest, CopyConstructor) {
    LinkedList<DynamicArray<int>> original;

    DynamicArray<int> row(2);
    row.set(0, 7); row.set(1, 8);
    original.append(row);

    LinkedList<DynamicArray<int>> copy(original);
    EXPECT_EQ(copy.get_first().get(0), 7);
    EXPECT_EQ(copy.get_first().get(1), 8);
}

TEST(ListOfArraysTest, AppendAndAccess) {
    LinkedList<DynamicArray<int>> list;

    DynamicArray<int> row_1(3);
    row_1.set(0, 1);
    row_1.set(1, 2);
    row_1.set(2, 3);

    DynamicArray<int> row_2(2);
    row_2.set(0, 10); 
    row_2.set(1, 20);

    list.append(row_1);
    list.append(row_2);

    EXPECT_EQ(list.get_length(), 2);
    EXPECT_EQ(list.get_first().get(0), 1);
    EXPECT_EQ(list.get_first().get(2), 3);
    EXPECT_EQ(list.get_last().get(0), 10);
}

TEST(ListOfArraysTest, PrependAndAccess) {
    LinkedList<DynamicArray<int>> list;

    DynamicArray<int> row_1(2);
    row_1.set(0, 1); 
    row_1.set(1, 2);

    DynamicArray<int> row_2(2);
    row_2.set(0, 3); 
    row_2.set(1, 4);

    list.append(row_1);
    list.prepend(row_2);

    EXPECT_EQ(list.get_first().get(0), 3);
    EXPECT_EQ(list.get_last().get(0), 1);
}

TEST(ListOfArraysTest, IndependentCopy) {
    LinkedList<DynamicArray<int>> original;

    DynamicArray<int> row(2);
    row.set(0, 1); row.set(1, 2);
    original.append(row);

    LinkedList<DynamicArray<int>> copy(original);

    DynamicArray<int> new_row(2); // меняем оригинал
    new_row.set(0, 99); new_row.set(1, 100);

    EXPECT_EQ(copy.get_first().get(0), 1); // копия не изменилась
}

// =======================

TEST(SplitTest, MutableArrayBasic) {
    int items[] = {1, 2, 0, 3, 4, 0, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 7);

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 3);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    int fragment_index = 0;
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        if (fragment_index == 0) {
            int expected[] = {1, 2};
            check_sequence(fragment, expected, 2);
        } else if (fragment_index == 1) {
            int expected[] = {3, 4};
            check_sequence(fragment, expected, 2);
        } else if (fragment_index == 2) {
            int expected[] = {5};
            check_sequence(fragment, expected, 1);
        }
        delete fragment;
        fragment_index++;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, MutableListBasic) {
    int items[] = {1, 2, 0, 3, 4, 0, 5};
    Sequence<int>* seq = new MutableListSequence<int>(items, 7);

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 3);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    int fragment_index = 0;
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        EXPECT_EQ(fragment->get_count(), (fragment_index == 2) ? 1 : 2);
        delete fragment;
        fragment_index++;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, DelimiterAtStart) {
    int items[] = {0, 1, 2, 3};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 4);

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 2);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    int fragment_index = 0;
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        if (fragment_index == 0) {
            EXPECT_EQ(fragment->get_count(), 0);
        } else if (fragment_index == 1) {
            int expected[] = {1, 2, 3};
            check_sequence(fragment, expected, 3);
        }
        delete fragment;
        fragment_index++;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, DelimiterAtEnd) {
    int items[] = {1, 2, 3, 0};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 4);

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 2);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    int fragment_index = 0;
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        if (fragment_index == 0) {
            int expected[] = {1, 2, 3};
            check_sequence(fragment, expected, 3);
        } else if (fragment_index == 1) {
            EXPECT_EQ(fragment->get_count(), 0);
        }
        delete fragment;
        fragment_index++;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, NoDelimiter) {
    int items[] = {1, 2, 3};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 3);

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 1);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        int expected[] = {1, 2, 3};
        check_sequence(fragment, expected, 3);
        delete fragment;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, EmptySequence) {
    Sequence<int>* seq = new MutableArraySequence<int>();

    auto* split_seq = split(seq, is_zero);

    EXPECT_EQ(split_seq->get_count(), 1);

    EnumeratorWrapper<Sequence<int>*> iter(split_seq->get_enumerator());
    while (iter.move_next()) {
        Sequence<int>* fragment = iter.get_current();
        EXPECT_EQ(fragment->get_count(), 0);
        delete fragment;
    }

    delete split_seq;
    delete seq;
}

TEST(SplitTest, BitSequenceSplit) {
    BitSequence seq;
    seq.append(Bit(1));
    seq.append(Bit(0));
    seq.append(Bit(1));
    seq.append(Bit(1));
    seq.append(Bit(0));
    seq.append(Bit(1));

    auto* split_seq = split(&seq, is_zero_bit);

    EXPECT_EQ(split_seq->get_count(), 3);

    EnumeratorWrapper<Sequence<Bit>*> iter(split_seq->get_enumerator());
    int fragment_index = 0;
    while (iter.move_next()) {
        Sequence<Bit>* fragment = iter.get_current();
        if (fragment_index == 0) {
            Bit expected[] = {Bit(1)};
            check_sequence(fragment, expected, 1);
        } else if (fragment_index == 1) {
            Bit expected[] = {Bit(1), Bit(1)};
            check_sequence(fragment, expected, 2);
        } else if (fragment_index == 2) {
            Bit expected[] = {Bit(1)};
            check_sequence(fragment, expected, 1);
        }
        delete fragment;
        fragment_index++;
    }

    delete split_seq;
}

// =======================

TEST(SliceTest, MutableArrayBasic) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 5);

    int rep_items[] = {9, 10};
    Sequence<int>* replacement = new MutableArraySequence<int>(rep_items, 2);

    Sequence<int>* sliced_seq = seq->slice(1, 2, replacement);

    int expected[] = {1, 9, 10, 4, 5};
    check_sequence<int>(sliced_seq, expected, 5);

    delete sliced_seq;
    delete replacement;
    delete seq;
}

TEST(SliceTest, MutableListBasic) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableListSequence<int>(items, 5);

    int rep_items[] = {9, 10};
    Sequence<int>* replacement = new MutableListSequence<int>(rep_items, 2);

    Sequence<int>* sliced_seq = seq->slice(1, 2, replacement);

    int expected[] = {1, 9, 10, 4, 5};
    check_sequence<int>(sliced_seq, expected, 5);

    delete sliced_seq;
    delete replacement;
    delete seq;
}

TEST(SliceTest, DeleteWithoutReplacement) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 5);

    Sequence<int>* sliced_seq = seq->slice(1, 2);

    int expected[] = {1, 4, 5};
    check_sequence<int>(sliced_seq, expected, 3);

    delete sliced_seq;
    delete seq;
}

TEST(SliceTest, NegativeIndex) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 5);

    Sequence<int>* sliced_seq = seq->slice(-2, 1); // index = 3, удаляем 1 элемент

    int expected[] = {1, 2, 3, 5};
    check_sequence<int>(sliced_seq, expected, 4);

    delete sliced_seq;
    delete seq;
}

TEST(SliceTest, NegativeIndexWithReplacement) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 5);

    int rep_items[] = {99};
    Sequence<int>* replacement = new MutableArraySequence<int>(rep_items, 1);

    Sequence<int>* sliced_seq = seq->slice(-3, 2, replacement); // index = 2, удаляем 2

    int expected[] = {1, 2, 99, 5};
    check_sequence<int>(sliced_seq, expected, 4);

    delete sliced_seq;
    delete replacement;
    delete seq;
}

TEST(SliceTest, IndexOutOfRange) {
    int items[] = {1, 2, 3};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 3);

    EXPECT_THROW(seq->slice(5, 1), std::out_of_range);
    EXPECT_THROW(seq->slice(-4, 1), std::out_of_range);

    delete seq;
}

TEST(SliceTest, DeleteMoreThanAvailable) {
    int items[] = {1, 2, 3, 4, 5};
    Sequence<int>* seq = new MutableArraySequence<int>(items, 5);

    Sequence<int>* sliced_seq = seq->slice(3, 100); // просит 100, но от позиции 3 только 2

    int expected[] = {1, 2, 3};
    check_sequence<int>(sliced_seq, expected, 3);

    delete sliced_seq;
    delete seq;
}

TEST(SliceTest, BitSequenceSlice) {
    BitSequence seq;
    seq.append(Bit(1));
    seq.append(Bit(0));
    seq.append(Bit(1));
    seq.append(Bit(0));
    seq.append(Bit(1));

    Sequence<Bit>* sliced_seq = seq.slice(1, 2); // удаляем 2 бита с позиции 1

    Bit expected[] = {Bit(1), Bit(0), Bit(1)};
    check_sequence<Bit>(sliced_seq, expected, 3);

    delete sliced_seq;
}

// =======================

TEST(SequencePolymorhTest, ArrayAndList) {
    Sequence<int>* arr = new MutableArraySequence<int>();
    Sequence<int>* list = new MutableListSequence<int>();

    arr->append(1);
    list->append(1);

    EXPECT_EQ(arr->get_first(), list->get_first());

    delete arr;
    delete list;
}

// =========================

TEST(CRTPTest, MutableArrayClone) {
    int items[] = {1, 2, 3};
    MutableArraySequence<int> original(items, 3);

    MutableArraySequence<int>* copy = original.Clone();

    int expected[] = {1, 2, 3};
    check_sequence(copy, expected, 3);

    // Меняем копию — оригинал не затрагивается
    copy->append(4);
    EXPECT_EQ(original.get_count(), 3);
    EXPECT_EQ(copy->get_count(), 4);

    delete copy;
}

TEST(CRTPTest, MutableArrayEmpty) {
    int items[] = {1, 2, 3};
    MutableArraySequence<int> original(items, 3);

    MutableArraySequence<int>* empty_arr = original.Empty();
    EXPECT_EQ(empty_arr->get_count(), 0);

    // Тип результата — именно MutableArraySequence, не абстрактный Sequence
    empty_arr->append(99);
    EXPECT_EQ(empty_arr->get_count(), 1);
    EXPECT_EQ(empty_arr->get_first(), 99);

    delete empty_arr;
}

TEST(CRTPTest, ImmutableArrayClone) {
    int items[] = {10, 20, 30};
    ImmutableArraySequence<int> original(items, 3);

    ImmutableArraySequence<int>* copy = original.Clone();

    int expected[] = {10, 20, 30};
    check_sequence(copy, expected, 3);
    EXPECT_EQ(copy->get_count(), 3);

    delete copy;
}

TEST(CRTPTest, ImmutableArrayEmpty) {
    int items[] = {1, 2, 3};
    ImmutableArraySequence<int> original(items, 3);

    ImmutableArraySequence<int>* empty_arr = original.Empty();
    EXPECT_EQ(empty_arr->get_count(), 0);

    delete empty_arr;
}

TEST(CRTPTest, MutableListClone) {
    int items[] = {1, 2, 3};
    MutableListSequence<int> original(items, 3);

    MutableListSequence<int>* copy = original.Clone();

    int expected[] = {1, 2, 3};
    check_sequence(copy, expected, 3);

    copy->append(4);
    EXPECT_EQ(original.get_count(), 3);
    EXPECT_EQ(copy->get_count(), 4);

    delete copy;
}

TEST(CRTPTest, MutableListEmpty) {
    int items[] = {1, 2, 3};
    MutableListSequence<int> original(items, 3);

    MutableListSequence<int>* empty_list = original.Empty();
    EXPECT_EQ(empty_list->get_count(), 0);

    empty_list->append(99);
    EXPECT_EQ(empty_list->get_count(), 1);
    EXPECT_EQ(empty_list->get_first(), 99);

    delete empty_list;
}

TEST(CRTPTest, ImmutableListClone) {
    int items[] = {10, 20, 30};
    ImmutableListSequence<int> original(items, 3);

    ImmutableListSequence<int>* copy = original.Clone();

    int expected[] = {10, 20, 30};
    check_sequence(copy, expected, 3);
    EXPECT_EQ(copy->get_count(), 3);

    delete copy;
}

TEST(CRTPTest, ImmutableListEmpty) {
    int items[] = {1, 2, 3};
    ImmutableListSequence<int> original(items, 3);

    ImmutableListSequence<int>* empty_list = original.Empty();
    EXPECT_EQ(empty_list->get_count(), 0);

    delete empty_list;
}

TEST(CRTPTest, CloneReturnsDefinedType) {
    MutableArraySequence<int> arr;
    arr.append(1);

    MutableArraySequence<int>* typed_copy = arr.Clone();
    EXPECT_EQ(typed_copy->get_count(), 1);

    delete typed_copy;
}

// =========================

TEST(ZipTest, BasicZip) {
    int a_items[] = {1, 2, 3};
    double b_items[] = {1.1, 2.2, 3.3};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 3);
    Sequence<double>* b = new MutableArraySequence<double>(b_items, 3);

    Sequence<Pair<int, double>>* zipped = zip(a, b);

    EXPECT_EQ(zipped->get_count(), 3);
    EXPECT_EQ(zipped->get_first().first(), 1);
    EXPECT_EQ(zipped->get_first().second(), 1.1);
    EXPECT_EQ(zipped->get_last().first(), 3);

    delete zipped;
    delete a;
    delete b;
}

TEST(ZipTest, DifferentLengths) {
    int a_items[] = {1, 2, 3, 4, 5};
    int b_items[] = {10, 20};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 5);
    Sequence<int>* b = new MutableArraySequence<int>(b_items, 2);

    Sequence<Pair<int, int>>* zipped = zip(a, b);

    // Размер = минимум из двух
    EXPECT_EQ(zipped->get_count(), 2);

    delete zipped;
    delete a;
    delete b;
}

TEST(ZipTest, ZipUnzip) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {10, 20, 30};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 3);
    Sequence<int>* b = new MutableArraySequence<int>(b_items, 3);

    Sequence<Pair<int, int>>* zipped = zip(a, b);
    Pair<Sequence<int>*, Sequence<int>*> unzipped = unzip(zipped);

    EXPECT_EQ(unzipped.first()->get_count(), 3);
    EXPECT_EQ(unzipped.second()->get_count(), 3);
    EXPECT_EQ(unzipped.first()->get_first(), 1);
    EXPECT_EQ(unzipped.second()->get_first(), 10);

    delete zipped;
    delete unzipped.first();
    delete unzipped.second();
    delete a;
    delete b;
}

// =========================

TEST(Zip3Test, BasicZip3) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {10, 20, 30};
    int c_items[] = {100, 200, 300};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 3);
    Sequence<int>* b = new MutableArraySequence<int>(b_items, 3);
    Sequence<int>* c = new MutableArraySequence<int>(c_items, 3);

    auto* zipped = zip3(a, b, c);

    EXPECT_EQ(zipped->get_count(), 3);

    const Triple<int, int, int>& first_triple = zipped->get_first();
    EXPECT_EQ(first_triple.first(), 1);
    EXPECT_EQ(first_triple.second(), 10);
    EXPECT_EQ(first_triple.third(), 100);

    const Triple<int, int, int>& last_triple = zipped->get_last();
    EXPECT_EQ(last_triple.first(), 3);
    EXPECT_EQ(last_triple.second(), 30);
    EXPECT_EQ(last_triple.third(), 300);

    delete zipped;
    delete a;
    delete b;
    delete c;
}

TEST(Zip3Test, DifferentLengths) {
    int a_items[] = {1, 2, 3, 4, 5};
    int b_items[] = {10, 20};
    int c_items[] = {100, 200, 300};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 5);
    Sequence<int>* b = new MutableArraySequence<int>(b_items, 2);
    Sequence<int>* c = new MutableArraySequence<int>(c_items, 3);

    auto* zipped = zip3(a, b, c);

    EXPECT_EQ(zipped->get_count(), 2);

    delete zipped;
    delete a;
    delete b;
    delete c;
}

TEST(Zip3Test, Zip3Unzip3) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {10, 20, 30};
    int c_items[] = {100, 200, 300};

    Sequence<int>* a = new MutableArraySequence<int>(a_items, 3);
    Sequence<int>* b = new MutableArraySequence<int>(b_items, 3);
    Sequence<int>* c = new MutableArraySequence<int>(c_items, 3);

    auto* zipped = zip3(a, b, c);
    auto unzipped = unzip3(zipped);

    EXPECT_EQ(unzipped.first()->get_count(), 3);
    EXPECT_EQ(unzipped.second()->get_count(), 3);
    EXPECT_EQ(unzipped.third()->get_count(), 3);

    EXPECT_EQ(unzipped.first()->get_first(), 1);
    EXPECT_EQ(unzipped.second()->get_first(), 10);
    EXPECT_EQ(unzipped.third()->get_first(), 100);

    delete zipped;
    delete unzipped.first();
    delete unzipped.second();
    delete unzipped.third();
    delete a;
    delete b;
    delete c;
}

TEST(Zip3Test, NullptrThrows) {
    int items[] = {1, 2, 3};
    Sequence<int>* a = new MutableArraySequence<int>(items, 3);

    EXPECT_THROW((zip3<int, int, int>(nullptr, a, a)), std::invalid_argument);
    EXPECT_THROW((zip3<int, int, int>(a, nullptr, a)), std::invalid_argument);
    EXPECT_THROW((zip3<int, int, int>(a, a, nullptr)), std::invalid_argument);

    delete a;
}

// ======================= map_define

TEST(DefineTest, MapDefineMutableArray) {
    int items[] = {1, -10, 3, 4, -5};
    MutableArraySequence<int> arr(items, 5);

    MutableArraySequence<int>* result = arr.map_defined(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, MapDefineImmutableArray) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableArraySequence<int> arr(items, 5);

    ImmutableArraySequence<int>* result = arr.map_defined(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, MapDefineMutableList) {
    int items[] = {1, -10, 3, 4, -5};
    MutableListSequence<int> list(items, 5);

    MutableListSequence<int>* result = list.map_defined(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, MapDefineImmutableList) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableListSequence<int> list(items, 5);

    ImmutableListSequence<int>* result = list.map_defined(square);
    int expected[] = {1, 100, 9, 16, 25};
    check_sequence(result, expected, 5);

    delete result;
}

// ======================= where_define

TEST(DefineTest, WhereDefineMutableArray) {
    int items[] = {1, -10, 3, 4, -5};
    MutableArraySequence<int> arr(items, 5);

    MutableArraySequence<int>* result = arr.where_defined(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence(result, expected, 3);

    delete result;
}

TEST(DefineTest, WhereDefineImmutableArray) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableArraySequence<int> arr(items, 5);

    ImmutableArraySequence<int>* result = arr.where_defined(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence(result, expected, 3);

    delete result;
}

TEST(DefineTest, WhereDefineMutableList) {
    int items[] = {1, -10, 3, 4, -5};
    MutableListSequence<int> list(items, 5);

    MutableListSequence<int>* result = list.where_defined(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence(result, expected, 3);

    delete result;
}

TEST(DefineTest, WhereDefineImmutableList) {
    int items[] = {1, -10, 3, 4, -5};
    ImmutableListSequence<int> list(items, 5);

    ImmutableListSequence<int>* result = list.where_defined(is_positive);
    int expected[] = {1, 3, 4};
    check_sequence(result, expected, 3);

    delete result;
}

// ======================= concat_define

TEST(DefineTest, ConcatDefineMutableArray) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {4, 5, 6};

    MutableArraySequence<int> a(a_items, 3);
    MutableArraySequence<int> b(b_items, 3);

    MutableArraySequence<int>* result = a.concat_defined(&b);
    int expected[] = {1, 2, 3, 4, 5, 6};
    check_sequence(result, expected, 6);

    delete result;
}

TEST(DefineTest, ConcatDefineImmutableArray) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {4, 5, 6};

    ImmutableArraySequence<int> a(a_items, 3);
    ImmutableArraySequence<int> b(b_items, 3);

    ImmutableArraySequence<int>* result = a.concat_defined(&b);
    int expected[] = {1, 2, 3, 4, 5, 6};
    check_sequence(result, expected, 6);

    delete result;
}

TEST(DefineTest, ConcatDefineMutableList) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {4, 5, 6};

    MutableListSequence<int> a(a_items, 3);
    MutableListSequence<int> b(b_items, 3);

    MutableListSequence<int>* result = a.concat_defined(&b);
    int expected[] = {1, 2, 3, 4, 5, 6};
    check_sequence(result, expected, 6);

    delete result;
}

TEST(DefineTest, ConcatDefineImmutableList) {
    int a_items[] = {1, 2, 3};
    int b_items[] = {4, 5, 6};

    ImmutableListSequence<int> a(a_items, 3);
    ImmutableListSequence<int> b(b_items, 3);

    ImmutableListSequence<int>* result = a.concat_defined(&b);
    int expected[] = {1, 2, 3, 4, 5, 6};
    check_sequence(result, expected, 6);

    delete result;
}

// ======================= slice_define


TEST(DefineTest, SliceDefineMutableArray) {
    int items[] = {1, 2, 3, 4, 5};
    MutableArraySequence<int> arr(items, 5);

    int rep_items[] = {9, 10};
    MutableArraySequence<int> replacement(rep_items, 2);

    MutableArraySequence<int>* result = arr.slice_defined(1, 2, &replacement);
    int expected[] = {1, 9, 10, 4, 5};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, SliceDefineImmutableArray) {
    int items[] = {1, 2, 3, 4, 5};
    ImmutableArraySequence<int> arr(items, 5);

    int rep_items[] = {9, 10};
    ImmutableArraySequence<int> replacement(rep_items, 2);

    ImmutableArraySequence<int>* result = arr.slice_defined(1, 2, &replacement);
    int expected[] = {1, 9, 10, 4, 5};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, SliceDefineMutableList) {
    int items[] = {1, 2, 3, 4, 5};
    MutableListSequence<int> list(items, 5);

    int rep_items[] = {9, 10};
    MutableListSequence<int> replacement(rep_items, 2);

    MutableListSequence<int>* result = list.slice_defined(1, 2, &replacement);
    int expected[] = {1, 9, 10, 4, 5};
    check_sequence(result, expected, 5);

    delete result;
}

TEST(DefineTest, SliceDefineImmutableList) {
    int items[] = {1, 2, 3, 4, 5};
    ImmutableListSequence<int> list(items, 5);

    int rep_items[] = {9, 10};
    ImmutableListSequence<int> replacement(rep_items, 2);

    ImmutableListSequence<int>* result = list.slice_defined(1, 2, &replacement);
    int expected[] = {1, 9, 10, 4, 5};
    check_sequence(result, expected, 5);

    delete result;
}

// ======================= chain_append

TEST(ChainTest, ChainAppendMutableArray) {
    MutableArraySequence<int> seq;
    seq.chain_append(1).chain_append(2).chain_append(3);

    int expected[] = {1, 2, 3};
    check_sequence(&seq, expected, 3);
}

TEST(ChainTest, ChainAppendMutableList) {
    MutableListSequence<int> list;
    list.chain_append(10).chain_append(20).chain_append(30);

    int expected[] = {10, 20, 30};
    check_sequence(&list, expected, 3);
}

// ======================= chain_prepend

TEST(ChainTest, ChainPrependMutableArray) {
    MutableArraySequence<int> seq;
    seq.chain_prepend(3).chain_prepend(2).chain_prepend(1);

    int expected[] = {1, 2, 3};
    check_sequence(&seq, expected, 3);
}

TEST(ChainTest, ChainPrependMutableList) {
    MutableListSequence<int> list;
    list.chain_prepend(30).chain_prepend(20).chain_prepend(10);

    int expected[] = {10, 20, 30};
    check_sequence(&list, expected, 3);
}

// ======================= chain_insert_at

TEST(ChainTest, ChainInsertAtMutableArray) {
    int items[] = {1, 5};
    MutableArraySequence<int> seq(items, 2);

    seq.chain_insert_at(2, 1).chain_insert_at(3, 2).chain_insert_at(4, 3);

    int expected[] = {1, 2, 3, 4, 5};
    check_sequence(&seq, expected, 5);
}

TEST(ChainTest, ChainInsertAtMutableList) {
    int items[] = {1, 5};
    MutableListSequence<int> list(items, 2);

    list.chain_insert_at(2, 1).chain_insert_at(3, 2).chain_insert_at(4, 3);

    int expected[] = {1, 2, 3, 4, 5};
    check_sequence(&list, expected, 5);
}

// ======================= Смешанные цепочки

TEST(ChainTest, MixedChainMutableArray) {
    MutableArraySequence<int> seq;
    seq.chain_append(2).chain_append(3).chain_prepend(1).chain_insert_at(4, 3);

    int expected[] = {1, 2, 3, 4};
    check_sequence(&seq, expected, 4);
}

TEST(ChainTest, MixedChainMutableList) {
    MutableListSequence<int> list;
    list.chain_append(20).chain_append(30).chain_prepend(10).chain_insert_at(40, 3);

    int expected[] = {10, 20, 30, 40};
    check_sequence(&list, expected, 4);
}