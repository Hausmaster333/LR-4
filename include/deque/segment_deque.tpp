#include "deque/segment_deque.h"
#include <stdexcept>

template <class T>
void SegmentedDeque<T>::sys_push_front(const T& item) {
    front_index--;

    if (front_index < 0) {
        front_block--;
        front_index = segment_size - 1;

        if (front_block < 0) {
            grow_map_front();
        }

        if (block_map.get(front_block) == nullptr) {
            block_map.set(front_block, allocate_block());
        }
    }  

    T* curr_block = block_map.get(front_block);
    curr_block[front_index] = item;

    count++;
}

template <class T>
T SegmentedDeque<T>::sys_pop_back() {
    if (count == 0) throw std::out_of_range("Deque is empty");

    back_index--;

    if (back_index < 0) {
        back_block--;
        back_index = segment_size - 1;
    }

    T* curr_block = block_map.get(back_block);
    T curr_elem = curr_block[back_index];

    count--;

    for (int index = back_block + 2; index < map_capacity; index++) {
        if (block_map.get(index) != nullptr) {
            delete[] block_map.get(index);

            block_map.set(index, nullptr);
        }
    }

    shrink_map();

    return curr_elem;
}

template <class T>
T SegmentedDeque<T>::sys_pop_front() {;
    if (count == 0) throw std::out_of_range("Deque is empty");

    T* curr_block = block_map.get(front_block);
    T curr_elem = curr_block[front_index];

    front_index++;

    if (front_index >= segment_size) {
        front_block++;
        front_index = 0;
    }

    count--;

    for (int index = front_block - 2; index >= 0; index--) {
        if (block_map.get(index) != nullptr) {
            delete[] block_map.get(index);
            block_map.set(index, nullptr);
        }
    }

    shrink_map();

    return curr_elem;
}

template <class T>
void SegmentedDeque<T>::sys_push_back(const T& item) {
    T* curr_block = block_map.get(back_block);
    curr_block[back_index] = item;
    back_index++;

    if (back_index == segment_size) {
        back_block++;
        back_index = 0;

        if (back_block >= map_capacity) {
            grow_map_back();
        }

        if (block_map.get(back_block) == nullptr) {
            block_map.set(back_block, allocate_block());
        }
    }

    count++;
}

template <class T>
T* SegmentedDeque<T>::allocate_block() {
    return new T[segment_size];
}   

template <class T>
void SegmentedDeque<T>::grow_map_front() {    
    int old_capacity = map_capacity;
    map_capacity = map_capacity * 2;
    block_map.resize(map_capacity);

    for (int index = map_capacity - 1; index >= old_capacity; index--) {
        T* old_block = block_map.get(index - old_capacity);
        block_map.set(index, old_block);
    }

    for (int index = old_capacity - 1; index >= 0; index--) {
        block_map.set(index, nullptr);
    }

    front_block += old_capacity;
    back_block += old_capacity;
}

template <class T>
void SegmentedDeque<T>::grow_map_back() {
    int old_capacity = map_capacity;
    map_capacity = map_capacity * 2;
    block_map.resize(map_capacity);

    for (int index = old_capacity; index < map_capacity; index++) {
        block_map.set(index, nullptr);
    }
}

template <class T>
void SegmentedDeque<T>::shrink_map() {
    // Если карта слишком большая относительно использования — сжимаем
    int used_blocks = (count == 0) ? 1 : (back_block - front_block + 1);
    
    if (map_capacity <= 4 || used_blocks * 4 > map_capacity) return; // сжимать не нужно
    
    int new_capacity = map_capacity / 2;
    // Но новая ёмкость должна вмещать used_blocks + запасные
    if (new_capacity < used_blocks + 2) new_capacity = used_blocks + 2;
    if (new_capacity < 4) new_capacity = 4;
    
    // Сдвигаем используемые блоки в начало карты (с одним пустым блоком перед front)
    int shift = front_block - 1;  // оставляем 1 запасной блок слева
    if (shift < 0) shift = 0;
    
    if (shift > 0) {
        for (int index = 0; index + shift < map_capacity; index++) {
            block_map.set(index, block_map.get(index + shift));
        }
        for (int index = map_capacity - shift; index < map_capacity; index++) {
            block_map.set(index, nullptr);
        }
        front_block -= shift;
        back_block -= shift;
    }
    
    block_map.resize(new_capacity);
    map_capacity = new_capacity;
}

template <class T>
void SegmentedDeque<T>::resolve_index(int index, int* block, int* offset) const { 
    int total_offset = front_index + index; // Самый первый индекс + смещение по индексу

    *block = front_block + total_offset / segment_size;
    *offset = total_offset % segment_size;
}

template <class T>
void SegmentedDeque<T>::sys_append(const T& item) {
    sys_push_back(item);
}

