#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace algemate::math {

class BigInt {
public:
    BigInt();
    BigInt(long long v);                   
    explicit BigInt(const std::string& s); 

    static BigInt fromString(const std::string& s); 

    BigInt  operator-() const;
    BigInt  operator+(const BigInt& r) const;
    BigInt  operator-(const BigInt& r) const;
    BigInt  operator*(const BigInt& r) const;
    BigInt  operator/(const BigInt& r) const;
    BigInt  operator%(const BigInt& r) const;

    BigInt& operator+=(const BigInt& r);
    BigInt& operator-=(const BigInt& r);
    BigInt& operator*=(const BigInt& r);
    BigInt& operator/=(const BigInt& r);
    BigInt& operator%=(const BigInt& r);

    BigInt& operator++();
    BigInt  operator++(int);
    BigInt& operator--();
    BigInt  operator--(int);

    bool operator==(const BigInt& r) const;
    bool operator!=(const BigInt& r) const;
    bool operator<(const BigInt& r) const;
    bool operator<=(const BigInt& r) const;
    bool operator>(const BigInt& r) const;
    bool operator>=(const BigInt& r) const;

    bool isZero() const;     
    bool isOne() const;      
    bool isNegative() const; 
    bool isPositive() const; 
    int  sign() const;       

    BigInt abs() const;      
    BigInt negate() const;   

    std::string toString() const;   
    long long   toLongLong() const; 
    double      toDouble() const;   

    static BigInt gcd(BigInt a, BigInt b);                  
    static BigInt pow(BigInt base, unsigned int exponent);  

private:
    std::vector<uint32_t> limbs_; 
    bool negative_ = false;       

    void normalize_(); 
    void setFromUnsigned_(unsigned long long v); 

    static int    absCompare_(const BigInt& a, const BigInt& b); 
    static BigInt absAdd_(const BigInt& a, const BigInt& b); 
    static BigInt absSub_(const BigInt& a, const BigInt& b); 
    static BigInt absMul_(const BigInt& a, const BigInt& b); 
    static void   absDivMod_(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r); 

    bool        bitAt_(std::size_t pos) const;  
    void        setBit_(std::size_t pos);       
    std::size_t bitLength_() const;             
    void        shiftLeftOneBit_();             
};

std::ostream& operator<<(std::ostream& os, const BigInt& v); 

}
