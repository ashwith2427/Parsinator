#include "parser.hpp"
#include <iostream>

int main()
{
    constexpr auto sp = stringParser("bsh").parse("ashwith");
    std::cout << sp;
    return 0;
}
