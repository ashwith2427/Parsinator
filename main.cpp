#include "parser.hpp"
#include <ios>
#include <iostream>
#include <ostream>
#include <type_traits>
#include <utility>

template <class T, class = void>
struct is_printable : std::false_type { };

template <class T>
struct is_printable<T,
    std::void_t<decltype(std::declval<std::ostream&>()
        << std::declval<T>())>> : std::true_type { };

template <typename Tuple, std::size_t... Is>
void print_tuple_impl(const Tuple& tup, std::index_sequence<Is...>)
{
    ((std::cout << (Is == 0 ? "" : ", ") << std::get<Is>(tup)), ...);
}

template <typename... Args>
void print_tuple(const std::tuple<Args...>& tup)
{
    std::cout << "(";
    print_tuple_impl(tup, std::index_sequence_for<Args...> {});
    std::cout << ")\n";
}

int main()
{
    auto sp = seqParser(characterParser('a'), stringParser("shwith"));
    auto result = sp.parse("ashwith");
    print_tuple(result.getValue());
}
