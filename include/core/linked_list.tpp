#include "core/linked_list.h"
#include <stdexcept>

template <class T>
LinkedList<T>::LinkedList(): head(nullptr), tail(nullptr), length(0) { // Создать пустой список
}

template <class T>
LinkedList<T>::LinkedList(const T* items, int count): head(nullptr), tail(nullptr), length(count) { // Создать список из списка элементов и их количества
    if (length < 0) throw std::out_of_range("Length must be >= 0");
    if (length == 0) return;

    head = new Node{items[0], nullptr}; // Первый узел
    tail = head; // смотрит за концом
    
    for (int idx = 1; idx < length; idx++) {
        tail->next = new Node{items[idx], nullptr}; // next начинает указывать на следующий узел, в котором уже другой next указывает на null
        tail = tail->next; // сдвигаем tail(теперь указывает на ноду, на которую указывает next)
    }
}

template <class T>
LinkedList<T>::LinkedList(const LinkedList<T>& other): head(nullptr), tail(nullptr), length(other.length) {
    if (other.head == nullptr) return;

    head = new Node{other.head->data, nullptr};
    tail = head;

    Node* current = other.head->next;

    while (current != nullptr) {
        tail->next = new Node{current->data, nullptr};
        tail = tail->next;
        current = current->next;
    }
}

template <class T>
LinkedList<T>& LinkedList<T>::operator=(const LinkedList<T>& other) {
    if (this == &other) return *this; // Чтобы не самоприсваивать

    Node* current = head;
    while (current != nullptr) {
        Node* tmp = current;
        current = current->next;
        delete tmp;
    }

    head = nullptr;
    tail = nullptr;
    length = 0;

    EnumeratorWrapper<T> other_iter(other.get_enumerator());
    while (other_iter.move_next()) {
        T curr_elem = other_iter.get_current();
        append(curr_elem);
    }

    return *this;
}

template <class T>
const T& LinkedList<T>::get_first() const {
    if (head == nullptr) {
        throw std::out_of_range("Index out of range");
    }

    return head->data;
}

template <class T>
const T& LinkedList<T>::get_last() const {
    if (get_length() == 0) throw std::out_of_range("List is empty");

    return tail->data;
}

template <class T>
int LinkedList<T>::get_length() const {
    return length;
}

template <class T>
size_t LinkedList<T>::get_node_size() const {
    return sizeof(Node);
}

template <class T>
LinkedList<T>* LinkedList<T>::get_sub_list(int start, int end) {
    if (start < 0 || end < 0 || start >= length || end >= length || start > end) {
        throw std::out_of_range("Index out of range");
    }

    LinkedList* sub_list = new LinkedList<T>();
    sub_list->length = end - start + 1;

    Node* current = head;

    for (int idx = 0; idx < start; idx++) {
        current = current->next;
    }

    sub_list->head = new Node{current->data, nullptr};
    sub_list->tail = sub_list->head;

    for (int idx = 0; idx < end - start; idx++) {
        current = current->next;
        sub_list->tail->next = new Node{current->data, nullptr};
        sub_list->tail = sub_list->tail->next;
    }

    return sub_list;
}

template <class T>
void LinkedList<T>::append(const T& item) {
    Node* new_node = new Node{item, nullptr};

    if (head == nullptr) {
        head = new_node;
        tail = new_node;
    } else {
        tail->next = new_node;
        tail = tail->next;    
    }
    length++;

    return;
}

template <class T>
void LinkedList<T>::prepend(const T& item) {
    Node* new_node = new Node{item, nullptr};

    if (head == nullptr) {
        head = new_node;
        tail = new_node;
    } else {
        new_node->next = head;  
        head = new_node;
    }
    length++;

    return;
}

template <class T>
void LinkedList<T>::insert_at(const T& item, int index) {
    if (index < 0 || index > length) {
        throw std::out_of_range("Index out of range");
    }

    if (index == 0) {
        prepend(item);
        return;
    }

    if (index == length) {
        append(item);
        return;
    }
    
    Node* new_node = new Node{item, nullptr};
    Node* current = head;

    for (int idx = 0; idx < index - 1; idx++) {
        current = current->next;
    }

    new_node->next = current->next;
    current->next = new_node;
    length++;

    return;
}

template <class T>
LinkedList<T>* LinkedList<T>::concat(const LinkedList<T>* other) {
    LinkedList<T>* concat_list = new LinkedList<T>();

    Node* current = head;

    for (int idx = 0; idx < length; idx++) {
        concat_list->append(current->data);
        current = current->next;
    }

    current = other->head;

    for (int idx = 0; idx < other->length; idx++) {
        concat_list->append(current->data);
        current = current->next;
    }

    return concat_list;
}

template <class T>
LinkedList<T>::~LinkedList() {
    Node* current = head;

    for (int idx = 0; idx < length; idx++) {
        Node* tmp = current;
        current = current->next;
        delete tmp;
    }
}