#ifndef ORDINAL_INDEXABLE_H
#define ORDINAL_INDEXABLE_H

#include "lazy/ordinal.h"

template <class T>
class OrdinalIndexable {
    public:
        virtual T get_at(Ordinal idx) const = 0;
        virtual ~OrdinalIndexable() = default;
};

#endif
