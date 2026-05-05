#pragma once

/*
* @file Step.h
* @brief 算法执行的单步记录, 供过程可视化使用
*
* 每一步包含操作类型 kind, 行/列下标 i/j, 分数参数 k,
* 操作完成后的矩阵快照 snapshot
*/

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <string>

namespace algemate::math {

enum class StepKind {
    Initial,      // 初始矩阵
    SwapRows,     // R_i <-> R_j
    ScaleRow,     // R_i *= k
    AddMulRow,    // R_i += k * R_j
    SelectPivot,  // 选主元 (r, c)
    Note,         // 纯文字说明
    Conclude      // 结论
};

struct Step {
    StepKind         kind = StepKind::Note;
    std::size_t      i = 0;
    std::size_t      j = 0;
    Fraction         k;
    Matrix<Fraction> snapshot;
    std::string      description;
};

}
