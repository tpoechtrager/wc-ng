#include <iostream>
#include <cstdlib>
#include <cstring>

#ifndef __GXX_EXPERIMENTAL_CXX0X__
#define constexpr
#endif

#define STANDALONE
#include "../mod/wcversion.h"

int main()
{
    std::cout << CLIENTVERSION.major << "."
              << CLIENTVERSION.minor << "."
              << CLIENTVERSION.patch
              << std::endl;
    return 0;
}
