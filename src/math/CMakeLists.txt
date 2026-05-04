add_library(algemate_math STATIC
    core/BigInt.h
    core/BigInt.cpp
    core/Fraction.h
    core/Fraction.cpp
    core/NumberFormatter.h
    core/NumberFormatter.cpp
    core/Matrix.h
    core/Polynomial.h
    core/AlgReal.h
    core/AlgReal.cpp
    core/Complex.h
    core/Complex.cpp
    core/PolynomialZp.h
    core/PolynomialZp.cpp
    core/SymbolicExpr.h
    core/SymbolicExpr.cpp
    trace/Step.h
    trace/StepSequence.h
    trace/StepSequence.cpp
    algorithm/LinearAlgebra.h
    algorithm/LinearAlgebra.cpp
    algorithm/PolynomialAlg.h
    algorithm/PolynomialAlg.cpp
    algorithm/LambdaMatrix.h
    algorithm/LambdaMatrix.cpp
    algorithm/RationalCanonical.h
    algorithm/RationalCanonical.cpp
    algorithm/ComplexEigen.h
    algorithm/ComplexEigen.cpp
    algorithm/JordanForm.h
    algorithm/JordanForm.cpp
    algorithm/Orthogonalization.h
    algorithm/Orthogonalization.cpp
    algorithm/BilinearForm.h
    algorithm/BilinearForm.cpp
    algorithm/OrthogonalDiag.h
    algorithm/OrthogonalDiag.cpp
    algorithm/RealEigen.h
    algorithm/RealEigen.cpp
    algorithm/JordanFormAlg.h
    algorithm/JordanFormAlg.cpp
    algorithm/FactorOverQ.cpp
    algorithm/SVD.h
    algorithm/SVD.cpp
    mpoly/Monomial.h
    mpoly/MPolynomial.h
    mpoly/PowerSum.h
    mpoly/SymmetricReduction.h
)

target_include_directories(algemate_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(algemate_math PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)
