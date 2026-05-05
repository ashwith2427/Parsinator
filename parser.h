#include <iostream>
#include <optional>
#include <span>
#include <stdint.h>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

class ParseError {
public:
    constexpr ParseError(const char* error)
        : _msg(error)
    {
    }
    constexpr const char* error() { return _msg; }

private:
    const char* _msg;
};

template <class T> class Result {
public:
    using value_type = T;

    constexpr Result(T v, uint32_t idx)
        : _ok(true)
        , _value(v)
        , _idx(idx)
    {
    }

    constexpr Result(ParseError e, uint32_t idx)
        : _ok(false)
        , _error(e)
        , _idx(idx)
    {
    }

    constexpr bool is_ok() const { return _ok; }
    constexpr bool is_error() const { return !_ok; }

    constexpr T const& ok() const { return _value; }
    constexpr ParseError const& error() const { return _error; }

    constexpr uint32_t index() const { return _idx; }

private:
    bool _ok;
    T _value { };
    ParseError _error { "" };
    uint32_t _idx;
};

template <> class Result<void> {
public:
    constexpr Result(uint32_t idx)
        : _ok(true)
        , _idx(idx)
    {
    }

    constexpr Result(ParseError e, uint32_t idx)
        : _ok(false)
        , _error(e)
        , _idx(idx)
    {
    }

    constexpr bool is_ok() const { return _ok; }
    constexpr bool is_error() const { return !_ok; }

    constexpr ParseError const& error() const { return _error; }
    constexpr uint32_t index() const { return _idx; }

private:
    bool _ok;
    ParseError _error { "" };
    uint32_t _idx;
};

template <class T, class Tok, class Fn> class Parser {
public:
    using result_type = T;
    using token_type = Tok;
    constexpr Parser(Fn&& fn)
        : _apply(std::forward<Fn>(fn))
    {
    }
    constexpr Result<T> parse(std::span<Tok const> tokens) const
    {
        return _apply(tokens);
    }
    template <class MapFn> constexpr auto map(MapFn&& fn) const
    {
        using U = std::invoke_result_t<MapFn, T>;
        auto newFn
            = [=, *this](std::span<Tok const> tokens) -> Result<U> {
            auto res = this->parse(tokens);
            if (res.is_error()) {
                return Result<U>(res.error(), res.index());
            }
            return Result<U>(fn(res.ok()), res.index());
        };
        return Parser<U, Tok, decltype(newFn)>(std::move(newFn));
    }

private:
    Fn _apply;
};

template <class T, class Tok, class Fn>
constexpr decltype(auto) parser(Fn&& fn)
{
    return Parser<T, Tok, Fn>(std::forward<Fn>(fn));
}

template <class P> using ValueOf = typename P::result_type;

template <class P> using TokenOf = typename P::token_type;

template <class... Ts> struct type_list { };

template <class T, class List> struct contains;

template <class T>
struct contains<T, type_list<>> : std::false_type { };

template <class T, class Head, class... Tail>
struct contains<T, type_list<Head, Tail...>>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type,
          contains<T, type_list<Tail...>>> { };

template <class T, class List> struct prepend;

template <class T, class... Ts> struct prepend<T, type_list<Ts...>> {
    using type = type_list<T, Ts...>;
};

template <class List> struct unique_list;

template <> struct unique_list<type_list<>> {
    using type = type_list<>;
};

template <class Head, class... Tail>
struct unique_list<type_list<Head, Tail...>> {
private:
    using rest_unique =
        typename unique_list<type_list<Tail...>>::type;

public:
    using type
        = std::conditional_t<contains<Head, rest_unique>::value,
            rest_unique, typename prepend<Head, rest_unique>::type>;
};

template <class... Lists> struct concat;

template <class... Ts> struct concat<type_list<Ts...>> {
    using type = type_list<Ts...>;
};

template <class... Ts, class... Us, class... Rest>
struct concat<type_list<Ts...>, type_list<Us...>, Rest...> {
    using type =
        typename concat<type_list<Ts..., Us...>, Rest...>::type;
};

template <class T> struct to_list {
    using type = type_list<T>;
};

template <class... Ts> struct to_list<std::tuple<Ts...>> {
    using type = typename concat<typename to_list<Ts>::type...>::type;
};

template <class List> struct to_tuple;

template <class... Ts> struct to_tuple<type_list<Ts...>> {
    using type = std::tuple<Ts...>;
};

template <class... Ts>
struct flatten_tuple : to_tuple<typename to_list<Ts...>::type> { };

template <class... Ts>
using flatten_tuple_t = typename flatten_tuple<Ts...>::type;

template <class T> struct is_tuple : std::false_type { };

template <class... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type { };

