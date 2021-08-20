#pragma once

#include <concepts>
#include <ranges>

namespace tfl {

    template<typename T, typename... Args>
    struct is_among : std::integral_constant<bool, (std::is_same_v<T, Args> || ...)> {};

    template<typename T, typename... Args>
    inline constexpr bool is_among_v = is_among<T, Args...>::value;



    template<typename> class Regex;
    template<typename, typename> class Parser;

    template<class C>
    struct iterated {};

    template<std::ranges::range C>
    struct iterated<C> {
        typedef std::iterator_traits<decltype(std::declval<C>().begin())>::value_type type;
    };

    template<class T>
    using iterated_t = typename iterated<T>::type;



    template<typename T, typename V>
    concept iterable = std::ranges::range<T> && std::same_as<V, iterated_t<T>>;



    template<class T, class V>
    concept container = 
        std::default_initializable<T> && 
        std::copy_constructible<T> && 
        iterable<T, V> &&
        requires(T t, V&& v){ t.push_back(v); };



    template<typename F, typename R, typename... Args>
    concept invocable_with_result = std::invocable<F, Args...> && std::same_as<R, std::invoke_result_t<F, Args...>>;



    template<typename P>
    struct is_regex : std::false_type {};

    template<typename T>
    struct is_regex<Regex<T>> : std::true_type {};

    template<typename R> requires  std::derived_from<R, Regex<typename R::TokenType>>
    struct is_regex<R> : std::true_type {};

    template<typename T>
    inline constexpr bool is_regex_v = is_regex<T>::value;

    template<class R>
    concept regex = is_regex_v<R>;



    template<typename P>
    struct is_parser : std::false_type {};

    template<typename T, typename R>
    struct is_parser<Parser<T, R>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_parser_v = is_parser<T>::value;

    template<class P>
    concept parser = is_parser_v<P>;

}