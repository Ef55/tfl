#pragma once

#include <stdexcept>
#include <deque>
#include <concepts>
#include <compare>
#include <iterator>

/**
 * @brief Contains the definition of \ref tfl::InputBuffer.
 * @file
 */

namespace tfl {

    /**
     * @brief A container whose values a lazily taken from another range.
     *
     * Satisfies `input_range` and `output_range`.
     * 
     * @tparam R Type of the underlying range.
     */
    template<std::ranges::input_range R>
    class InputBuffer final {
    public:
        /** @brief Type of the contained values. */
        using ValueType = std::ranges::range_value_t<R>;
        /** @brief Type for indices. */
        using SizeType = std::size_t;

        class Iterator;
        class Sentinel;

    private:

        std::deque<ValueType> _buf;
        std::ranges::iterator_t<R> _next;
        std::ranges::sentinel_t<R> _end;

        bool shift() {
            if(!consumed_all()) {
                ValueType val = *_next;
                _buf.push_back(val);
                ++_next;
                return true;
            }
            else {
                return false;
            }
        }

        bool ensure(SizeType idx) {
            while(buffed_size() <= idx) {
                if(!shift()) {
                    return false;
                }
            }
            return true;
        }

    public:

        /**
         * @brief Creates an input buffer on top of a range.
         *
         * @warning The range is expected to be alive for at least
         * as long as this buffer.
         */
        InputBuffer(R&& range): _buf(), _next(std::ranges::begin(range)), _end(std::ranges::end(range)) {}

        /**
         * @brief Checks whether the range was fully consumed. Might never be true.
         */
        bool consumed_all() const {
            return _next == _end;
        }

        /**
         * @brief Returns the number of values actually in the buffer.
         */
        auto buffed_size() const {
            return _buf.size();
        }

        /**
         * @brief Gets value at `idx` in the buffer.
         *
         * Might consume values from the input range.
         */
        ValueType& operator[](SizeType idx) {
            if(!ensure(idx)) {
                throw std::invalid_argument("Index out of bounds.");
            }

            return _buf[idx];
        }

        /**
         * @brief Remove the `count` first values from the buffer.
         * 
         * All indices will be shifted by `-count`.
         *
         * @warning Every iterator (except the sentinel) will be invalidated.
         */
        void release(SizeType count) {
            for(; count > 0; --count) {
                if(_buf.empty()) {
                    throw std::invalid_argument("Cannot release value: not enough values in buffer.");
                }

                _buf.pop_front();
            }
        }

        /**
         * @brief Returns an iterator pointing to the first value of this buffer.
         */
        Iterator begin() {
            return Iterator(0, this);
        }

        /**
         * @brief Returns the sentinel iterator.
         */
        Sentinel end() {
            return SENTINEL;
        }

        /**
         * @brief Allows traversal of the values of the associated buffer.
         */
        class Iterator final {
            friend class InputBuffer;
            SizeType _idx;
            InputBuffer* _parent;

            Iterator(SizeType idx, InputBuffer* parent): _idx(idx), _parent(parent) {}

        public:
            /** Type for distance between two iterators. */
            using difference_type = std::ptrdiff_t;
            /** Type obtained after dereference. */
            using value_type = ValueType;
            
            /**
             * @deprecated Here for compatibility with compilers not fixing this <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2325r3.html">defect</a>.
             */
            [[deprecated]]
            Iterator(): _idx(0), _parent(nullptr) {
                throw std::logic_error("InputBuffer::Iterator() shouldn't be called: just here for compatibility.");
            }

            /**
             * @brief Increment this iterator.
             *
             * Might consume values from the input range.
             * 
             * @return This.
             */
            Iterator& operator++() {
                _parent->ensure(_idx++);
                return *this;
            }

            /**
             * @brief Increment this iterator.
             *
             * Might consume values from the input range.
             * 
             * @return A copy of `this` before being incremented.
             */
            Iterator operator++(int) {
                Iterator old(_idx, _parent);
                _parent->ensure(_idx++);
                return old;
            }

            /**
             * @brief Dereference `this`.
             *
             * Might consume values from the input range.
             * 
             * @return A value in the underlying buffer.
             * @throws std::out_of_range If `this` is past the end.
             */
            value_type& operator*() const {
                if(_parent->ensure(_idx)) {
                    return (*_parent)[_idx];
                }
                else {
                    throw std::out_of_range("Buffer iterator is out of bounds.");
                }
            }

            /**
             * @brief Compute the number of values in range [`this`, `that`[.
             */
            difference_type operator-(Iterator const& that) const {
                return _idx - that._idx;
            }

            /**
             * @brief Tests whether the end of the buffer was reached.
             */
            bool operator==(Sentinel const& that) const {
                return _parent->consumed_all() && _idx == _parent->buffed_size();
            }

            /**
             * @brief Orders two iterators.
             *
             * Two iterators are strongly ordered when on the same
             * buffer, and unordered otherwise.
             */
            std::partial_ordering operator<=>(Iterator const& that) const {
                return _parent != that._parent ? 
                    std::partial_ordering::unordered :
                    _idx <=> that._idx; 
            }

            /**
             * @brief Two iterators are equal if they have the same parent and point to the same value.
             */
            bool operator==(Iterator const& that) const = default;
        };

        /**
         * @brief Represents an iterator which traversed the whole buffer.
         */
        class Sentinel final {
        public:
            constexpr Sentinel() {}

            /** @see \ref Iterator::operator==() */
            bool operator==(Iterator const& that) const {
                return that == *this;
            }
        };

        /** Sentinel iterator. */
        static constexpr Sentinel SENTINEL{};

    };

    /** @brief CTAD for \ref InputBuffer. */
    template<class R>
    InputBuffer(R&&) -> InputBuffer<std::remove_cv_t<R>>;
}