template <class T> constexpr auto as_tuple(T&& v)
{
    if constexpr (is_tuple<std::decay_t<T>>::value) {
        return std::forward<T>(v);
    } else {
        return std::tuple<T> { std::forward<T>(v) };
    }
}

template <class A, class B> constexpr auto Sequence(A&& a, B&& b)
{
    using T1 = ValueOf<A>;
    using T2 = ValueOf<B>;
    using Tok = TokenOf<A>;
    using T = flatten_tuple_t<std::tuple<T1, T2>>;
    return parser<T, Tok>(
        [=](std::span<Tok const> tokens) constexpr -> Result<T> {
            auto res1 = a.parse(tokens);
            if (res1.is_error()) {
                return { res1.error(), res1.index() };
            }
            auto res2 = b.parse(tokens.subspan(res1.index()));
            if (res2.is_error()) {
                return { res2.error(), res1.index() + res2.index() };
            }
            return Result<T>(std::tuple_cat(as_tuple(res1.ok()),
                                 as_tuple(res2.ok())),
                res1.index() + res2.index());
        });
}

template <class P> constexpr auto Optional(P&& p)
{
    using T = ValueOf<P>;
    using Tok = TokenOf<P>;
    return parser<std::optional<T>, Tok>(
        [=](std::span<Tok const> tokens) constexpr
            -> Result<std::optional<T>> {
            auto res = p.parse(tokens);
            if (res.is_error()) {
                return { std::nullopt, 0 };
            }
            return { std::optional<T> { res.ok() }, res.index() };
        });
}

template <class P> constexpr auto Discard(P&& p)
{
    using T = ValueOf<P>;
    using Tok = TokenOf<P>;
    return parser<void, Tok>(
        [=](auto tokens) constexpr -> Result<void> {
            auto res = p.parse(tokens);
            if (res.is_error()) {
                return Result<void>(0);
            }
            size_t idx = res.index();
            while (!res.is_error()) {
                if (res.index() == 0)
                    break;
                res = p.parse(tokens.subspan(idx));
                idx += res.index();
            }
            return Result<void>(idx);
        });
}

template <class T, size_t N> struct vec {
    std::array<T, N> data { };
    size_t size = 0;
};

template <uint64_t N, class P> constexpr auto Many(P&& p)
{
    using T = vec<ValueOf<P>, N>;
    using Tok = TokenOf<P>;

    return parser<T, Tok>(
        [=](std::span<Tok const> tokens) constexpr -> Result<T> {
            T result { };
            uint32_t idx = 0;
            while (result.size < N) {
                auto res = p.parse(tokens.subspan(idx));
                if (res.is_error())
                    break;
                if (res.index() == 0)
                    break;
                result.data[result.size++] = res.ok();
                idx += res.index();
            }
            return { result, idx };
        });
}

template <class... Ts> struct to_list<std::variant<Ts...>> {
private:
    using expanded =
        typename concat<typename to_list<Ts>::type...>::type;

public:
    using type = expanded;
};

template <class...> struct to_variant;

template <class... Ts> struct to_variant<type_list<Ts...>> {
    using type = std::variant<Ts...>;
};

template <class... Ts>
struct flatten_variant
    : to_variant<
          typename unique_list<typename to_list<Ts...>::type>::type> {
};

template <class... Ts>
using flatten_variant_t = typename flatten_variant<Ts...>::type;

template <class A, class B> constexpr auto Choice(A&& a, B&& b)
{
    using T = flatten_variant_t<ValueOf<A>, ValueOf<B>>;
    using Tok = TokenOf<A>;
    return parser<T, Tok>([=](auto tokens) constexpr -> Result<T> {
        auto res1 = a.parse(tokens);
        if (res1.is_ok()) {
            return Result<T>(T { res1.ok() }, res1.index());
        }
        auto res2 = b.parse(tokens);
        if (res2.is_ok()) {
            return Result<T>(T { res2.ok() }, res2.index());
        }
        return Result<T>(
            "Choice parser cannot parse single parser", 0);
    });
}

constexpr auto CharacterParser(char c)
{
    return parser<char, char>([=](auto tokens) constexpr
                                  -> Result<char> {
        if (!tokens.empty() and tokens[0] == c) {
            return Result<char>(c, 1);
        }
        return Result<char>(
            ParseError(
                "Character Parser Failed: Characters not matched"),
            0);
    });
}

template <uint32_t idx> struct ErrorAt;

template <bool Ok, uint32_t Index> struct DebugParse {
    static_assert(Ok, "Parse failed");

    using Fail = ErrorAt<Index>;
};

template <class T> void print_T()
{
    std::cout << __PRETTY_FUNCTION__ << '\n';
}

template <class A, class B> constexpr auto operator>>(A&& a, B&& b)
{
    return Sequence(std::forward<A>(a), std::forward<B>(b));
};
