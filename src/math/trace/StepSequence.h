#pragma once

/*
* @file StepSequence.h
* @brief Step 序列容器, 记录算法从初始态到终止态的全部中间态
*
* 算法函数通过 push* 接口追加步骤, UI 层遍历 steps() 即可按序呈现
*/

#include "Step.h"

#include <cstddef>
#include <string>
#include <vector>

namespace algemate::math {

class StepSequence {
public:
    void pushInitial    (const Matrix<Fraction>& m, std::string desc = "初始矩阵");
    void pushSwapRows   (std::size_t i, std::size_t j, const Matrix<Fraction>& after);
    void pushScaleRow   (std::size_t i, const Fraction& k, const Matrix<Fraction>& after);
    void pushAddMulRow  (std::size_t i, std::size_t j, const Fraction& k, const Matrix<Fraction>& after);
    void pushSelectPivot(std::size_t r, std::size_t c, const Matrix<Fraction>& after);
    void pushNote       (std::string desc, const Matrix<Fraction>& after);
    void pushConclude   (std::string desc, const Matrix<Fraction>& after);

    const std::vector<Step>& steps() const { return steps_; }
    std::size_t size() const  { return steps_.size(); }
    bool        empty() const { return steps_.empty(); }
    void        clear()       { steps_.clear(); }

    std::string toString() const;

private:
    std::vector<Step> steps_;
};

}
