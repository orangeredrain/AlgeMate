#include "StepSequence.h"

#include <sstream>
#include <utility>

namespace algemate::math {

void StepSequence::pushInitial(const Matrix<Fraction>& m, std::string desc) {
    Step s;
    s.kind        = StepKind::Initial;
    s.snapshot    = m;
    s.description = std::move(desc);
    steps_.push_back(std::move(s));
}

void StepSequence::pushSwapRows(std::size_t i, std::size_t j, const Matrix<Fraction>& after) {
    Step s;
    s.kind     = StepKind::SwapRows;
    s.i        = i;
    s.j        = j;
    s.snapshot = after;
    std::ostringstream oss;
    oss << "交换 R" << (i + 1) << " <-> R" << (j + 1);
    s.description = oss.str();
    steps_.push_back(std::move(s));
}

void StepSequence::pushScaleRow(std::size_t i, const Fraction& k, const Matrix<Fraction>& after) {
    Step s;
    s.kind     = StepKind::ScaleRow;
    s.i        = i;
    s.k        = k;
    s.snapshot = after;
    std::ostringstream oss;
    oss << "R" << (i + 1) << " *= " << k.toString();
    s.description = oss.str();
    steps_.push_back(std::move(s));
}

void StepSequence::pushAddMulRow(std::size_t i, std::size_t j, const Fraction& k, const Matrix<Fraction>& after) {
    Step s;
    s.kind     = StepKind::AddMulRow;
    s.i        = i;
    s.j        = j;
    s.k        = k;
    s.snapshot = after;
    std::ostringstream oss;
    oss << "R" << (i + 1) << " += (" << k.toString() << ") * R" << (j + 1);
    s.description = oss.str();
    steps_.push_back(std::move(s));
}

void StepSequence::pushSelectPivot(std::size_t r, std::size_t c, const Matrix<Fraction>& after) {
    Step s;
    s.kind     = StepKind::SelectPivot;
    s.i        = r;
    s.j        = c;
    s.snapshot = after;
    std::ostringstream oss;
    oss << "选主元 (R" << (r + 1) << ", C" << (c + 1) << ")";
    s.description = oss.str();
    steps_.push_back(std::move(s));
}

void StepSequence::pushNote(std::string desc, const Matrix<Fraction>& after) {
    Step s;
    s.kind        = StepKind::Note;
    s.snapshot    = after;
    s.description = std::move(desc);
    steps_.push_back(std::move(s));
}

void StepSequence::pushConclude(std::string desc, const Matrix<Fraction>& after) {
    Step s;
    s.kind        = StepKind::Conclude;
    s.snapshot    = after;
    s.description = std::move(desc);
    steps_.push_back(std::move(s));
}

std::string StepSequence::toString() const {
    std::ostringstream out;
    for (std::size_t idx = 0; idx < steps_.size(); ++idx) {
        const Step& s = steps_[idx];
        out << "[" << (idx + 1) << "] " << s.description << "\n";
        out << s.snapshot.toString() << "\n\n";
    }
    return out.str();
}

}
