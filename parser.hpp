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

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

struct ParserError {
    std::uint32_t idx;
    const char* msg;
    friend std::ostream& operator<<(
        std::ostream& os, ParserError error)
    {
        os << "Parser Failed: \n";
        os << "Index: " << error.idx << '\n';
        os << "Reason: " << error.msg << '\n';
        return os;
    }
};

template <class T, class = void> class Result {
    std::uint32_t idx;
    std::variant<T, ParserError> value;

public:
    using value_type = T;
    constexpr static Result Ok(std::uint32_t idx, T value)
    {
        return Result(idx, value);
    }

    constexpr static Result Err(ParserError error)
    {
        return Result(error);
    }

    constexpr Result() = default;

    constexpr bool is_err() const
    {
        return std::holds_alternative<ParserError>(value);
    }
    constexpr bool is_ok() const
    {
        return std::holds_alternative<T>(value);
    }
    constexpr bool is_void() const { return false; }
    constexpr std::uint32_t index() const { return idx; }
    constexpr ParserError error() const
    {
        return std::get<ParserError>(value);
    }
    constexpr T unwrap() const { return std::get<T>(value); }

    friend std::ostream& operator<<(std::ostream& os, Result r)
    {
        if (r.is_err()) {
            os << "Error: " << r.error() << '\n';
            os << "Position: " << r.index() << '\n';
        } else {
            os << "Result: " << r.unwrap() << '\n';
        }
        return os;
    }

private:
    constexpr Result(std::uint32_t idx, T value)
        : idx(idx)
        , value(value)
    {
    }
    constexpr Result(ParserError err)
        : idx(err.idx)
        , value(err)
    {
    }

    constexpr Result(std::monostate m, std::uint32_t idx)
        : idx(idx)
        , value(m)
    {
    }
};

template <> class Result<void> {
public:
    using value_type = void;
    constexpr bool is_void() { return true; }
    constexpr std::uint32_t index() const
    {
        return std::get<std::uint32_t>(value);
    }
    constexpr ParserError error() const
    {
        return std::get<ParserError>(value);
    }
    constexpr bool is_ok() const
    {
        return std::holds_alternative<std::uint32_t>(value);
    }
    constexpr bool is_err() const
    {
        return std::holds_alternative<ParserError>(value);
    }
    constexpr static Result Ok(std::uint32_t idx)
    {
        return Result(idx);
    }
    constexpr static Result Err(ParserError err)
    {
        return Result(err);
    }

private:
    constexpr Result(std::uint32_t idx)
        : value(idx)
    {
    }
    constexpr Result(ParserError error)
        : value(error)
    {
    }
    std::variant<std::uint32_t, ParserError> value;
};

template <class Res> using InnerResultType = typename Res::value_type;

template <class Output, class Fn> class Parser {
public:
    constexpr Parser(Fn&& f)
        : apply(std::forward<Fn>(f))
    {
    }
    constexpr auto parse(std::string_view input) const
        -> Result<Output>
    {
        return apply(input);
    }

private:
    Fn apply;
};

template <class Output, class Fn> constexpr auto ParserType(Fn&& f)
{
    return Parser<Output, std::decay_t<Fn>>(std::forward<Fn>(f));
}

constexpr auto characterParser(char c)
{
    return ParserType<char>(
        [=](std::string_view input) constexpr -> Result<char> {
            if (input.empty())
                return Result<char>::Err(
                    ParserError(0, "Empty input"));
            if (input[0] == c)
                return Result<char>::Ok(1, c);
            return Result<char>::Err(ParserError(0, "No match"));
        });
}

constexpr auto stringParser(std::string_view expected)
{
    return ParserType<std::string_view>([=](std::string_view
                                                input) constexpr {
        if (expected.size() <= input.size()) {
            if (input.starts_with(expected)) {
                return Result<std::string_view>::Ok(
                    expected.size(), expected);
            } else {
                return Result<std::string_view>::Err(
                    ParserError(0, "String cannot be matched!"));
            }
        }
        return Result<std::string_view>::Err(ParserError(0,
            "Expected string should be less than or equal to input "
            "string"));
    });
}

template <class Parser>
using InnerType = decltype(std::declval<Parser>().parse(
    std::declval<std::string_view>()));

template <size_t I = 0, class TupleResult, class FirstParser,
    class... RestParsers>
