/*
 * MIT License

 Copyright (c) 2025 ashwith2427

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

template <class T> constexpr void print_T()
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

template <typename T> struct is_tuple : std::false_type { };

template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type { };

template <typename T>
constexpr bool is_tuple_v = is_tuple<std::decay_t<T>>::value;

template <typename T, typename... List>
constexpr bool is_in_list = (std::is_same_v<T, List> || ...);

template <typename... Types> struct flat_type_container {
    using type = flat_type_container<Types...>;
    using variant = std::variant<Types...>;
};

template <typename First, typename... Types>
struct flat_type_container<First, flat_type_container<Types...>> {
    using type = flat_type_container<First, Types...>;
};

template <typename First, typename... Rest> struct unique_types {
    using type =
        typename std::conditional_t<!is_in_list<First, Rest...>,
            typename flat_type_container<First,
                typename unique_types<Rest...>::type>::type,
            typename unique_types<Rest...>::type>;
};

template <typename First> struct unique_types<First> {
    using type = flat_type_container<First>;
};

template <typename... Ts>
constexpr auto flat_tuple(std::tuple<Ts...>);

template <typename T>
constexpr auto flat_one_impl(std::true_type, T&& value)
{
    return flat_tuple(std::forward<T>(value));
}

template <typename T>
constexpr auto flat_one_impl(std::false_type, T&& value)
{
    return std::make_tuple(std::forward<T>(value));
}

template <typename T> constexpr auto flat_one(T&& value)
{
    return flat_one_impl(
        is_tuple<std::decay_t<T>> {}, std::forward<T>(value));
}

template <typename... Ts>
constexpr auto flat_tuple(std::tuple<Ts...> tup)
{
    return std::apply(
        [](auto&&... elems) {
            return std::tuple_cat(
                flat_one(std::forward<decltype(elems)>(elems))...);
        },
        std::forward<std::tuple<Ts...>>(tup));
}

struct ParseError {
    std::string error;
};

template <class T> class Result {
public:
    static constexpr bool is_void = false;
    Result() = delete;
    constexpr static Result Ok(std::uint32_t idx, T value)
    {
        return Result(idx, value);
    }
    constexpr static Result Err(std::uint32_t idx, ParseError error)
    {
        return Result(idx, error);
    }

    constexpr bool is_error() const
    {
        return std::holds_alternative<ParseError>(value_);
    }
    constexpr bool is_ok() const
    {
        return std::holds_alternative<T>(value_);
    }

    constexpr std::uint32_t index() const { return idx_; }

    constexpr T const& unwrap() const { return std::get<T>(value_); }
    constexpr ParseError error() const
    {
        return std::get<ParseError>(value_);
    }

private:
    std::uint32_t idx_;
    std::variant<T, ParseError> value_;
    constexpr Result(std::uint32_t idx, T value)
        : idx_(idx)
        , value_(value)
    {
    }
    constexpr Result(std::uint32_t idx, ParseError error)
        : idx_(idx)
        , value_(error)
    {
    }
};

template <> class Result<void> {
public:
    static constexpr bool is_void = true;
    static constexpr Result Ok(std::size_t index)
    {
        return Result(index);
    }
    constexpr std::uint32_t index() { return idx_; }

private:
    constexpr Result(std::uint32_t index)
        : idx_(index)
    {
    }
    std::uint32_t idx_;
};

template <class Output, class TokenT, class Fn> class ParserType {
public:
    using inner_type = Output;
    using token_type = TokenT;
    constexpr ParserType(Fn&& f)
        : apply(std::forward<Fn>(f))
    {
    }
    constexpr auto parse(std::span<TokenT const> tokens) const
        -> Result<Output>
    {
        return apply(tokens);
    }

    template <typename NewOutput, typename MapFn>
    constexpr auto map(MapFn&& mapFunction) const
    {
        auto lambda_ = [*this, g = std::forward<MapFn>(mapFunction)](
                           std::span<TokenT const> tokens) {
            auto result = this->parse(tokens);
            if (result.is_error()) {
                return Result<NewOutput>::Err(
                    result.index(), result.error());
            }
            return Result<NewOutput>::Ok(
                result.index(), g(result.unwrap()));
        };

        return ParserType<NewOutput, TokenT, decltype(lambda_)>(
            std::move(lambda_));
    }

private:
    Fn apply;
};

template <class Output, class TokenT, class Fn>
constexpr auto Parser(Fn&& fn)
{
    return ParserType<Output, TokenT, std::decay_t<Fn>>(
        std::forward<Fn>(fn));
}

template <class P>
using InnerType = typename std::remove_reference_t<P>::inner_type;

template <class P>
using TokenOf = typename std::remove_reference_t<P>::token_type;

template <class A, class B>
constexpr auto sequenceParser(A&& a, B&& b)
{
    using ResultTuple = std::tuple<InnerType<A>, InnerType<B>>;
    using TokenT = TokenOf<A>;
    static_assert(std::is_same_v<TokenOf<A>, TokenOf<B>>,
        "All parsers must have same token type.");
    return Parser<ResultTuple, TokenT>(
        [=](std::span<TokenT const> tokens) constexpr
            -> Result<ResultTuple> {
            auto res1 = a.parse(tokens);
            if (res1.is_error()) {
                return Result<ResultTuple>::Err(
                    res1.index(), res1.error());
            }
            auto res2 = b.parse(tokens.subspan(res1.index()));
            if (res2.is_error()) {
                return Result<ResultTuple>::Err(
                    res1.index() + res2.index(), res2.error());
            }
            return Result<ResultTuple>::Ok(
                res1.index() + res2.index(),
                ResultTuple(res1.unwrap(), res2.unwrap()));
        });
}

template <class P> constexpr auto optionalParser(P&& p)
{
    using ResultType = std::optional<InnerType<P>>;
    using TokenT = TokenOf<P>;
    return Parser<ResultType, TokenT>(
        [=](std::span<TokenT const> tokens) constexpr
            -> Result<ResultType> {
            auto result = p.parse(tokens);
            if (result.is_error()) {
                return Result<ResultType>::Ok(0, std::nullopt);
            }
            return Result<ResultType>::Ok(
                result.index(), result.unwrap());
        });
}

constexpr auto MaxLimit = std::numeric_limits<std::uint64_t>::max();
template <class P>
constexpr auto manyParser(
    P&& parser, const std::uint64_t max = MaxLimit)
{
    using ResultType = std::vector<InnerType<P>>;
    using TokenT = TokenOf<P>;
    return Parser<ResultType, TokenT>(
        [=](std::span<TokenT const> tokens) {
            ResultType results {};
            auto result = parser.parse(tokens);
            std::size_t idx = result.index();
            if (result.is_error()) {
                return Result<ResultType>::Err(
                    result.index(), result.error());
            }
            std::uint64_t counter {};
            while (!result.is_error()) {
                if (counter >= max) {
                    return Result<ResultType>::Err(idx,
                        ParseError(
                            "Maximum iterations reached for many "
                            "parser. Use Block based slidiing."));
                }
                results.push_back(result.unwrap());
                result = parser.parse(tokens.subspan(idx));
                idx += result.index();
            }
            return Result<ResultType>::Ok(idx, results);
        });
}

template <class P> constexpr auto discard(P&& parser)
{
    using TokenT = TokenOf<P>;
    return Parser<void, TokenT>(
        [=](std::span<TokenT const> tokens) constexpr
            -> Result<void> {
            auto result = parser.parse(tokens);
            if (result.is_error()) {
                return Result<void>::Ok(0);
            }
            std::size_t idx = result.index();
            while (!result.is_error()) {
                result = parser.parse(tokens.subspan(idx));
                idx += result.index();
            }
            return Result<void>::Ok(idx);
        });
}

template <class... Ts> struct type_list { };

template <class, class, class...> struct flattended_type_list;

template <class... Ts, class U>
struct flattended_type_list<type_list<Ts...>, U> {
    using type = type_list<Ts..., U>;
};

template <class... Ts, class... Us>
struct flattended_type_list<type_list<Ts...>, std::variant<Us...>> {
    using type = type_list<Ts..., Us...>;
};

template <class... Ts, class U, class... Rest>
struct flattended_type_list<type_list<Ts...>, U, Rest...> {
    using type = typename flattended_type_list<type_list<Ts..., U>,
        Rest...>::type;
};

template <class... Ts, class... Us, class... Rest>
struct flattended_type_list<type_list<Ts...>, std::variant<Us...>,
    Rest...> {
    using type =
        typename flattended_type_list<type_list<Ts..., Us...>,
            Rest...>::type;
};

template <class T> void unique_id_creator();
template <class T> constexpr auto unique_id = &unique_id_creator<T>;

template <typename T, typename U, std::size_t N>
constexpr std::size_t filter_unique_shift_duplicates(
    std::array<std::pair<T, U>, N>& arr)
{
    std::size_t write_index = 0;
    for (std::size_t i = 0; i < N; ++i) {
        bool is_duplicate = false;
        for (std::size_t j = 0; j < write_index; ++j) {
            if (arr[i].second == arr[j].second) {
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) {
            if (i != write_index) {
                std::swap(arr[write_index], arr[i]);
            }
            ++write_index;
        }
    }

    return write_index == 0 ? 0 : write_index - 1;
}

template <class... Ts> constexpr auto filter_unique(type_list<Ts...>)
{
    auto indexpair = []<std::size_t... Is>(
                         std::index_sequence<Is...>) {
        return std::array { std::pair { Is, unique_id<Ts> }... };
    }(std::make_index_sequence<sizeof...(Ts)>());
    const auto pred
        = [](auto& a, auto& b) { return a.second == b.second; };
    return std::pair { indexpair,
        filter_unique_shift_duplicates(indexpair) };
}

template <class... Ts>
std::type_identity<std::tuple<Ts...>> to_tuple(type_list<Ts...>);

template <class... Ts> void unique_f()
{
    using flattened = flattended_type_list<type_list<>, Ts...>::type;
    using tuple = decltype(to_tuple(flattened {}))::type;
    constexpr auto filtered = filter_unique(flattened {});
    std::cout << filtered.second << '\n';
    for (auto [a, b] : filtered.first) {
        std::cout << a << '\n';
    }
}

template <class... Ts>
using unique_variant = decltype([]() {
    using flattened = flattended_type_list<type_list<>, Ts...>::type;
    using tuple = decltype(to_tuple(flattened {}))::type;
    constexpr auto filtered = filter_unique(flattened {});

    return [filtered]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::type_identity<std::variant<std::tuple_element_t<
            filtered.first[Is].first, tuple>...>> {};
    }(std::make_index_sequence<filtered.second + 1>());
}())::type;

template <class A, class B> constexpr auto choiceParser(A&& a, B&& b)
{
    using ResultType = unique_variant<InnerType<A>, InnerType<B>>;
    using TokenT = TokenOf<A>;
    static_assert(std::is_same_v<TokenOf<A>, TokenOf<B>>,
        "All parsers must have same token type.");
    return Parser<ResultType, TokenT>(
        [=](std::span<TokenT const> tokens) constexpr
            -> Result<ResultType> {
            auto res1 = a.parse(tokens);
            if (res1.is_ok()) {
                return Result<ResultType>::Ok(
                    res1.index(), ResultType(res1.unwrap()));
            }
            auto res2 = b.parse(tokens);
            if (res2.is_ok()) {
                return Result<ResultType>::Ok(
                    res2.index(), ResultType(res2.unwrap()));
            }
            return Result<ResultType>::Err(
                0, ParseError("No parser matched in Choice parser"));
        });
}

template <class A, class B> constexpr auto operator>>(A&& a, B&& b)
{
    return sequenceParser(std::forward<A>(std::decay_t<A>(a)),
        std::forward<B>(std::decay_t<B>(b)));
}

template <class A, class B> constexpr auto operator|(A&& a, B&& b)
{
    return choiceParser(std::forward<A>(std::decay_t<A>(a)),
        std::forward<B>(std::decay_t<B>(b)));
}
