#ifndef ORDINAL_INDEXABLE_H
#define ORDINAL_INDEXABLE_H

#include "lazy/ordinal_index.h"

// Mixin-интерфейс для генераторов, которые поддерживают прямой доступ по ординальному индексу (omega_part, finite_part).
// Реализуют: ConcatGenerator, InsertAtGenerator.
// LazySequence::get(OrdinalIndex) делает один dynamic_cast к этому интерфейсу - удобно расширять новыми операциями без правок LazySequence.
template <class T>
class OrdinalIndexable {
    public:
        virtual T get_at(OrdinalIndex idx) const = 0;
        virtual ~OrdinalIndexable() = default;
};

#endif
