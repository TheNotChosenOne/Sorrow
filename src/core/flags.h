#pragma once

#include <ctti/type_id.hpp>
#include <iostream>
// Ensures we get the right definition for std::less
// before you use flags
#include "entities/signature.h"
#include "utility/io.h"

typedef ctti::type_id_t FlagType;

class BaseFlag {
    public:
        virtual ~BaseFlag() { }
        virtual void dump(std::ostream &os) const = 0;
};

std::ostream &operator<<(std::ostream &os, const BaseFlag &flag);

template< typename T >
class Flag: public BaseFlag {
    public:
        typedef T Type;
        Type value;

        ~Flag() { }
        Flag(T value) : value(value) { }
        void dump(std::ostream &os) const {
            os << "Flag(" << ctti::nameof< T >() << ")";
        }
};
