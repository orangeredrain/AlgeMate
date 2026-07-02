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

struct EvalResult {
    bool    ok = true;

    QString assignedName;     
    Value   value;            
    QString typeDesc;         
    QString extraNote;        

    QString error;
    int     errorPos = 0;
};

class Evaluator {
public:
    EvalResult evaluate(const QString& source);

    const std::unordered_map<std::string, Value>& env() const { return env_; }
    void clear() { env_.clear(); }

    static std::vector<QString> supportedFunctions();

private:
    std::unordered_map<std::string, Value> env_;
    QString lastCallNote_;

    Value eval_(const NodePtr& n);
    Value callFn_(const std::string& fn, const std::vector<Value>& args);

    void collectFreeVars_(const NodePtr& n, std::set<std::string>& out);
    bool dependsOnVar_(const NodePtr& n, const std::string& var);
    std::vector<Scalar> astToPolyCoeffs_(const NodePtr& n, const std::string& var);
};

}

#endif
