#pragma once
#include <algorithm> //push_heap, pop_heap. 
#include <functional> //std::greater
namespace DynaPlex {

    namespace Concepts {

        template <typename T>
        concept Comparable = requires(T a, T b) {
            { a > b } -> std::convertible_to<bool>;
            { a < b } -> std::convertible_to<bool>;
        };
    }

    /**
     * @brief Custom heap implementation with iterator access.
     * @tparam T The type of elements in the heap.
      */
    template <typename T>
    class EventHeap {
      
    public:
        using value_type = T;
        
        EventHeap() {
            static_assert(DynaPlex::Concepts::Comparable<T>, "DynaPlex::EventHeap<T> : Type T must be comparable using the '>' operator");
        }


        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;
      
        /**
         * @brief Get an iterator to the beginning of the container.
         * @return Iterator to the start of the container.
         */
        iterator begin() { return data.begin(); }

        /**
         * @brief Get a constant iterator to the beginning of the container.
         * @return Constant iterator to the start of the container.
         */
        const_iterator begin() const { return data.begin(); }

        /**
         * @brief Get an iterator to the end of the container.
         * @return Iterator to the end of the container.
         */
        iterator end() { return data.end(); }

        /**
         * @brief Get a constant iterator to the end of the container.
         * @return Constant iterator to the end of the container.
         */
        const_iterator end() const { return data.end(); }


        /**
        * @brief Add an element to the heap. Note: Provided for consistency with API - heap does not have a back
        * so it will just be pushed onto the heap. 
        * @param value The value to add to the heap.
        */
        void push_back(const T& value) {
            push(value);
        }

        /**
         * @brief Add an element to the heap.
         * @param value The value to add to the heap.
         */
        void push(const T& value) {
            data.push_back(value);
            std::push_heap(data.begin(), data.end(), std::greater<T>());

        }

        /**
         * @brief Remove the top item from the heap.
         */
        void pop() {
            std::pop_heap(data.begin(), data.end(), std::greater<T>());
            data.pop_back();
        }

        /**
         * @brief Access the first element of the heap.
         * @return A constant reference to the top element.
         */
        const T& first() const {
            return data.front();
        }

        /**
         * @brief Clear all elements from the heap.
         */
        void clear() {
            data.clear();
        }
 

    private:
        /** @brief The underlying container storing the heap elements. */
        std::vector<T> data;

       

    };

    /**
     * Two heaps are considered equal if, after sorting, the matching elements match.
     */
    template<typename T>
    bool operator==(const DynaPlex::EventHeap<T>& lhs, const DynaPlex::EventHeap<T>& rhs) {
        auto lhs_data = std::vector<T>(lhs.begin(), lhs.end());
        auto rhs_data = std::vector<T>(rhs.begin(), rhs.end());
        std::sort(lhs_data.begin(), lhs_data.end());
        std::sort(rhs_data.begin(), rhs_data.end());
        return lhs_data == rhs_data;
    }
}