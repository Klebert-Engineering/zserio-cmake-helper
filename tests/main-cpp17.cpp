#include <iostream>

// zserio::Optional<T> is declared by the
// C++17 runtime, but not by the default
// zserio C++ runtime.
#include "zserio/Optional.h"
#if __has_include(<zserio/AnyHolder.h>)
#    error "C++17 Binary linked to non-C++17 runtime!"
#endif

int main()
{
    zserio::Optional<int> opt(123);

    std::cout << "Test Binary Main.\n";
    return 0;
}
