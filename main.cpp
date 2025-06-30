#include "parser.hpp"
#include <chrono>
#include <iostream>
#include <ostream>
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

// template <class T> void print_T()
// {
//     std::cout << __PRETTY_FUNCTION__ << '\n';
// }
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

template <typename T> void print_result(const Result<T>& res)
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
                res.unwrap());
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
    auto result = choice(stringParser("a"), stringParser("also"))
                      .parse("ashwith");
    if (!result.is_err()) {
        print_T<decltype(result.unwrap())>();
        std::cout << std::get<Result<std::string_view>>(
            result.unwrap());
    } else {
        std::cout << result.error();
    }
    int a[10];
    // std::cout << res.parse("ashwith");
    // print_tuple(result.getValue());
    // print_T<decltype(result.parse(""))>();
    // print_result(result);
    // if (result.is_err()) {
    //     std::cout << result.error();
    // } else {
    //     std::cout << "Test Passed:\n";
    //     std::cout << "Value: " << result.index() << '\n';
    // }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout
        << "Microseconds: "
        << std::chrono::duration_cast<std::chrono::microseconds>(
               (end - start))
        << '\n';
}
