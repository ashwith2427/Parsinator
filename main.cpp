#include "parser.hpp"
#include <chrono>
#include <ios>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

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

template <class T> void print_T()
{
    std::cout << __PRETTY_FUNCTION__ << '\n';
}
// Helper to make overloaded lambdas
template <typename T> struct is_variant : std::false_type { };

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type { };

template <typename T>
constexpr bool is_variant_v = is_variant<T>::value;

// Overloaded helper for std::visit (if you want)
template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename T, typename E>
void print_result(const Result<T, E>& res)
{
    if (res.is_ok()) {
        if constexpr (is_variant_v<T>) {
            // Call std::visit only if T is a variant
            std::visit(
                overloaded { [](char c) {
                                std::cout << "char: " << c << '\n';
                            },
                    [](std::string_view sv) {
                        std::cout << "string_view: " << sv << '\n';
                    } },
                res.getValue());
        } else {
            // Plain type: print directly
            std::cout << res.getValue() << '\n';
        }
    } else {
        std::cout << "Error: " << res.getError() << '\n';
    }
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();
    constexpr auto result = stringParser("ash").parse("ashwith");
    // std::cout << res.parse("ashwith");
    // print_tuple(result.getValue());
    // print_T<decltype(result.parse(""))>();
    // print_result(result);
    auto end = std::chrono::high_resolution_clock::now();

    std::cout
        << "Microseconds: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
               (end - start))
        << '\n';
}
