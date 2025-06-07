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
#include <optional>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
template <class T, class E = const char*> class Result {
    std::uint32_t idx;
    std::optional<T> value;
    std::optional<E> error;

public:
    constexpr static Result Ok(std::uint32_t idx, T value)
    {
        return Result(idx, value);
    }

    constexpr static Result Err(std::uint32_t idx, E error)
    {
        return Result(idx, error);
    }

    constexpr bool is_err() const { return error.has_value(); }
    constexpr bool is_ok() const { return value.has_value(); }
    constexpr std::uint32_t getIndex() const { return idx; }
    constexpr E getError() const { return error.value(); }
    constexpr T getValue() const { return value.value(); }

    friend std::ostream& operator<<(std::ostream& os, Result r)
    {
        if (r.is_err()) {
            os << "Error: " << r.getError() << '\n';
            os << "Position: " << r.getIndex() << '\n';
        } else {
            os << "Result: " << r.getValue() << '\n';
        }
        return os;
    }

private:
    constexpr Result(std::uint32_t idx, T value)
        : idx(idx)
        , value(value)
    {
    }
    constexpr Result(std::uint32_t idx, E err)
        : idx(idx)
        , error(err)
    {
    }
};

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
        [=](std::string_view input) -> Result<char> {
            if (input.empty())
                return Result<char>::Err(0, "Empty input");
            if (input[0] == c)
                return Result<char>::Ok(1, c);
            return Result<char>::Err(0, "No match");
        });
}

constexpr auto stringParser(std::string_view expected)
{
    return ParserType<std::string_view>([=](std::string_view input) {
        if (expected.size() <= input.size()) {
            if (input.starts_with(expected)) {
                return Result<std::string_view>::Ok(
                    expected.size(), expected);
            } else {
                return Result<std::string_view>::Err(
                    0, "String cannot be matched!");
            }
        }
        return Result<std::string_view>::Err(0,
            "Expected string should be less than or equal to input "
            "string");
    });
}
