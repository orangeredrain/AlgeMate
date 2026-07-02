#ifndef ALGEMATE_CALC_INTERACTIVE_VALUE_H
#define ALGEMATE_CALC_INTERACTIVE_VALUE_H

#include "math/core/AlgReal.h"
#include "math/core/Complex.h"
#include "math/core/Matrix.h"
#include "RenderSettings.h"

#include <QString>
#include <string>
#include <utility>
#include <vector>

class QTextDocument;

namespace AlgeMate::Calculator::Interactive {

using Scalar  = algemate::math::AlgReal;
using MatrixA = algemate::math::Matrix<Scalar>;
using ComplexC = algemate::math::Complex;

class Value {
public:
    enum class Kind { Scalar, ComplexScalar, Matrix, Polynomial, RootList, Factored, VectorList, NamedMatrices, Text };

    Value();                               
    Value(const Scalar& s);
    Value(const ComplexC& c);
    Value(const MatrixA& m);

    Value(const std::vector<Scalar>& coeffs, const QString& var);

    static Value makeRootList(std::vector<std::pair<Scalar, Scalar>> roots, const QString& var);

    static Value makeFactored(std::vector<Scalar> origCoeffs,
                              Scalar leading,
                              std::vector<std::pair<std::vector<Scalar>, int>> factors,
                              QString var);

    static Value makeVectorList(std::vector<MatrixA> vectors, const QString& var);

    static Value makeNamedMatrices(std::vector<std::pair<QString, MatrixA>> items);

    static Value makeText(QString content);

    Kind kind() const { return kind_; }
    bool isScalar()        const { return kind_ == Kind::Scalar; }
    bool isComplexScalar() const { return kind_ == Kind::ComplexScalar; }
    bool isAnyScalar()     const { return kind_ == Kind::Scalar || kind_ == Kind::ComplexScalar; }
    bool isMatrix()        const { return kind_ == Kind::Matrix; }
    bool isPolynomial()    const { return kind_ == Kind::Polynomial; }
    bool isRootList()      const { return kind_ == Kind::RootList; }
    bool isFactored()      const { return kind_ == Kind::Factored; }
    bool isVectorList()    const { return kind_ == Kind::VectorList; }
    bool isNamedMatrices() const { return kind_ == Kind::NamedMatrices; }
    bool isText()          const { return kind_ == Kind::Text; }

    const Scalar&   asScalar()  const { return s_; }
    const ComplexC& asComplex() const { return c_; }
    const MatrixA&  asMatrix()  const { return m_; }
    const std::vector<Scalar>& asPolyCoeffs() const { return polyCoeffs_; }
    const QString&             polyVar()      const { return polyVar_; }
    const std::vector<std::pair<Scalar, Scalar>>& asRoots() const { return roots_; }
    const QString&                                rootVar() const { return rootVar_; }
    const std::vector<MatrixA>& asVectors() const { return vectors_; }
    const QString&              vecVar()    const { return vecVar_; }
    const std::vector<std::pair<QString, MatrixA>>& asNamedMatrices() const { return namedMats_; }
    const QString& asText() const { return textContent_; }

    ComplexC toComplex() const;

    Value add(const Value& r) const;
    Value sub(const Value& r) const;
    Value mul(const Value& r) const;
    Value div(const Value& r) const;
    Value pow(const Value& r) const;
    Value neg() const;
    Value transpose() const;

    QString toHtml(const RenderTheme& th, const DisplayFormat& fmt,
                   QTextDocument* doc = nullptr) const;

    QString toPlain(const DisplayFormat& fmt) const;

    QString typeLabel() const;

private:
    Kind    kind_;
    Scalar  s_;
    ComplexC c_;
    MatrixA m_;
    std::vector<Scalar> polyCoeffs_;
    QString polyVar_;
    std::vector<std::pair<Scalar, Scalar>> roots_;
    QString rootVar_;
    std::vector<std::pair<std::vector<Scalar>, int>> factors_;
    std::vector<MatrixA> vectors_;
    QString vecVar_;
    std::vector<std::pair<QString, MatrixA>> namedMats_;
    QString textContent_;
};

QString renderNoteWithLatex(const QString& src, const RenderTheme& th,
                            QTextDocument* doc, int fontPt = 13);

QString latexForImageUrl(const QString& url);

void clearLatexImageCache();

}

#endif
