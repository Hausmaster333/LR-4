#include "menu.h"
#include "core/sequence.h"
#include "deque/segment_deque.h"
#include "types/complex.h"
#include "types/person_types.h"
#include "hanoi/hanoi.h"
#include <iostream>
#include <string>
#include <chrono>

const int MAX_DEQUES = 10;

// ====== Ввод

void read_int(int& value) {
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        try {
            size_t pos;
            value = std::stoi(line, &pos);
            if (pos == line.size()) return;
        } catch (...) {}
        std::cout << "Incorrect input! Try again: ";
    }
}

void read_double(double& value) {
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        try {
            size_t pos;
            value = std::stod(line, &pos);
            if (pos == line.size()) return;
        } catch (...) {}
        std::cout << "Incorrect input! Try again: ";
    }
}

void read_string(char* buf, int max_len) {
    std::string line;
    std::getline(std::cin, line);
    strncpy(buf, line.c_str(), max_len - 1);
    buf[max_len - 1] = '\0';
}

// ====== Тип элементов

enum ElementType { 
    TYPE_INT = 1, 
    TYPE_DOUBLE, 
    TYPE_STRING, 
    TYPE_COMPLEX 
};

ElementType current_type = TYPE_INT;

// ====== Хранение деков для каждого типа

MutableSegmentedDeque<int>* int_deques[MAX_DEQUES];
int int_deque_count = 0;

MutableSegmentedDeque<double>* double_deques[MAX_DEQUES];
int double_deque_count = 0;

MutableSegmentedDeque<std::string>* string_deques[MAX_DEQUES];
int string_deque_count = 0;

MutableSegmentedDeque<Complex>* complex_deques[MAX_DEQUES];
int complex_deque_count = 0;

// ====== Вспомогательные функции

int int_square(const int& x) { return x * x; }
int int_double_val(const int& x) { return x * 2; }
int int_sum(const int& a, const int& b) { return a + b; }
bool int_is_positive(const int& x) { return x > 0; }
bool int_is_even(const int& x) { return x % 2 == 0; }

double dbl_square(const double& x) { return x * x; }
double dbl_sum(const double& a, const double& b) { return a + b; }
bool dbl_is_positive(const double& x) { return x > 0; }

Complex cmplx_square(const Complex& c) { return Complex(c.get_real() * c.get_real() - c.get_imag() * c.get_imag(), 2 * c.get_real() * c.get_imag()); }
Complex cmplx_sum(const Complex& a, const Complex& b) { return Complex(a.get_real() + b.get_real(), a.get_imag() + b.get_imag()); }
bool cmplx_modulus_gt1(const Complex& c) { return c.modulus() > 1.0; }

// ====== Печать деков

void print_int_deque(MutableSegmentedDeque<int>* deq) {
    std::cout << "[";
    for (int idx = 0; idx < deq->get_count(); idx++) {
        if (idx > 0) std::cout << ", ";
        std::cout << deq->get(idx);
    }
    std::cout << "]" << std::endl;
}

void print_double_deque(MutableSegmentedDeque<double>* deq) {
    std::cout << "[";
    for (int idx = 0; idx < deq->get_count(); idx++) {
        if (idx > 0) std::cout << ", ";
        std::cout << deq->get(idx);
    }
    std::cout << "]" << std::endl;
}

void print_string_deque(MutableSegmentedDeque<std::string>* deq) {
    std::cout << "[";
    for (int idx = 0; idx < deq->get_count(); idx++) {
        if (idx > 0) std::cout << ", ";
        std::cout << "\"" << deq->get(idx) << "\"";
    }
    std::cout << "]" << std::endl;
}

void print_complex_deque(MutableSegmentedDeque<Complex>* deq) {
    std::cout << "[";
    for (int idx = 0; idx < deq->get_count(); idx++) {
        if (idx > 0) std::cout << ", ";
        std::cout << "(" << deq->get(idx).get_real() << " + " << deq->get(idx).get_imag() << "i)";
    }
    std::cout << "]" << std::endl;
}

void print_ring_deque(MutableSegmentedDeque<Ring>* deq) {
    std::cout << "[";
    for (int idx = 0; idx < deq->get_count(); idx++) {
        if (idx > 0) std::cout << ", ";
        std::cout << "{size=" << deq->get(idx).get_size() << ", shape=" << deq->get(idx).get_shape() << ", color=" << deq->get(idx).get_color() << "}";
    }
    std::cout << "]" << std::endl;
}

// ====== Выбор типа

void menu_select_type() {
    std::cout << "\n=== Select element type ===" << std::endl;
    std::cout << "1. int" << std::endl;
    std::cout << "2. double" << std::endl;
    std::cout << "3. string" << std::endl;
    std::cout << "4. Complex" << std::endl;
    std::cout << "Choice: ";

    int choice;
    read_int(choice);

    if (choice >= 1 && choice <= 4) {
        current_type = static_cast<ElementType>(choice);
        const char* names[] = {"", "int", "double", "string", "Complex"};
        std::cout << "Type set to: " << names[current_type] << std::endl;
    } else {
        std::cout << "Invalid choice" << std::endl;
    }
}

