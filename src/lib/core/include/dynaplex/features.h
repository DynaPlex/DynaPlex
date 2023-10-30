#pragma once
#include <span>
#include <vector>
#include <stdexcept>
#include "dynaplex/error.h"

namespace DynaPlex {

  
    template<typename T>
    concept ConvertibleToFloat = std::is_convertible_v<T, float>;

    // Define ReadableContainer
    template<typename T>
    concept FloatConvertibleContainer = requires(T a) {
        typename T::value_type;
        requires ConvertibleToFloat<typename T::value_type> || std::is_same_v<T, bool>;
        requires std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<decltype(a.begin())>::iterator_category>;
        requires std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<decltype(a.end())>::iterator_category>;
    };


    class Features {


    private:
        std::span<float> dataSpan;
        size_t index = 0;

        // Helper function to add a value and update the index
        void addValue(float value) {
            if (index < dataSpan.size()) {
                dataSpan[index] = value;
            }
            index++;
        }

    public:

        const bool IsFilled() const
        {
            return index == dataSpan.size();
        }

        // Constructor
        explicit Features(std::span<float> span) : dataSpan(span) {}

        const size_t NumFeatsAdded() const
        {
            return index;
        }

        void Add(int64_t value) {
            addValue(static_cast<float>(value));
        }

        void Add(double value) {
            addValue(static_cast<float>(value));
        }

        void Add(float value) {
            addValue(value);
        }

        void Add(bool value) {
            addValue(value ? 1.0f : 0.0f);
        }

        template<ConvertibleToFloat T>
        void Add(T value) {
            addValue(static_cast<float>(value));
        }

        template<FloatConvertibleContainer T>
        void Add(T values) {
            for (auto& v : values) {
                Add(v);
            }
        }

        void Add(std::span<int64_t> values) {
            for (auto& v : values) {
                Add(v);
            }
        }

        void Add(std::span<double> values) {
            for (auto& v : values) {
                Add(v);
            }
        }

        void Add(std::span<float> values) {
            for (auto& v : values) {
                Add(v);
            }
        }

        void Add(std::span<bool> values) {
            for (auto& v : values) {
                Add(v);
            }
        }
       
        float& operator[](size_t idx) {
            if (idx >= index || idx>= dataSpan.size()) {
                throw Error("Features[]: Index out of range");
            }
            return dataSpan[idx];
        }

        const float& operator[](size_t idx) const {
            if (idx >= index || idx >= dataSpan.size()) {
                throw Error("Features[]: Index out of range");
            }
            return dataSpan[idx];
        }
    };

} // namespace DynaPlex
