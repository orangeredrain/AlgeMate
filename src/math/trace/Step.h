#pragma once

#include "core/Fraction.h"
#include "core/Matrix.h"

#include <cstddef>
#include <string>

namespace algemate::math {

enum class StepKind {
    Initial,      
    SwapRows,     
    ScaleRow,     
    AddMulRow,    
    SelectPivot,  
    Note,         
    Conclude      
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