// ====== Создание дека

void menu_create_deque() {
    std::cout << "\n=== Create Deque ===" << std::endl;
    std::cout << "1. Empty" << std::endl;
    std::cout << "2. From elements" << std::endl;
    std::cout << "Choice: ";

    int choice;
    read_int(choice);

    if (current_type == TYPE_INT) {
        if (int_deque_count >= MAX_DEQUES) { 
            std::cout << "Max deques reached" << std::endl; 
            return; 
        }

        MutableSegmentedDeque<int>* deq = new MutableSegmentedDeque<int>();

        if (choice == 2) {
            std::cout << "Number of elements: ";
            int n;
            read_int(n);
            for (int idx = 0; idx < n; idx++) {
                std::cout << "Element " << idx << ": ";
                int val;
                read_int(val);

                deq->push_back(val);
            }
        }
        int_deques[int_deque_count++] = deq;
        std::cout << "Deque<int> created (index: " << int_deque_count - 1 << ")" << std::endl;

    } else if (current_type == TYPE_DOUBLE) {
        if (double_deque_count >= MAX_DEQUES) { 
            std::cout << "Max deques reached" << std::endl; 
            return; 
        }

        MutableSegmentedDeque<double>* deq = new MutableSegmentedDeque<double>();

        if (choice == 2) {
            std::cout << "Number of elements: ";
            int n;
            read_int(n);
            for (int idx = 0; idx < n; idx++) {
                std::cout << "Element " << idx << ": ";
                double val;
                read_double(val);

                deq->push_back(val);
            }
        }
        double_deques[double_deque_count++] = deq;
        std::cout << "Deque<double> created (index: " << double_deque_count - 1 << ")" << std::endl;

    } else if (current_type == TYPE_STRING) {
        if (string_deque_count >= MAX_DEQUES) { 
            std::cout << "Max deques reached" << std::endl; 
            return; 
        }

        MutableSegmentedDeque<std::string>* deq = new MutableSegmentedDeque<std::string>();

        if (choice == 2) {
            std::cout << "Number of elements: ";
            int n;
            read_int(n);
            for (int idx = 0; idx < n; idx++) {
                std::cout << "Element " << idx << ": ";
                std::string val;
                std::getline(std::cin, val);

                deq->push_back(val);
            }
        }
        string_deques[string_deque_count++] = deq;
        std::cout << "Deque<string> created (index: " << string_deque_count - 1 << ")" << std::endl;

    } else if (current_type == TYPE_COMPLEX) {
        if (complex_deque_count >= MAX_DEQUES) { 
            std::cout << "Max deques reached" << std::endl; 
            return; 
        }

        MutableSegmentedDeque<Complex>* deq = new MutableSegmentedDeque<Complex>();

        if (choice == 2) {
            std::cout << "Number of elements: ";
            int n;
            read_int(n);
            for (int idx = 0; idx < n; idx++) {
                std::cout << "Element " << idx << " (real): ";
                double re;
                read_double(re);

                std::cout << "Element " << idx << " (imag): ";
                double im;
                read_double(im);

                deq->push_back(Complex(re, im));
            }
        }
        complex_deques[complex_deque_count++] = deq;
        std::cout << "Deque<Complex> created (index: " << complex_deque_count - 1 << ")" << std::endl;
    }
}

// ====== Выбор дека

int select_int_deque(const char* msg) {
    if (int_deque_count == 0) { 
        std::cout << "No int deques" << std::endl; 
        return -1; 
    }

    std::cout << msg << std::endl;

    for (int idx = 0; idx < int_deque_count; idx++) { 
        std::cout << idx << ": "; 
        print_int_deque(int_deques[idx]); 
    }

    std::cout << "Index: ";
    int idx;
    read_int(idx);

    if (idx < 0 || idx >= int_deque_count) { 
        std::cout << "Invalid index" << std::endl; 
        return -1; 
    }
    return idx;
}

int select_double_deque(const char* msg) {
    if (double_deque_count == 0) { 
        std::cout << "No double deques" << std::endl; 
        return -1; 
    }

    std::cout << msg << std::endl;

    for (int idx = 0; idx < double_deque_count; idx++) { 
        std::cout << idx << ": "; 
        print_double_deque(double_deques[idx]); 
    }

    std::cout << "Index: ";
    int idx;
    read_int(idx);

    if (idx < 0 || idx >= double_deque_count) { 
        std::cout << "Invalid index" << std::endl; 
        return -1; 
    }
    return idx;
}

