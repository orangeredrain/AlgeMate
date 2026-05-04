#ifndef ALGEMATE_CALC_INTERACTIVE_EVALUATOR_H
#define ALGEMATE_CALC_INTERACTIVE_EVALUATOR_H

#include "Value.h"
#include "Parser.h"

#include <QString>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace AlgeMate::Calculator::Interactive {

// 求值结果 (面向 UI 直接消费)
struct EvalResult {
    bool    ok = true;

    // 成功:
    QString assignedName;     // 非空 = 刚做了赋值 (如 "m")
    Value   value;            // 表达式结果
    QString typeDesc;         // "标量" / "3 × 3 矩阵"
    QString extraNote;        // 额外说明 (比如 charpoly 的系数顺序)

    // 失败:
    QString error;
    int     errorPos = 0;
};

class Evaluator {
public:
    EvalResult evaluate(const QString& source);

    // 变量环境查询 (用于右栏 "变量列表" 显示)
    const std::unordered_map<std::string, Value>& env() const { return env_; }
    void clear() { env_.clear(); }

    // 支持的函数名列表 (用于 UI 提示 / 自动完成占位)
    static std::vector<QString> supportedFunctions();

private:
    std::unordered_map<std::string, Value> env_;
    QString lastCallNote_;

    Value eval_(const NodePtr& n);
    Value callFn_(const std::string& fn, const std::vector<Value>& args);

    // AST → 多项式 辅助 (仅为 gcd/factor/... 等多项式函数服务)
    void collectFreeVars_(const NodePtr& n, std::set<std::string>& out);
    bool dependsOnVar_(const NodePtr& n, const std::string& var);
    std::vector<Scalar> astToPolyCoeffs_(const NodePtr& n, const std::string& var);
};

}

#endif
