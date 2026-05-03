#include "parser.h"

int main()
{
    constexpr auto result
        = CharacterParser<'f'>().parse(TokenStream("vailed", 6));
    return 0;
}
