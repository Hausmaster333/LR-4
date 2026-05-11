#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "core/ienumerator.h"

template <class T>
class LinkedList {
    private:
        struct Node {
            T data;
            Node* next;
        };

        Node* head;
        Node* tail;
        int length;
    public:
        LinkedList();
        LinkedList(const T* items, int count);
        LinkedList(const LinkedList<T>& other);

        LinkedList<T>& operator=(const LinkedList<T>& other);

        const T& get_first() const;
        const T& get_last() const;
        int get_length() const;
        size_t get_node_size() const;

        LinkedList<T>* get_sub_list(int start, int end);

        void append(const T& item); // В конец
        void prepend(const T& item); // В начало
        void insert_at(const T& item, int index);

        LinkedList<T>* concat(const LinkedList<T>* other);

        ~LinkedList();

        class Enumerator : public IEnumerator<T> {
            private:
                Node* head;
                Node* current;
            public : 
                Enumerator(Node* head) : head(head), current(nullptr) {}

                bool move_next() override {
                    if (current == nullptr) {
                        current = head;
                    } else {
                        current = current->next;
                    }
                    return current != nullptr;
                }

                const T& get_current() const override {
                    return current->data;
                }

                void reset() override {
                    current = nullptr;
                }
        };

        IEnumerator<T>* get_enumerator() const {
            return new Enumerator(head);
        }
};

#include "linked_list.tpp"

#endif