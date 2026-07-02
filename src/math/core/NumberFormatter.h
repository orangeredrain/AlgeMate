#pragma once

#include "Fraction.h"

#include <string>

namespace algemate::math {

enum class RoundMode {
    Truncate, 
    HalfUp,   
    Floor,    
    Ceil      
};

class NumberFormatter {
public:

    static std::string toDecimal(const Fraction& f,
                                 int precision = 6,
                                 RoundMode mode = RoundMode::HalfUp);

    static std::string toRepeatingDecimal(const Fraction& f,
                                          int maxPeriod = 64);

    static Fraction fromDecimal(const std::string& s);

    static Fraction fromDouble(double v,
                               long long maxDenominator = 1000000);
};

}
