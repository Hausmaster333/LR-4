#ifndef PERSON_H
#define PERSON_H

#include <cstring>

struct Person_ID {
    int series;
    int number;

    bool operator==(const Person_ID& other) const { return (series == other.series && number == other.number); }
    bool operator!=(const Person_ID& other) const { return !(*this == other); }
    bool operator<(const Person_ID& other) const {
        if (series == other.series) {
            return number < other.number;
        } else {
            return series < other.series;
        }
    }
    bool operator>(const Person_ID& other) const {
        if (series == other.series) {
            return number > other.number;
        } else {
            return series > other.series;
        }
    } 
};

class Person {
    private:
        Person_ID id;
        char* first_name;
        char* middle_name;
        char* last_name;
    public:
        Person() : id{0, 0}, first_name(nullptr), middle_name(nullptr), last_name(nullptr) {}
        Person(Person_ID id, char* first_name, char* middle_name, char* last_name) : id{id.series, id.number} {
            this->first_name = new char[strlen(first_name) + 1]; // + 1 делаем для \0
            strcpy(this->first_name, first_name); 

            this->middle_name = new char[strlen(middle_name) + 1];
            strcpy(this->middle_name, middle_name);

            this->last_name = new char[strlen(last_name) + 1];
            strcpy(this->last_name, last_name);
        }
        Person(const Person& other) : id{other.id.series, other.id.number} {
            this->first_name = new char[strlen(other.first_name) + 1];
            strcpy(this->first_name, other.first_name);

            this->middle_name = new char[strlen(other.middle_name) + 1];
            strcpy(this->middle_name, other.middle_name);

            this->last_name = new char[strlen(other.last_name) + 1];
            strcpy(this->last_name, other.last_name);
        }

        Person_ID get_id() const { return id; }
        
        const char* get_first_name() const { return first_name; }
        const char* get_middle_name() const { return middle_name; }
        const char* get_last_name() const { return last_name; }
        const char* get_full_name() const {  // Нужен delete[] после вызова
            char* full_name = new char[strlen(first_name) + strlen(middle_name) + strlen(last_name) + 3];

            strcpy(full_name, first_name);
            strcat(full_name, " ");
            strcat(full_name, middle_name);
            strcat(full_name, " ");
            strcat(full_name, last_name);

            return full_name;
        }

        bool operator==(const Person& other) const { return id == other.id; }
        bool operator!=(const Person& other) const { return !(*this == other); }         
        bool operator<(const Person& other) const { return id < other.id; }
        bool operator>(const Person& other) const { return id > other.id; }
        Person& operator=(const Person& other) {
            if (this == &other) return *this;

            delete[] first_name;
            delete[] middle_name;
            delete[] last_name;

            id.series = other.id.series;
            id.number = other.id.number;

            this->first_name = new char[strlen(other.first_name) + 1];
            strcpy(this->first_name, other.first_name);

            this->middle_name = new char[strlen(other.middle_name) + 1];
            strcpy(this->middle_name, other.middle_name);

            this->last_name = new char[strlen(other.last_name) + 1];
            strcpy(this->last_name, other.last_name);

            return *this;
        }

        ~Person() {
            delete[] first_name;
            delete[] middle_name;
            delete[] last_name;
        }
};

#endif