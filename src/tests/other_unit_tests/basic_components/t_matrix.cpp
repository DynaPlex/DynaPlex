#include <gtest/gtest.h>
#include "dynaplex/modelling/matrix.h"

#include "dynaplex/modelling/queue.h"
#include "dynaplex/error.h"
#include "dynaplex/vargroup.h"

namespace DynaPlex::Tests {

    TEST(Matrix, ConstructorAndGetters) {
        Matrix<int64_t> intMatrix(3, 2, 42);
        EXPECT_EQ(intMatrix.rows(), 3);
        EXPECT_EQ(intMatrix.cols(), 2);
        EXPECT_EQ(intMatrix.at(0, 0), 42);
    }

    TEST(Matrix, AtMethod) {
        Matrix<double> doubleMatrix(2, 3, 3.14);
        EXPECT_EQ(doubleMatrix.at(0, 0), 3.14);

        doubleMatrix.at(0, 0) = 2.71;
        EXPECT_EQ(doubleMatrix.at(0, 0), 2.71);

        // Test bounds checking
        EXPECT_THROW(doubleMatrix.at(3, 0), DynaPlex::Error);
        EXPECT_THROW(doubleMatrix.at(0, 4), DynaPlex::Error);
    }

    TEST(Matrix, VarGroupConversion) {
        Matrix<std::string> stringMatrix(2, 2, "hello");
        VarGroup vg = stringMatrix.ToVarGroup();

        // Test conversion back to Matrix
        Matrix<std::string> loadedMatrix(vg);
        EXPECT_EQ(loadedMatrix.at(0, 0), "hello");
        EXPECT_EQ(loadedMatrix.at(1, 1), "hello");
        EXPECT_EQ(loadedMatrix.rows(), 2);
        EXPECT_EQ(loadedMatrix.cols(), 2);

        // Test for invalid VarGroup
        vg.Set("rows_", 3); // Make it inconsistent
        EXPECT_THROW(Matrix<std::string> invalidMatrix(vg), DynaPlex::Error);
    }

    TEST(Matrix, OutOfBoundsAccess) {
        Matrix<int64_t> int64Matrix(1, 1, 0);
        EXPECT_THROW(int64Matrix.at(1, 1), DynaPlex::Error);
        EXPECT_THROW(int64Matrix.at(-1, 0), DynaPlex::Error);
    }
    TEST(Matrix, Add_to_vargroup) {
        Matrix<int64_t> int64Matrix(3, 3, 0);
        VarGroup vars{};
        vars.Add("matrix", int64Matrix);
        Matrix<int64_t> int64Matrix_2;

        vars.Get("matrix", int64Matrix_2);

        EXPECT_EQ(int64Matrix.cols(), int64Matrix_2.cols());


    }



    

} // namespace DynaPlex::Tests