int select_string_deque(const char* msg) {
    if (string_deque_count == 0) { 
        std::cout << "No string deques" << std::endl; 
        return -1; 
    }

    std::cout << msg << std::endl;

    for (int idx = 0; idx < string_deque_count; idx++) { 
        std::cout << idx << ": "; 
        print_string_deque(string_deques[idx]); 
    }

    std::cout << "Index: ";
    int idx;
    read_int(idx);

    if (idx < 0 || idx >= string_deque_count) { 
        std::cout << "Invalid index" << std::endl; 
        return -1; 
    }
    return idx;
}

int select_complex_deque(const char* msg) {
    if (complex_deque_count == 0) { 
        std::cout << "No Complex deques" << std::endl; 
        return -1; 
    }

    std::cout << msg << std::endl;

    for (int idx = 0; idx < complex_deque_count; idx++) { 
        std::cout << idx << ": "; 
        print_complex_deque(complex_deques[idx]); 
    }

    std::cout << "Index: ";
    int idx;
    read_int(idx);

    if (idx < 0 || idx >= complex_deque_count) { 
        std::cout << "Invalid index" << std::endl; 
        return -1; 
    }
    return idx;
}

// ====== Push

void menu_push() {
    std::cout << "\n=== Push ===" << std::endl;
    std::cout << "1. Push back" << std::endl;
    std::cout << "2. Push front" << std::endl;
    std::cout << "Choice: ";
    int side;
    read_int(side);

    if (side != 1 && side != 2) { 
        std::cout << "Invalid choice" << std::endl; 
        return; 
    }

    if (current_type == TYPE_INT) {
        int idx = select_int_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "Value: ";
        int val;
        read_int(val);

        if (side == 1) {
            int_deques[idx]->push_back(val);
        } else {
            int_deques[idx]->push_front(val);
        }

    } else if (current_type == TYPE_DOUBLE) {
        int idx = select_double_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "Value: "; 
        double val; 
        read_double(val);

        if (side == 1) {
            double_deques[idx]->push_back(val);
        } else {
            double_deques[idx]->push_front(val);
        }

    } else if (current_type == TYPE_STRING) {
        int idx = select_string_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "Value: "; 
        std::string val; 
        std::getline(std::cin, val);

        if (side == 1) {
            string_deques[idx]->push_back(val);
        } else {
            string_deques[idx]->push_front(val);
        }

    } else if (current_type == TYPE_COMPLEX) {
        int idx = select_complex_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "Real: "; 
        double re; 
        read_double(re);

        std::cout << "Imag: "; 
        double im; 
        read_double(im);

        if (side == 1) {
            complex_deques[idx]->push_back(Complex(re, im));
        } else {
            complex_deques[idx]->push_front(Complex(re, im));
        }
    }
    std::cout << "Done" << std::endl;
}

// ====== Pop

