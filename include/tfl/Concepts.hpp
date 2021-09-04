#pragma once

#include <concepts>
#include <ranges>

/**
 * @brief Contains some <a href="https://en.cppreference.com/w/cpp/concepts">concepts</a>.
 * @file
 */

namespace tfl {

    /**
     * @brief Checks if a type is among a list of types.
     * 
     * The associated helper `is_among_v` is provided for convenience.
     *
     * @tparam T The type to test.
     * @tparam Args The list of types.
     *
     */
    template<typename T, typename... Args>
    struct is_among : std::integral_constant<bool, (std::is_same_v<T, Args> || ...)> {};

    /**
     * @brief Heper value for \ref is_among.
     */
    template<typename T, typename... Args>
    inline constexpr bool is_among_v = is_among<T, Args...>::value;



    /**
     * @brief Specifies that a type is a range whose iteration yields a specific type.
     * 
     * @tparam R The range type.
     * @tparam V The iterated type.
     */
    template<typename R, typename V>
    concept range_of = std::ranges::range<R> && std::same_as<V, std::ranges::range_value_t<R>>;



    /**
     * @brief Specifies that a type is a container.
     * 
     * A container is a type which verifies:
     * - It is a \ref range_of;
     * - It is default constructible;
     * - It is copiable;
     * - It supports `push_back` for element addition.
     * 
     * @tparam R The container type.
     * @tparam V The iterated type.
     */
    template<class R, class V>
    concept container = 
        range_of<R, V> &&
        std::default_initializable<R> && 
        std::copy_constructible<R> && 
        requires(R r, V&& v){ r.push_back(v); };



    /**
     * @brief Specifies that a type is invocable with some specific argument types and yields a specific return type.
     * 
     * @tparam F The invocable type.
     * @tparam R The return type.
     * @tparam Args The argument types.
     */
    template<typename F, typename R, typename... Args>
    concept invocable_with_result = std::invocable<F, Args...> && std::same_as<R, std::invoke_result_t<F, Args...>>;
}