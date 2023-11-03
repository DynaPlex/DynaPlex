#pragma once
#include <vector>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "dynaplex/error.h"
#include "dynaplex/vargroup.h" 

namespace DynaPlex {

    template<typename T>
    class Matrix {
        static_assert(DynaPlex::Concepts::DP_ElementType<T>, " DynaPlex::Matrix<T> - T must be of DP_ElementType, i.e. double, int64_t, string, or DynaPlex::VarGroupConvertible.");


    private:
        int64_t rows_;
        int64_t cols_;
        std::vector<T> data_;

        int64_t index(int64_t row, int64_t col) const {
            if (row >= rows_ || row<0 || col >= cols_ ||col<0) {
                throw DynaPlex::Error("Matrix index out of bounds");
            }
            return row * cols_ + col;
        }

    public:
        Matrix()
            : rows_(0), cols_(0)
        {}
        Matrix(int64_t rows, int64_t cols, const T& defaultValue = T())
            : rows_(rows), cols_(cols), data_(rows* cols, defaultValue) {}

        explicit Matrix(const DynaPlex::VarGroup& vars)
        {
            vars.Get("rows_", rows_);
            vars.Get("cols_", cols_);
            vars.Get("data_", data_);
            if (data_.size() != rows_ * cols_)
            {
                throw DynaPlex::Error("Invalid matrix loaded.");
            }
        }

        T& at(int64_t row, int64_t col) {
            return data_[index(row, col)];
        }

        const T& at(int64_t row, int64_t col) const {
            return data_[index(row, col)];
        }

        int64_t rows() const {
            return rows_;
        }

        int64_t cols() const {
            return cols_;
        }
               

        VarGroup ToVarGroup() const {
            VarGroup vars{};
            vars.Add("rows_", rows_);
            vars.Add("cols_", cols_);
            vars.Add("data_", data_);
            return vars;
        }

    };
}