void menu_pop() {
    std::cout << "\n=== Pop ===" << std::endl;
    std::cout << "1. Pop back" << std::endl;
    std::cout << "2. Pop front" << std::endl;
    std::cout << "Choice: ";
    int side;
    read_int(side);

    if (side != 1 && side != 2) {
        std::cout << "Invalid choice" << std::endl;
        return;
    }

    try {
        if (current_type == TYPE_INT) {
            int idx = select_int_deque("Select deque:");
            if (idx == -1) return;

            int val;
            if (side == 1) {
                int_deques[idx]->pop_back(&val);
            }
            else {
                int_deques[idx]->pop_front(&val);
            }

            std::cout << "Popped: " << val << std::endl;

        } else if (current_type == TYPE_DOUBLE) {
            int idx = select_double_deque("Select deque:");
            if (idx == -1) return;

            double val;
            if (side == 1) {
                double_deques[idx]->pop_back(&val);
            }
            else {
                double_deques[idx]->pop_front(&val);
            }

            std::cout << "Popped: " << val << std::endl;

        } else if (current_type == TYPE_STRING) {
            int idx = select_string_deque("Select deque:");
            if (idx == -1) return;

            std::string val;
            if (side == 1) {
                string_deques[idx]->pop_back(&val);
            }
            else {
                string_deques[idx]->pop_front(&val);
            }

            std::cout << "Popped: \"" << val << "\"" << std::endl;

        } else if (current_type == TYPE_COMPLEX) {
            int idx = select_complex_deque("Select deque:");
            if (idx == -1) return;

            Complex val;
            if (side == 1) {
                complex_deques[idx]->pop_back(&val);
            }
            else {
                complex_deques[idx]->pop_front(&val);
            }

            std::cout << "Popped: (" << val.get_real() << " + " << val.get_imag() << "i)" << std::endl;
        }
    } catch (const std::out_of_range& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// ====== Вывести все последовательности

void menu_print_deques() {
    std::cout << "\n=== All Deques ===" << std::endl;

    if (int_deque_count > 0) {
        std::cout << "--- int ---" << std::endl;
        for (int idx = 0; idx < int_deque_count; idx++) { 
            std::cout << "[" << idx << "]: "; 
            print_int_deque(int_deques[idx]); 
        }
    }
    if (double_deque_count > 0) {
        std::cout << "--- double ---" << std::endl;
        for (int idx = 0; idx < double_deque_count; idx++) { 
            std::cout << "[" << idx << "]: "; 
            print_double_deque(double_deques[idx]); 
        }
    }
    if (string_deque_count > 0) {
        std::cout << "--- string ---" << std::endl;
        for (int idx = 0; idx < string_deque_count; idx++) { 
            std::cout << "[" << idx << "]: "; 
            print_string_deque(string_deques[idx]); 
        }
    }
    if (complex_deque_count > 0) {
        std::cout << "--- Complex ---" << std::endl;
        for (int idx = 0; idx < complex_deque_count; idx++) { 
            std::cout << "[" << idx << "]: "; 
            print_complex_deque(complex_deques[idx]); 
        }
    }
    if (int_deque_count == 0 && double_deque_count == 0 && string_deque_count == 0 && complex_deque_count == 0) {
        std::cout << "No deques created" << std::endl;
    }
}

// ====== Получить элемент

void menu_get_element() {
    std::cout << "Index: ";
    int pos;
    read_int(pos);

    try {
        if (current_type == TYPE_INT) {
            int idx = select_int_deque("Select deque:");
            if (idx == -1) return;

            std::cout << "Element: " << int_deques[idx]->get(pos) << std::endl;
        } else if (current_type == TYPE_DOUBLE) {
            int idx = select_double_deque("Select deque:");
            if (idx == -1) return;

            std::cout << "Element: " << double_deques[idx]->get(pos) << std::endl;
        } else if (current_type == TYPE_STRING) {
            int idx = select_string_deque("Select deque:");
            if (idx == -1) return;

            std::cout << "Element: \"" << string_deques[idx]->get(pos) << "\"" << std::endl;
        } else if (current_type == TYPE_COMPLEX) {
            int idx = select_complex_deque("Select deque:");
            if (idx == -1) return;

            Complex c = complex_deques[idx]->get(pos);
            std::cout << "Element: (" << c.get_real() << " + " << c.get_imag() << "i)" << std::endl;
        }
    } catch (const std::out_of_range& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// ====== get_sub_sequence

void menu_sub_sequence() {
    std::cout << "Start index: ";
    int start;
    read_int(start);

    std::cout << "End index: ";
    int end;
    read_int(end);

    try {
        if (current_type == TYPE_INT) {
            int idx = select_int_deque("Select deque:");
            if (idx == -1) return;

            Sequence<int>* sub = int_deques[idx]->get_sub_sequence(start, end);
            MutableSegmentedDeque<int>* sub_deque = dynamic_cast<MutableSegmentedDeque<int>*>(sub);
            std::cout << "Subsequence: "; 

            print_int_deque(sub_deque);

            if (int_deque_count < MAX_DEQUES) {
                int_deques[int_deque_count++] = sub_deque;
                std::cout << "Saved as index " << int_deque_count - 1 << std::endl;
            } else {
                delete sub;
            }

        } else if (current_type == TYPE_DOUBLE) {
            int idx = select_double_deque("Select deque:");
            if (idx == -1) return;

            Sequence<double>* sub = double_deques[idx]->get_sub_sequence(start, end);
            MutableSegmentedDeque<double>* sub_deque = dynamic_cast<MutableSegmentedDeque<double>*>(sub);
            std::cout << "Subsequence: "; 
            
            print_double_deque(sub_deque);

            if (double_deque_count < MAX_DEQUES) {
                double_deques[double_deque_count++] = sub_deque;
                std::cout << "Saved as index " << double_deque_count - 1 << std::endl;
            } else {
                delete sub;
            }

        } else if (current_type == TYPE_STRING) {
            int idx = select_string_deque("Select deque:");
            if (idx == -1) return;

            Sequence<std::string>* sub = string_deques[idx]->get_sub_sequence(start, end);
            MutableSegmentedDeque<std::string>* sub_deque = dynamic_cast<MutableSegmentedDeque<std::string>*>(sub);
            std::cout << "Subsequence: ";

            print_string_deque(sub_deque);

            if (string_deque_count < MAX_DEQUES) {
                string_deques[string_deque_count++] = sub_deque;
                std::cout << "Saved as index " << string_deque_count - 1 << std::endl;
            } else {
                delete sub;
            }

        } else if (current_type == TYPE_COMPLEX) {
            int idx = select_complex_deque("Select deque:");
            if (idx == -1) return;

            Sequence<Complex>* sub = complex_deques[idx]->get_sub_sequence(start, end);
            MutableSegmentedDeque<Complex>* sub_deque = dynamic_cast<MutableSegmentedDeque<Complex>*>(sub);
            std::cout << "Subsequence: "; 
            
            print_complex_deque(sub_deque);

            if (complex_deque_count < MAX_DEQUES) {
                complex_deques[complex_deque_count++] = sub_deque;
                std::cout << "Saved as index " << complex_deque_count - 1 << std::endl;
            } else {
                delete sub;
            }
        }
    } catch (const std::out_of_range& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// ====== Concat

void menu_concat() {
    if (current_type == TYPE_INT) {
        int i1 = select_int_deque("Select first deque:");
        if (i1 == -1) return;

        int i2 = select_int_deque("Select second deque:");
        if (i2 == -1) return;

        Sequence<int>* res = int_deques[i1]->concat(int_deques[i2]);
        MutableSegmentedDeque<int>* res_d = dynamic_cast<MutableSegmentedDeque<int>*>(res);

        std::cout << "Result: "; 
        print_int_deque(res_d);

        if (int_deque_count < MAX_DEQUES) { 
            int_deques[int_deque_count++] = res_d; 
            std::cout << "Saved as index " << int_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_DOUBLE) {
        int i1 = select_double_deque("Select first deque:");
        if (i1 == -1) return;

        int i2 = select_double_deque("Select second deque:");
        if (i2 == -1) return;

        Sequence<double>* res = double_deques[i1]->concat(double_deques[i2]);
        MutableSegmentedDeque<double>* res_d = dynamic_cast<MutableSegmentedDeque<double>*>(res);

        std::cout << "Result: "; 
        print_double_deque(res_d);

        if (double_deque_count < MAX_DEQUES) { 
            double_deques[double_deque_count++] = res_d; 
            std::cout << "Saved as index " << double_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_STRING) {
        int i1 = select_string_deque("Select first deque:");
        if (i1 == -1) return;

        int i2 = select_string_deque("Select second deque:");
        if (i2 == -1) return;

        Sequence<std::string>* res = string_deques[i1]->concat(string_deques[i2]);
        MutableSegmentedDeque<std::string>* res_d = dynamic_cast<MutableSegmentedDeque<std::string>*>(res);

        std::cout << "Result: "; 
        print_string_deque(res_d);

        if (string_deque_count < MAX_DEQUES) { 
            string_deques[string_deque_count++] = res_d; 
            std::cout << "Saved as index " << string_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_COMPLEX) {
        int i1 = select_complex_deque("Select first deque:");
        if (i1 == -1) return;

        int i2 = select_complex_deque("Select second deque:");
        if (i2 == -1) return;

        Sequence<Complex>* res = complex_deques[i1]->concat(complex_deques[i2]);
        MutableSegmentedDeque<Complex>* res_d = dynamic_cast<MutableSegmentedDeque<Complex>*>(res);

        std::cout << "Result: "; 
        print_complex_deque(res_d);

        if (complex_deque_count < MAX_DEQUES) { 
            complex_deques[complex_deque_count++] = res_d; 
            std::cout << "Saved as index " << complex_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }
    }
}

// ====== Map

void menu_map() {
    if (current_type == TYPE_INT) {
        int idx = select_int_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "1. square (x^2)" << std::endl;
        std::cout << "2. double (x*2)" << std::endl;
        std::cout << "Choice: "; 
        int choice; 
        read_int(choice);

        Sequence<int>* res = nullptr;

        if (choice == 1) {
            res = int_deques[idx]->map(int_square);
        } else if (choice == 2) {
            res = int_deques[idx]->map(int_double_val);
        } else { 
            std::cout << "Invalid" << std::endl;
            return;
        }

        MutableSegmentedDeque<int>* res_d = dynamic_cast<MutableSegmentedDeque<int>*>(res);

        std::cout << "Result: "; 
        print_int_deque(res_d);
        if (int_deque_count < MAX_DEQUES) { 
            int_deques[int_deque_count++] = res_d; 
            std::cout << "Saved as index " << int_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_DOUBLE) {
        int idx = select_double_deque("Select deque:");
        if (idx == -1) return;

        Sequence<double>* res = double_deques[idx]->map(dbl_square);
        MutableSegmentedDeque<double>* res_d = dynamic_cast<MutableSegmentedDeque<double>*>(res);

        std::cout << "Result map(square): "; 
        print_double_deque(res_d);
        if (double_deque_count < MAX_DEQUES) { 
            double_deques[double_deque_count++] = res_d; 
            std::cout << "Saved as index " << double_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_COMPLEX) {
        int idx = select_complex_deque("Select deque:");
        if (idx == -1) return;

        Sequence<Complex>* res = complex_deques[idx]->map(cmplx_square);
        MutableSegmentedDeque<Complex>* res_d = dynamic_cast<MutableSegmentedDeque<Complex>*>(res);

        std::cout << "Result map(square): "; 
        print_complex_deque(res_d);
        if (complex_deque_count < MAX_DEQUES) { 
            complex_deques[complex_deque_count++] = res_d; 
            std::cout << "Saved as index " << complex_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else {
        std::cout << "Map not available for this type" << std::endl;
    }
}

// ====== Where

void menu_where() {
    if (current_type == TYPE_INT) {
        int idx = select_int_deque("Select deque:");
        if (idx == -1) return;

        std::cout << "1. is_positive (x > 0)" << std::endl;
        std::cout << "2. is_even (x % 2 == 0)" << std::endl;
        std::cout << "Choice: ";
        int choice;
        read_int(choice);

        Sequence<int>* res = nullptr;

        if (choice == 1) {
            res = int_deques[idx]->where(int_is_positive);
        } else if (choice == 2) {
            res = int_deques[idx]->where(int_is_even);
        } else {
            std::cout << "Invalid" << std::endl;
            return;
        }

        MutableSegmentedDeque<int>* res_d = dynamic_cast<MutableSegmentedDeque<int>*>(res);
        std::cout << "Result: "; 
        print_int_deque(res_d);

        if (int_deque_count < MAX_DEQUES) { 
            int_deques[int_deque_count++] = res_d; 
            std::cout << "Saved as index " << int_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_DOUBLE) {
        int idx = select_double_deque("Select deque:");
        if (idx == -1) return;

        Sequence<double>* res = double_deques[idx]->where(dbl_is_positive);
        MutableSegmentedDeque<double>* res_d = dynamic_cast<MutableSegmentedDeque<double>*>(res);

        std::cout << "Result where(positive): "; 
        print_double_deque(res_d);

        if (double_deque_count < MAX_DEQUES) { 
            double_deques[double_deque_count++] = res_d; 
            std::cout << "Saved as index " << double_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_COMPLEX) {
        int idx = select_complex_deque("Select deque:");
        if (idx == -1) return;

        Sequence<Complex>* res = complex_deques[idx]->where(cmplx_modulus_gt1);
        MutableSegmentedDeque<Complex>* res_d = dynamic_cast<MutableSegmentedDeque<Complex>*>(res);

        std::cout << "Result where(modulus > 1): "; 
        print_complex_deque(res_d);

        if (complex_deque_count < MAX_DEQUES) { 
            complex_deques[complex_deque_count++] = res_d; 
            std::cout << "Saved as index " << complex_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else {
        std::cout << "Where not available for this type" << std::endl;
    }
}

// ====== Reduce

void menu_reduce() {
    if (current_type == TYPE_INT) {
        int idx = select_int_deque("Select deque:");
        if (idx == -1) return;
        int result = int_deques[idx]->reduce(int_sum, 0);
        std::cout << "Result reduce(sum, 0): " << result << std::endl;

    } else if (current_type == TYPE_DOUBLE) {
        int idx = select_double_deque("Select deque:");
        if (idx == -1) return;

        double result = double_deques[idx]->reduce(dbl_sum, 0.0);
        std::cout << "Result reduce(sum, 0): " << result << std::endl;

    } else if (current_type == TYPE_COMPLEX) {
        int idx = select_complex_deque("Select deque:");
        if (idx == -1) return;

        Complex result = complex_deques[idx]->reduce(cmplx_sum, Complex(0, 0));
        std::cout << "Result reduce(sum): (" << result.get_real() << " + " << result.get_imag() << "i)" << std::endl;

    } else {
        std::cout << "Reduce not available for this type" << std::endl;
    }
}

// ====== Sort

void menu_sort() {
    if (current_type == TYPE_INT) {
        int idx = select_int_deque("Select deque:");
        if (idx == -1) return;

        int_deques[idx]->sort();
        std::cout << "Sorted: ";
        print_int_deque(int_deques[idx]);

    } else if (current_type == TYPE_DOUBLE) {
        int idx = select_double_deque("Select deque:");
        if (idx == -1) return;

        double_deques[idx]->sort();
        std::cout << "Sorted: ";
        print_double_deque(double_deques[idx]);

    } else if (current_type == TYPE_STRING) {
        int idx = select_string_deque("Select deque:");
        if (idx == -1) return;

        string_deques[idx]->sort();
        std::cout << "Sorted: ";
        print_string_deque(string_deques[idx]);

    } else if (current_type == TYPE_COMPLEX) {
        int idx = select_complex_deque("Select deque:");
        if (idx == -1) return;

        complex_deques[idx]->sort();
        std::cout << "Sorted (by modulus): ";
        print_complex_deque(complex_deques[idx]);
    }
}

// ====== Merge

void menu_merge() {
    if (current_type == TYPE_INT) {
        int i1 = select_int_deque("Select first sorted deque:");
        if (i1 == -1) return;

        int i2 = select_int_deque("Select second sorted deque:");
        if (i2 == -1) return;

        SegmentedDeque<int>* res = int_deques[i1]->merge(int_deques[i2]);
        MutableSegmentedDeque<int>* res_d = dynamic_cast<MutableSegmentedDeque<int>*>(res);

        std::cout << "Merged: "; 
        print_int_deque(res_d);

        if (int_deque_count < MAX_DEQUES) { 
            int_deques[int_deque_count++] = res_d; 
            std::cout << "Saved as index " << int_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else if (current_type == TYPE_DOUBLE) {
        int i1 = select_double_deque("Select first sorted deque:");
        if (i1 == -1) return;

        int i2 = select_double_deque("Select second sorted deque:");
        if (i2 == -1) return;

        SegmentedDeque<double>* res = double_deques[i1]->merge(double_deques[i2]);
        MutableSegmentedDeque<double>* res_d = dynamic_cast<MutableSegmentedDeque<double>*>(res);

        std::cout << "Merged: "; 
        print_double_deque(res_d);

        if (double_deque_count < MAX_DEQUES) { 
            double_deques[double_deque_count++] = res_d; 
            std::cout << "Saved as index " << double_deque_count - 1 << std::endl; 
        } else {
            delete res;
        }

    } else {
        std::cout << "Merge available for int and double" << std::endl;
    }
}

// ====== Find subsequence

void menu_find_sub() {
    if (current_type == TYPE_INT) {
        int i1 = select_int_deque("Select deque to search in:");
        if (i1 == -1) return;

        int i2 = select_int_deque("Select subsequence deque:");
        if (i2 == -1) return;

        int pos = int_deques[i1]->find_sub_sequence(int_deques[i2]);
        if (pos >= 0) std::cout << "Found at index: " << pos << std::endl;
        else {
            std::cout << "Not found" << std::endl;
        }

    } else if (current_type == TYPE_DOUBLE) {
        int i1 = select_double_deque("Select deque to search in:");
        if (i1 == -1) return;

        int i2 = select_double_deque("Select subsequence deque:");
        if (i2 == -1) return;

        int pos = double_deques[i1]->find_sub_sequence(double_deques[i2]);
        if (pos >= 0) {
            std::cout << "Found at index: " << pos << std::endl;
        }
        else {
            std::cout << "Not found" << std::endl;
        }

    } else if (current_type == TYPE_STRING) {
        int i1 = select_string_deque("Select deque to search in:");
        if (i1 == -1) return;

        int i2 = select_string_deque("Select subsequence deque:");
        if (i2 == -1) return;

        int pos = string_deques[i1]->find_sub_sequence(string_deques[i2]);
        if (pos >= 0) std::cout << "Found at index: " << pos << std::endl;
        else std::cout << "Not found" << std::endl;

    } else if (current_type == TYPE_COMPLEX) {
        int i1 = select_complex_deque("Select deque to search in:");
        if (i1 == -1) return;

        int i2 = select_complex_deque("Select subsequence deque:");
        if (i2 == -1) return;

        int pos = complex_deques[i1]->find_sub_sequence(complex_deques[i2]);
        if (pos >= 0) {
            std::cout << "Found at index: " << pos << std::endl;
        } else {
            std::cout << "Not found" << std::endl;
        }
    }
}

// ====== Hanoi

void menu_hanoi() {
    std::cout << "\n=== Hanoi Tower ===" << std::endl;
    std::cout << "Number of rings: ";
    int n; read_int(n);
    if (n <= 0) {
        std::cout << "Must be > 0" << std::endl;
        return;
    }

    MutableSegmentedDeque<Ring> rings;
    for (int idx = 0; idx < n; idx++) {
        std::cout << "Ring " << idx << " size: ";
        int size; read_int(size);
        char shape[64], color[64];
        std::cout << "Ring " << idx << " shape: ";
        read_string(shape, 64);
        std::cout << "Ring " << idx << " color: ";
        read_string(color, 64);

        rings.push_back(Ring(size, shape, color));
    }

    std::cout << "Start stick (0-2): ";
    int start; 
    read_int(start);

    std::cout << "Target stick (0-2): ";
    int target; 
    read_int(target);

    try {
        hanoi(rings, start, target);
        std::cout << "Hanoi complete!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// ====== Performance test

void menu_performance() {
    std::cout << "\n=== Performance Test ===" << std::endl;
    std::cout << "Number of elements: ";
    int n; read_int(n);

    MutableSegmentedDeque<int> deque;

    // push_back
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int idx = 0; idx < n; idx++) {
        deque.push_back(idx);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto push_back_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "push_back x" << n << ": " << push_back_ms << " us" << std::endl;

    // get (random access)
    t1 = std::chrono::high_resolution_clock::now();
    for (int idx = 0; idx < n; idx++) {
        deque.get(idx);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto get_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "get x" << n << ": " << get_ms << " us" << std::endl;

    // push_front
    MutableSegmentedDeque<int> deque2;
    t1 = std::chrono::high_resolution_clock::now();
    for (int idx = 0; idx < n; idx++) {
        deque2.push_front(idx);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto push_front_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "push_front x" << n << ": " << push_front_ms << " us" << std::endl;

    // sort
    t1 = std::chrono::high_resolution_clock::now();
    deque2.sort();
    t2 = std::chrono::high_resolution_clock::now();
    auto sort_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "sort x" << n << ": " << sort_ms << " us" << std::endl;

    // pop_back
    t1 = std::chrono::high_resolution_clock::now();
    int val;
    for (int idx = 0; idx < n; idx++) {
        deque.pop_back(&val);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto pop_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "pop_back x" << n << ": " << pop_ms << " us" << std::endl;

    // Сравнение с ArraySequence
    std::cout << "\n--- Comparison: push_front ---" << std::endl;
    MutableArraySequence<int> arr_seq;

    t1 = std::chrono::high_resolution_clock::now();
    for (int idx = 0; idx < n; idx++) {
        arr_seq.prepend(idx);
    }
    t2 = std::chrono::high_resolution_clock::now();
    auto arr_prepend_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    std::cout << "ArraySequence prepend x" << n << ": " << arr_prepend_ms << " us" << std::endl;
    std::cout << "SegmentedDeque push_front x" << n << ": " << push_front_ms << " us" << std::endl;

    if (arr_prepend_ms > 0) {
        std::cout << "Deque is ~" << arr_prepend_ms / (push_front_ms > 0 ? push_front_ms : 1) << "x faster for push_front" << std::endl;
    }
}

void menu_run_tests() {
    std::cout << "\nRunning tests..." << std::endl;
    int ret = system("deq_tests");
    if (ret != 0) {
        std::cout << "Error running tests" << std::endl;
    }
}

void run_menu() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n==== Segmented Deque Menu ====" << std::endl;
        std::cout << "Current type: ";
        const char* names[] = {"", "int", "double", "string", "Complex"};
        std::cout << names[current_type] << std::endl;
        std::cout << "1.  Select type" << std::endl;
        std::cout << "2.  Create deque" << std::endl;
        std::cout << "3.  Push (back/front)" << std::endl;
        std::cout << "4.  Pop (back/front)" << std::endl;
        std::cout << "5.  Print all deques" << std::endl;
        std::cout << "6.  Get element" << std::endl;
        std::cout << "7.  Subsequence" << std::endl;
        std::cout << "8.  Concat" << std::endl;
        std::cout << "9.  Map" << std::endl;
        std::cout << "10. Where" << std::endl;
        std::cout << "11. Reduce" << std::endl;
        std::cout << "12. Sort" << std::endl;
        std::cout << "13. Merge" << std::endl;
        std::cout << "14. Find subsequence" << std::endl;
        std::cout << "15. Hanoi Tower" << std::endl;
        std::cout << "16. Performance test" << std::endl;
        std::cout << "17. Run tests" << std::endl;
        std::cout << "0.  Exit" << std::endl;
        std::cout << "Choice: ";
        read_int(choice);

        switch (choice) {
            case 1: 
                menu_select_type(); 
                break;
            case 2: 
                menu_create_deque(); 
                break;
            case 3: 
                menu_push(); 
                break;
            case 4: 
                menu_pop(); 
                break;
            case 5: 
                menu_print_deques(); 
                break;
            case 6: 
                menu_get_element(); 
                break;
            case 7: 
                menu_sub_sequence(); 
                break;
            case 8: 
                menu_concat(); 
                break;
            case 9: 
                menu_map(); 
                break;
            case 10: 
                menu_where(); 
                break;
            case 11: 
                menu_reduce(); 
                break;
            case 12: 
                menu_sort(); 
                break;
            case 13: 
                menu_merge(); 
                break;
            case 14: 
                menu_find_sub(); 
                break;
            case 15: 
                menu_hanoi(); 
                break;
            case 16: 
                menu_performance(); 
                break;
            case 17: 
                menu_run_tests(); 
                break;
            case 0: 
                std::cout << "Exit" << std::endl; 
                break;
            default: 
                std::cout << "Invalid choice" << std::endl; 
                break;
        }
    }

    for (int idx = 0; idx < int_deque_count; idx++) {
        delete int_deques[idx];
    }

    for (int idx = 0; idx < double_deque_count; idx++) {
        delete double_deques[idx];
    }
    
    for (int idx = 0; idx < string_deque_count; idx++) {
        delete string_deques[idx];
    }

    for (int idx = 0; idx < complex_deque_count; idx++) {
        delete complex_deques[idx];
    }
}