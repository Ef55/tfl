#pragma once

#include <memory>
#include <functional>
#include <variant>

#include "Concepts.hpp"

namespace tfl {

    template<typename> class Lazy;

    /**
     * @brief Specifies that a type `F` is suitable for `Lazy<T>::flat_map(F&&)`.
     * @see lazy_flat_map_result_t For the type which will result from the call.
     */
    template<typename F, typename T>
    concept lazy_flat_mapper = 
        std::invocable<F, T const&> &&
        std::same_as<std::invoke_result_t<F, T const&>, Lazy<typename std::invoke_result_t<F, T const&>::ValueType>>;

    /**
     * @brief Type resulting from `Lazy<T>::flat_map(F&&)`.
     */
    template<typename T, typename F> requires lazy_flat_mapper<F, T>
    using lazy_flat_map_result_t = typename std::invoke_result_t<F, T const&>::ValueType;

    /**
     * @brief Contains either a value or a computation which generates the value.
     *
     * Lazys are useful to represent values which are 
     * 1. Expensive to compute;
     * 2. Not necessarily needed.
     *
     * This typically mean that we don't want to compute the value uselessly,
     * but still want to have some caching mechanism to avoid computing multiple times.
     *
     * Lazy is designed for this: once provided with some code to execute (a computation), it will
     * only compute it if the value is requested, but not otherwise.
     * The value is then stored, such that, if it is requested again, 
     * it can be provided without doing the computation again. 
     * 
     * @tparam T Type of the value resulting from the computation.
     */
    template<typename T>
    class Lazy final {
        using Computation = std::function<T()>;
        using Value = T;
        using Var = std::variant<Value, Computation>;

        std::shared_ptr<Var> _state;

        Lazy(Computation const& computation): _state(std::make_shared<Var>(computation)) {}
        Lazy(Value const& value): _state(std::make_shared<Var>(value)) {}

    public:
        using ValueType = T;

        /**
         * @name Constructors
         *
         * @note All constructors guarantee that the provided computation
         * will be evaluated at most once.
         * @warning Every argument will be copied.
         * @{
         */

        /**
         * @brief Creates an already evaluated lazy with given value.
         */
        template<std::convertible_to<T> S>
        static Lazy<T> value(S&& value) {
            return Lazy<T>(std::forward<S>(value));
        }

        /**
         * @brief Creates an a lazy from a function to evaluate when the value is required.
         */
        template<std::invocable F> requires std::convertible_to<std::invoke_result_t<F>, T>
        static Lazy<T> computation(F&& computation) {
            return Lazy<T>(Computation(computation));
        }

        /**
         * @brief Creates an a lazy from a function to call with the provided arguments when the value is required.
         */
        template<typename F, typename... Args> requires 
            std::invocable<F, Args...> &&
            std::convertible_to<std::invoke_result_t<F, Args...>, T>
        static Lazy<T> computation(F&& computation, Args&&... args) {
            return Lazy<T>(Computation([computation, args...](){ return std::invoke(computation, args...); }));
        }

        /**
         * @brief Creates an a lazy from a type to construct with the provided arguments when the value is required.
         */
        template<std::convertible_to<T> S = T, typename... Args> requires 
            std::constructible_from<S, Args...>
        static Lazy<T> construction(Args&&... args) {
            return Lazy<T>([args...](){ return S(args...); });
        }

        ///@}

        /**
         * @brief Checks whether the computation was already done.
         */
        bool evaluated() const {
            return std::holds_alternative<Value>(*_state);
        }

        /**
         * @brief Computes the value (if not yet computed).
         */
        void kick() const {
            if(!evaluated()) {
                *_state = std::get<Computation>(*_state)();
            }
        }

        /**
         * @name Value access
         * @brief Returns the computed value. Performs the computation if needed.
         * @{
         */
        T const& get() const {
            kick();
            return std::get<Value>(*_state);
        }

        operator T const&() const {
            return get();
        }
        ///@}

        /**
         * @name Monad operations
         * @{
         */

        /**
         * @brief Applies (lazily) a function to the result of this computation.
         * 
         * @note This computation is guaranteed to happen at most once, even if map is called.
         */
        template<std::invocable<T const&> F>
        Lazy<std::invoke_result_t<F, T const&>> map(F&& map) const {
            using S = std::invoke_result_t<F, T const&>;
            Lazy<T> self(*this);
            return Lazy<S>::computation([self, map](){ return map(self.get()); });
        }

        /**
         * @brief Applies (lazily) a function to the result of this computation.
         * 
         * @note This computation is guaranteed to happen at most once, even if map is called.
         * @tparam F A function which generates another lazy computation.
         */
        template<lazy_flat_mapper<T> F>
        Lazy<lazy_flat_map_result_t<T, F>> flat_map(F&& map) const {
            Lazy<T> self(*this);
            return Lazy<lazy_flat_map_result_t<T, F>>::computation([self, map](){ return map(self.get()).get(); });
        }

        ///@}

    };
}