constexpr decltype(auto) parseEach(TupleResult& tuple,
    std::string_view input, FirstParser&& first,
    RestParsers&&... rest)
{
    auto result = first.parse(input);
    if (result.is_err()) {
        return Result<TupleResult>::Err(result.index(),
            "Parser " + std::to_string(I) + " cannot be parsed");
    }
    std::get<I>(tuple) = result;
    std::string_view remaining = input.substr(result.index());
    if constexpr (sizeof...(RestParsers) == 0) {
        return Result<TupleResult>::Ok(
            input.size() - remaining.size(), tuple);
    } else {
        return parseEach<I + 1>(
            tuple, remaining, std::forward<RestParsers>(rest)...);
    }
}

template <class... Parsers>
constexpr auto seqParser(Parsers&&... parsers)
{
    using TupleResult = std::tuple<InnerType<Parsers>...>;
    return ParserType<TupleResult>(
        [=](std::string_view input) constexpr -> Result<TupleResult> {
            TupleResult result_tuple { InnerType<Parsers>()... };
            return parseEach(result_tuple, input, parsers...);
        });
}

template <class ResultType, size_t I = 0, class Parser>
constexpr std::pair<int, ResultType> choice_impl(
    std::string_view input, Parser&& parser)
{
    auto result = parser.parse(input);
    if (result.is_err()) {
        return Result<ResultType>::Err(ParserError(
            result.index(), "None of the parsers parsed"));
    }
    return { result.index(), ResultType(result.unwrap()) };
}
template <class ResultType, size_t I = 0, class FirstParser,
    class... RestParsers>
constexpr std::pair<int, ResultType> choice_impl(
    std::string_view input, FirstParser&& first,
    RestParsers&&... rest)
{
    auto result = first.parse(input);
    if (result.is_ok()) {
        return { result.index(), ResultType(result.unwrap()) };
    }
    if constexpr (sizeof...(RestParsers) == 0) {
        return Result<ResultType>::Err(ParserError(result.index(),
            "None of the parsers matched in choice parser"));
    }
    return choice_impl<ResultType, I + 1>(
        input, std::forward<RestParsers>(rest)...);
}

template <class... Parsers>
constexpr decltype(auto) choice(Parsers&&... parsers)
{
    using ResultType
        = std::variant<InnerResultType<InnerType<Parsers>>...>;
    return ParserType<ResultType>(
        [=](std::string_view input) constexpr {
            auto [idx, res]
                = choice_impl<ResultType>(input, parsers...);
            return Result<decltype(res)>::Ok(idx, res);
        });
}

template <size_t N, class Parser>
constexpr auto manyNParser(Parser&& parser)
{
    using ResultType = InnerResultType<InnerType<Parser>>;
    using ResultArray = std::array<ResultType, N>;
    return ParserType<ResultArray>(
        [=](std::string_view input) constexpr -> Result<ResultArray> {
            ResultArray set {};
            size_t idx = 0;
            for (int i = 0; i < N; i++) {
                if (idx >= input.size()) {
                    return Result<ResultArray>::Err(ParserError(0,
                        "NXM(M = expected_size) is greater than "
                        "input.size()"));
                }
                auto result = parser.parse(input.substr(idx));
                if (result.is_ok() or result.is_empty()) {
                    set[i] = result.unwrap();
                    idx += result.index();
                } else {
                    return Result<ResultArray>::Err(ParserError(
                        0, "Parser failed to parse N times."));
                }
            }
            return Result<ResultArray>(set, idx);
        });
}

template <class Parser> constexpr decltype(auto) skip(Parser&& parser)
{
    return ParserType<void>(
        [=](std::string_view input) constexpr -> Result<void> {
            size_t consumed = 0;
            while (true) {
                auto result = parser.parse(input.substr(consumed));
                if (result.is_err()) {
                    break;
                }
                if (result.index() == 0)
                    break;
                consumed += result.index();
            }
            return Result<void>::Ok(consumed);
        });
}

template <class Parser>
constexpr decltype(auto) discard(Parser&& parser)
{
    return ParserType<void>(
        [=](std::string_view input) constexpr -> Result<void> {
            auto result = parser.parse(input);
            if (result.is_ok()) {
                return Result<void>::Ok(result.index());
            }
            return Result<void>::Err(
                ParserError(0, "Discard parser failed to parse"));
        });
}