template <class T>
SegmentedDeque<T>::SegmentedDeque() {
    block_map = DynamicArray<T*>(4);

    for (int index = 0; index < 4; index++) {
        block_map.set(index, nullptr);
    }

    T* init_block = allocate_block();
    block_map.set(1, init_block); // Ставим начальный блок по центру

    front_block = 1;
    back_block = 1;
    front_index = 4; // Ставим в первом сегменте посередине первый элемент
    back_index = 4;
    count = 0;
    map_capacity = 4;
}

template <class T>
SegmentedDeque<T>::SegmentedDeque(const T* items, int count) : SegmentedDeque() {
    if (count < 0) throw std::out_of_range("count must be >= 0");

    for (int index = 0; index < count; index++) {
        sys_push_back(items[index]);
    }
}

template <class T>
SegmentedDeque<T>::SegmentedDeque(const SegmentedDeque<T>& other) : SegmentedDeque() {
    for (int index = 0; index < other.get_count(); index++) {
        T other_elem = other.get(index);

        sys_push_back(other_elem);
    }
}

template <class T>
SegmentedDeque<T>* SegmentedDeque<T>::push_front(const T& item) {
    SegmentedDeque<T>* inst = Instance();

    inst->sys_push_front(item);

    return inst;
}

template <class T>
SegmentedDeque<T>* SegmentedDeque<T>::push_back(const T& item) {
    SegmentedDeque<T>* inst = Instance();

    inst->sys_push_back(item);

    return inst;
}

template <class T>
SegmentedDeque<T>* SegmentedDeque<T>::pop_front(T* result) {
    SegmentedDeque<T>* inst = Instance();

    *result = inst->sys_pop_front();

    return inst;
}

template <class T>
SegmentedDeque<T>* SegmentedDeque<T>::pop_back(T* result) {
    SegmentedDeque<T>* inst = Instance();

    *result = inst->sys_pop_back();

    return inst;
}

template <class T>
SegmentedDeque<T>& SegmentedDeque<T>::operator=(const SegmentedDeque<T>& other) {
    if (this == &other) return *this;

    for (int index = 0; index < this->map_capacity; index++) {
        delete[] block_map.get(index);
    }

    block_map = DynamicArray<T*>(4);

    for (int index = 0; index < 4; index++) {
        block_map.set(index, nullptr);
    }

    T* init_block = allocate_block();
    block_map.set(1, init_block); // Ставим начальный блок по центру

    front_block = 1;
    back_block = 1;
    front_index = 4; // Ставим в первом сегменте посередине первый элемент
    back_index = 4;
    count = 0;
    map_capacity = 4;

    for (int index = 0; index < other.count; index++) {
        sys_push_back(other.get(index));
    }

    return *this; // Возвращаем сам объект
}

template <class T>
const T& SegmentedDeque<T>::get_first() const {
    if (count == 0) throw std::out_of_range("Deque is empty");

    T* curr_block = block_map.get(front_block);

    return curr_block[front_index];
}

template <class T>
const T& SegmentedDeque<T>::get_last() const {
    if (count == 0) throw std::out_of_range("Deque is empty");
    
    int curr_block_index, offset;
    resolve_index(count - 1, &curr_block_index, &offset); // Получаем положение последнего элементам дека

    T* curr_block = block_map.get(curr_block_index);

    return curr_block[offset];
}

template <class T>
const T& SegmentedDeque<T>::get(int index) const {
    if (index < 0 || index >= count) throw std::out_of_range("Index out of range");

    int block_index, offset;
    resolve_index(index, &block_index, &offset);

    T* curr_block = block_map.get(block_index);

    return curr_block[offset];
}

template <class T>
Option<T> SegmentedDeque<T>::try_get_first() const {
    if (count == 0) return Option<T>::None();

    T* curr_block = block_map.get(front_block);

    return Option<T>::Some(curr_block[front_index]);
}

template <class T>
Option<T> SegmentedDeque<T>::try_get_last() const {
    if (count == 0) return Option<T>::None();

    int curr_block_index, offset;
    resolve_index(count - 1, &curr_block_index, &offset);

    T* curr_block = block_map.get(curr_block_index);

    return Option<T>::Some(curr_block[offset]);
}

template <class T>
Option<T> SegmentedDeque<T>::try_get(int index) const {
    if (index < 0 || index >= count) return Option<T>::None();

    int curr_block_index, offset;
    resolve_index(index, &curr_block_index, &offset);

    T* curr_block = block_map.get(curr_block_index);

    return Option<T>::Some(curr_block[offset]);
}

template <class T>
int SegmentedDeque<T>::get_count() const {
    return count;
}

template <class T>
Sequence<T>* SegmentedDeque<T>::append(const T& item) {
    SegmentedDeque<T>* inst = Instance();

    inst->sys_push_back(item);

    return inst;
}

template <class T>
Sequence<T>* SegmentedDeque<T>::prepend(const T& item) {
    SegmentedDeque<T>* inst = Instance();

    inst->sys_push_front(item);

    return inst;
}

