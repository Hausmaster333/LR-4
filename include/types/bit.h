#ifndef BIT_H
#define BIT_H

class Bit {
    private:
        bool value;
    public:
        Bit() : value(false) {}
        Bit(bool val) : value(val) {}
        Bit(int val) : value(val != 0) {}

        bool get() const { return value; }

        Bit operator&(const Bit& other) const { return Bit(value && other.value); } // Логическое И
        Bit operator|(const Bit& other) const { return Bit(value || other.value); } // Логическое ИЛИ
        Bit operator^(const Bit& other) const { return Bit(value != other.value); } // Логическое XOR
        Bit operator~() const { return Bit(!value); } // Логическое НЕ

        bool operator==(const Bit& other) const { return value == other.value; }
        bool operator!=(const Bit& other) const { return value != other.value; }
};

#endif