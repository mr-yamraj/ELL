////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Matrix_test.h (math_test)
//  Authors:  Ofer Dekel
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Matrix.h"

using namespace ell;

template <typename ElementType, math::MatrixLayout Layout>
void TestMatrix1();

template <typename ElementType, math::MatrixLayout Layout>
void TestMatrix2();

template <typename ElementType, math::MatrixLayout Layout1, math::MatrixLayout Layout2>
void TestMatrixCopy();

template <typename ElementType>
void TestMatrixReference();

template <typename ElementType, math::MatrixLayout Layout, math::ImplementationType Implementation>
void TestMatrixOperations();

template <typename ElementType, math::MatrixLayout Layout>
void TestConstMatrixReference();

template <typename ElementType, math::ImplementationType Implementation>
void TestMatrixMatrixAdd();

template <typename ElementType, math::MatrixLayout LayoutA, math::MatrixLayout LayoutB, math::ImplementationType Implementation>
void TestMatrixMatrixMultiply();










#include "../tcc/Matrix_test.tcc"