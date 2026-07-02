#pragma once

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>

namespace algemate::math {

Fraction dotProduct(const Matrix<Fraction>& u, const Matrix<Fraction>& v);

Fraction normSquared(const Matrix<Fraction>& v);

bool isOrthogonal(const Matrix<Fraction>& u, const Matrix<Fraction>& v);

bool isOrthogonalSet(const Matrix<Fraction>& V);

bool areLinearlyIndependent(const Matrix<Fraction>& V);

Matrix<Fraction> gramSchmidt(const Matrix<Fraction>& V);

}
