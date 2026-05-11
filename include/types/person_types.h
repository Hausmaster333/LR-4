#ifndef PERSONS_H
#define PERSONS_H

#include "types/person.h"

class Student: public Person {
    private:
        int grade_book_num;
        char* group_id;
    public:
        Student() : Person(), grade_book_num(0), group_id(nullptr) {}
        Student(Person_ID id, char* first_name, char* middle_name, char* last_name, int grade_book_num, char* group_id)
         : Person(id, first_name, middle_name, last_name), grade_book_num(grade_book_num) {
            this->group_id = new char[strlen(group_id) + 1];
            strcpy(this->group_id, group_id);
        }
        Student(const Person& other, int grade_book_num, char* group_id) : Person(other), grade_book_num(grade_book_num) {
            this->group_id = new char[strlen(group_id) + 1];
            strcpy(this->group_id, group_id);
        }
        Student(const Student& other) : Person(other), grade_book_num(other.grade_book_num) {
            this->group_id = new char[strlen(other.group_id) + 1];
            strcpy(this->group_id, other.group_id);
        }

        int get_grade_book_num() const { return grade_book_num; }
        const char* get_group_id() const { return group_id; }

        Student& operator=(const Student& other) {
            if (this == &other) return *this;

            Person::operator=(other);

            delete[] group_id;

            grade_book_num = other.grade_book_num;

            this->group_id = new char[strlen(other.group_id) + 1];
            strcpy(this->group_id, other.group_id);

            return *this;
        }

        ~Student() {
            delete[] group_id;
        }
};

class Teacher: public Person {
    private:
        int depart_num;
        char* position;
    public:
        Teacher() : Person(), depart_num(0), position(nullptr) {}
        Teacher(Person_ID id, char* first_name, char* middle_name, char* last_name, int depart_num, char* position)
         : Person(id, first_name, middle_name, last_name), depart_num(depart_num) {
            this->position = new char[strlen(position) + 1];
            strcpy(this->position, position);
        }
        Teacher(const Person& other, int depart_num, char* position) : Person(other), depart_num(depart_num) {
            this->position = new char[strlen(position) + 1];
            strcpy(this->position, position); 
        }
        Teacher(const Teacher& other) : Person(other), depart_num(other.depart_num) {
            this->position = new char[strlen(other.position) + 1];
            strcpy(this->position, other.position); 
        }

        int get_depart_num() const { return depart_num; }
        const char* get_position() const { return position; }

        Teacher& operator=(const Teacher& other) {
            if (this == &other) return *this;

            Person::operator=(other);

            delete[] position;

            depart_num = other.depart_num;

            this->position = new char[strlen(other.position) + 1];
            strcpy(this->position, other.position);

            return *this;
        }

        ~Teacher() {
            delete[] position;
        }
};

#endif