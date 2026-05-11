#ifndef IENUMERATOR_H
#define IENUMERATOR_H

template <class T>
class IEnumerator {
public:
    virtual bool move_next() = 0; // Перейти к следующему элементу
    virtual const T& get_current() const = 0; // Получить текущий элемент
    virtual void reset() = 0; // Вернуться в начало
    virtual ~IEnumerator() {}
};

template <class T>
class EnumeratorWrapper {
private:
    IEnumerator<T>* iter;
    bool has_current;
public:
    EnumeratorWrapper(IEnumerator<T>* iter) : iter(iter), has_current(false) {}

    bool move_next() {
        has_current = iter->move_next();
        return has_current;
    }

    const T& get_current() const {
        return iter->get_current();
    }

    void reset() {
        iter->reset();
        has_current = false;
    }
    
    ~EnumeratorWrapper() {
        delete iter;
    }
};

#endif