template <class T>
Sequence<T>* SegmentedDeque<T>::insert_at(const T& item, int target_index) {
    SegmentedDeque<T>* inst = Instance();

    if (target_index < 0 || target_index > inst->count) throw std::out_of_range("Index out of range");

    inst->push_back(get(count - 1)); // Добавляем пустой элемент в конец через T() либо копируем последний, для своих классов нужен конструктор по умолчанию

    int curr_block_index, offset;
    resolve_index(target_index, &curr_block_index, &offset);
    
    T* curr_block = inst->block_map.get(curr_block_index);

    for (int index = count - 1; index > target_index; index--) {
        int write_block, write_offset; // Куда пишем
        resolve_index(index, &write_block, &write_offset);

        int read_block, read_offset; // Откуда пишем
        resolve_index(index - 1, &read_block, &read_offset);

        T* curr_write_block = inst->block_map.get(write_block);
        T* curr_read_block = inst->block_map.get(read_block);

        curr_write_block[write_offset] = curr_read_block[read_offset];                              
    }

    curr_block[offset] = item;

    return inst;
}

template <class T>
void SegmentedDeque<T>::sort(bool (*compare)(const T& a, const T& b)) {
    if (count <= 1) return;

    Sequence<T>* left_seq = this->get_sub_sequence(0, count / 2 - 1);
    Sequence<T>* right_seq = this->get_sub_sequence(count / 2, count - 1);

    SegmentedDeque<T>* left_deque = dynamic_cast<SegmentedDeque<T>*>(left_seq);
    SegmentedDeque<T>* right_deque = dynamic_cast<SegmentedDeque<T>*>(right_seq);

    left_deque->sort(compare);
    right_deque->sort(compare);

    SegmentedDeque<T>* merged = left_deque->merge(right_deque, compare);

    while (count > 0) sys_pop_back();

    for (int index = 0; index < merged->get_count(); index++) {
        this->sys_push_back(merged->get(index));
    }

    delete left_deque;
    delete right_deque;
    delete merged;
}

template <class T>
SegmentedDeque<T>* SegmentedDeque<T>::merge(const SegmentedDeque<T>* other, bool (*compare)(const T& a, const T& b)) {
    SegmentedDeque<T>* merge_res = CreateEmpty();

    int this_idx = 0;
    int other_idx = 0;

    while (this_idx < count && other_idx < other->get_count()) {
        T this_elem = get(this_idx);
        T other_elem = other->get(other_idx);

        if (compare == nullptr) {
            if (this_elem < other_elem) {
                merge_res->sys_push_back(this_elem);
                this_idx++;
            } else {
                merge_res->sys_push_back(other_elem);
                other_idx++;
            }
        } else {
            if (compare(this_elem, other_elem)) {
                merge_res->sys_push_back(this_elem);
                this_idx++;
            } else {
                merge_res->sys_push_back(other_elem);
                other_idx++;
            }
        }       
    }
    // Дописываем остатки, если какой-то из деков закончился раньше
    while (this_idx < count) { 
        merge_res->sys_push_back(get(this_idx));
        this_idx++;
    }

    while (other_idx < other->get_count()) {
        merge_res->sys_push_back(other->get(other_idx));
        other_idx++;
    }

    return merge_res;
}

template <class T>
int SegmentedDeque<T>::find_sub_sequence(const Sequence<T>* sub) const {
    if (sub == nullptr) throw std::invalid_argument("Cannot find nullptr sub sequence");

    int sub_count = sub->get_count();

    if (sub_count == 0) return 0;
    if (sub_count > count) return -1;

    for (int this_idx = 0; this_idx <= count - sub_count; this_idx++) {
        bool found = true;
        int offset = 0;

        EnumeratorWrapper<T> sub_iter(sub->get_enumerator());

        while (sub_iter.move_next()) {
            if (!(get(this_idx + offset) == sub_iter.get_current())) {
                found = false;
                break;
            }

            offset++;
        }

        if (found) return this_idx;
    }

    return -1;
}

template <class T>
void SegmentedDeque<T>::reset_deque() {
    for (int index = 0; index < this->map_capacity; index++) {
        delete[] block_map.get(index);
    }

    block_map = DynamicArray<T*>(4);

    for (int index = 0; index < 4; index++) {
        block_map.set(index, nullptr);
    }

    T* init_block = allocate_block();
    block_map.set(1, init_block); // Ставим начальный блок по центру

    front_block = 1;
    back_block = 1;
    front_index = 4; // Ставим в первом сегменте посередине первый элемент
    back_index = 4;
    count = 0;
    map_capacity = 4;
}

template <class T>
SegmentedDeque<T>::~SegmentedDeque() {
    for (int index = 0; index < map_capacity; index++) {
        delete[] block_map.get(index);
    }
}

