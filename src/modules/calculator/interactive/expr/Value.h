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

// Calculator 交互式引擎的动态类型:  标量 (AlgReal) / 复标量 (Complex = Q[i]) / 矩阵 / 多项式
class Value {
public:
    enum class Kind { Scalar, ComplexScalar, Matrix, Polynomial, RootList, Factored, VectorList, NamedMatrices, Text };

    Value();                               // 默认 0 标量
    Value(const Scalar& s);
    Value(const ComplexC& c);
    Value(const MatrixA& m);
    // 多项式: coeffs 低次 → 高次, var 为变量名 (如 "λ" / "x")
    Value(const std::vector<Scalar>& coeffs, const QString& var);

    // 工厂: 根列表 (特征值 / 有理根等); roots[i] = (实部, 虚部); var 为显示符号 ("λ" / "x" / ...)
    static Value makeRootList(std::vector<std::pair<Scalar, Scalar>> roots, const QString& var);

    // 工厂: 因式分解结果. 展示为  f(x) = L * (p1)^e1 * (p2)^e2 ... 的 LaTeX 等式.
    //   origCoeffs: 原多项式系数 (低次→高次)
    //   leading:    首项系数 L
    //   factors:    [(因式系数, 重数), ...]
    //   var:        变量名 ("x" / "λ" / ...)
    static Value makeFactored(std::vector<Scalar> origCoeffs,
                              Scalar leading,
                              std::vector<std::pair<std::vector<Scalar>, int>> factors,
                              QString var);

    // 工厂: 向量列表 (零空间基 / 特征向量组等); vectors[i] = n×1 列向量; var 为显示符号 ("η" / "v" / ...)
    static Value makeVectorList(std::vector<MatrixA> vectors, const QString& var);

    // 工厂: 带标签的矩阵列表 (LU / QR / 合同对角化的多矩阵输出).
    //   items[i].first  为 LaTeX 片段 (如 "L = " / "Q = "), 直接用 embedLatexAsImg 渲染;
    //   items[i].second 为矩阵本体, 渲染策略同通用矩阵.
    //   允许 空矩阵 (0×0) 作为纯标签行, 此时只渲染 LaTeX 内容.
    static Value makeNamedMatrices(std::vector<std::pair<QString, MatrixA>> items);

    // 工厂: 纯文本结果 (支持内联 $...$ 的 LaTeX 片段).
    //   适用于 definiteness / signature 等结果以文字为主的函数.
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

    // 两种标量的统一提升: 把实标量提升为复标量, 复标量原样返回
    ComplexC toComplex() const;

    // 四则运算 / 幂 / 取负 / 转置  (异常抛 std::runtime_error)
    Value add(const Value& r) const;
    Value sub(const Value& r) const;
    Value mul(const Value& r) const;
    Value div(const Value& r) const;
    Value pow(const Value& r) const;
    Value neg() const;
    Value transpose() const;

    // 格式化: 主题感知 + 精确/小数切换
    //   doc 非空时, 矩阵以 QPixmap + QTextDocument image resource 的方式渲染为真正的方括号数学版式;
    //   doc 为空时退化为纯 HTML table 版本 (用于单独测试)
    QString toHtml(const RenderTheme& th, const DisplayFormat& fmt,
                   QTextDocument* doc = nullptr) const;
    // 纯文本 (单行或多行), 用于 clipboard / 日志
    QString toPlain(const DisplayFormat& fmt) const;

    // 辅助: 类型描述 "标量" / "3×3 矩阵" / "λ 的多项式 (次数 N)"
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

// 渲染含内联 LaTeX 片段的说明文本: `$...$` 中间走 LaTeX 渲染, 其余文本 HTML 转义.
// 使用示例: "在 $\\mathbb{Q}[x]$ 上不可约" → 文字 + 公式图.
// doc 必须非空 (用于注册 QPixmap 资源). fontPt 默认 13pt 适配小字号说明.
QString renderNoteWithLatex(const QString& src, const RenderTheme& th,
                            QTextDocument* doc, int fontPt = 13);

// 根据公式图片的资源 URL (calc-tex://N) 查询对应的 LaTeX 源码; 未知返回空串。
// 用于输出区复制时把图片还原为 LaTeX 文本。
QString latexForImageUrl(const QString& url);

// 清空 URL → LaTeX 映射缓存。QTextDocument 重建时（清空历史/主题切换）应调用，
// 否则旧图片的 LaTeX 源码会一直驻留内存。
void clearLatexImageCache();

}

#endif
