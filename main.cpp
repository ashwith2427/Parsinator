#include "parser.hpp"
#include <iostream>

int main()
{
    auto sp = seqParser(characterParser('a'), stringParser("shwith"));
    return 0;
}
