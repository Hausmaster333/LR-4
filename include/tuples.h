#ifndef TUPLES_H
#define TUPLES_H

template <class T1, class T2>
class Pair { // 2-Tuple
private:
    T1 first_value;
    T2 second_value;
public:
    Pair() : first_value(), second_value() {}
    Pair(const T1& first, const T2& second) : first_value(first), second_value(second) {}

    const T1& first() const { return first_value; }
    const T2& second() const { return second_value; }

    bool operator==(const Pair<T1, T2>& other) const {
        return first_value == other.first_value && second_value == other.second_value;
    }

    bool operator!=(const Pair<T1, T2>& other) const {
        return !(*this == other);
    }
};

template <class T1, class T2, class T3>
class Triple { // 3-Tuple
private:
    T1 first_value;
    T2 second_value;
    T3 third_value;
public:
    Triple() : first_value(), second_value(), third_value() {}
    Triple(const T1& first, const T2& second, const T3& third) : first_value(first), second_value(second), third_value(third) {}

    const T1& first() const { return first_value; }
    const T2& second() const { return second_value; }
    const T3& third() const { return third_value; }

    bool operator==(const Triple<T1, T2, T3>& other) const {
        return first_value == other.first_value && second_value == other.second_value && third_value == other.third_value;
    }

    bool operator!=(const Triple<T1, T2, T3>& other) const {
        return !(*this == other);
    }
};

#endif