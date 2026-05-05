#include "VisualizePage.h"
#include "QuadricWidget.h"

#include "math/core/Fraction.h"
#include "math/core/Matrix.h"
#include "modules/calculator/interactive/expr/RenderSettings.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace algemate::math;
using AlgeMate::Calculator::Interactive::RenderTheme;

namespace AlgeMate::Calculator::Visualize {

// ── 17-class quadric classification table ──

static const QuadricClass kClasses[] = {
    { 0,  "椭球面",       "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}+\\frac{z^2}{c^2}=1",     3,0,0, "有界封闭卵形曲面",           "非退化"},
    { 2,  "单叶双曲面",   "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}-\\frac{z^2}{c^2}=1",      2,1,0, "无界直纹曲面，单连通",       "非退化"},
    { 3,  "双叶双曲面",   "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}-\\frac{z^2}{c^2}=-1",     1,2,0, "无界双连通曲面",             "非退化"},
    { 4,  "椭圆抛物面",   "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}=2pz",                      2,0,1, "开口抛物面",                 "非退化"},
    { 5,  "双曲抛物面",   "\\frac{x^2}{a^2}-\\frac{y^2}{b^2}=2pz",                      1,1,1, "马鞍形直纹面",               "非退化"},
    { 6,  "实二次锥面",   "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}-\\frac{z^2}{c^2}=0",      2,1,0, "顶点在原点的锥面",           "中心退化"},
    { 8,  "椭圆柱面",     "\\frac{x^2}{a^2}+\\frac{y^2}{b^2}=1",                        2,0,1, "柱面",                       "中心退化"},
    {10,  "双曲柱面",     "\\frac{x^2}{a^2}-\\frac{y^2}{b^2}=1",                        1,1,1, "柱面",                       "中心退化"},
    {11,  "抛物柱面",     "y^2=2px",                                                    1,0,2, "柱面",                       "抛物退化"},
};

static constexpr int kClassCount = sizeof(kClasses) / sizeof(kClasses[0]);

// ── Preset surface sampling functions ──

static void setPresetSurface(QuadricWidget* w, int id, double a, double b, double c, double p) {
    auto pi = float(M_PI);
    float fa = float(a), fb = float(b), fc = float(c), fp = float(p);

    switch (id) {
    case 0: // Ellipsoid
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D {
            return {fa * sinf(u) * cosf(v), fb * sinf(u) * sinf(v), fc * cosf(u)};
        }, 0, pi, 0, 2*pi, 72, 72);
        w->setColor({0.30f, 0.55f, 1.0f}); w->setAlpha(0.90f);
        break;
    case 2: // Hyperboloid of one sheet
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D {
            float ch = coshf(u), sh = sinhf(u);
            return {fa * ch * cosf(v), fb * ch * sinf(v), fc * sh};
        }, -1.2f, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.20f, 0.75f, 0.55f}); w->setAlpha(0.85f);
        break;
    case 3: // Hyperboloid of two sheets: upper (z>0) + lower (z<0)
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D {
            float sh = sinhf(u), ch = coshf(u);
            return {fa * sh * cosf(v), fb * sh * sinf(v), fc * ch};
        }, 0, 1.2f, 0, 2*pi, 80, 72);
        w->setSecondSurface([fa, fb, fc](float u, float v) -> QVector3D {
            float sh = sinhf(u), ch = coshf(u);
            return {fa * sh * cosf(v), fb * sh * sinf(v), -fc * ch};
        }, 0, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.85f, 0.40f, 0.25f}); w->setAlpha(0.85f);
        break;
    case 4: // Elliptic paraboloid
        w->setSurface([fa, fb, fp](float u, float v) -> QVector3D {
            float r = u;
            return {fa * r * cosf(v), fb * r * sinf(v), fp * r * r};
        }, 0, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.25f, 0.65f, 0.90f}); w->setAlpha(0.90f);
        break;
    case 5: // Hyperbolic paraboloid
        w->setSurface([fa, fb, fp](float u, float v) -> QVector3D {
            return {u, v, fp * (u * u / (fa * fa) - v * v / (fb * fb))};
        }, -2.0f, 2.0f, -2.0f, 2.0f, 72, 72);
        w->setColor({0.85f, 0.55f, 0.20f}); w->setAlpha(0.85f);
        break;
    case 6: // Real quadratic cone
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D {
            float t = u;
            return {fa * t * cosf(v), fb * t * sinf(v), fc * t};
        }, -2.0f, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.75f, 0.60f, 0.30f}); w->setAlpha(0.80f);
        break;
    case 8: // Elliptic cylinder
        w->setSurface([fa, fb](float u, float v) -> QVector3D {
            return {fa * cosf(u), fb * sinf(u), v};
        }, 0, 2*pi, -2.5f, 2.5f, 72, 40);
        w->setColor({0.40f, 0.70f, 0.80f}); w->setAlpha(0.85f);
        break;
    case 10: // Hyperbolic cylinder: right branch + left branch
        w->setSurface([fa, fb](float u, float v) -> QVector3D {
            return {fa * coshf(u), fb * sinhf(u), v};
        }, -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setSecondSurface([fa, fb](float u, float v) -> QVector3D {
            return {-fa * coshf(u), fb * sinhf(u), v};
        }, -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setColor({0.60f, 0.45f, 0.75f}); w->setAlpha(0.85f);
        break;
    case 11: // Parabolic cylinder
        w->setSurface([fp](float u, float v) -> QVector3D {
            return {v, u, fp * u * u};
        }, -2.0f, 2.0f, -2.0f, 2.0f, 72, 40);
        w->setColor({0.55f, 0.75f, 0.40f}); w->setAlpha(0.85f);
        break;
    default:
        w->setSurface([](float u, float v) -> QVector3D {
            return {0.3f * sinf(u) * cosf(v), 0.3f * sinf(u) * sinf(v), 0.3f * cosf(u)};
        }, 0, pi, 0, 2*pi, 32, 32);
        w->setColor({0.4f, 0.4f, 0.4f}); w->setAlpha(0.3f);
        break;
    }
    w->resetView();
}

// ── Classify a general quadric from 10 coefficients ──

static int classifyQuadric(const std::vector<double>& coeffs) {
    double a11 = coeffs[0], a22 = coeffs[1], a33 = coeffs[2];
    double a12 = coeffs[3], a13 = coeffs[4], a23 = coeffs[5];
    double a1  = coeffs[6], a2  = coeffs[7], a3  = coeffs[8];
    double a0  = coeffs[9];

    double A[3][3] = {
        {a11, a12, a13},
        {a12, a22, a23},
        {a13, a23, a33}
    };

    double d1 = A[0][0];
    double d2 = d1 * A[1][1] - A[0][1] * A[1][0];
    double d3 = d1 * (A[1][1]*A[2][2] - A[1][2]*A[2][1])
              - A[0][1] * (A[1][0]*A[2][2] - A[1][2]*A[2][0])
              + A[0][2] * (A[1][0]*A[2][1] - A[1][1]*A[2][0]);

    double detA = d3;
    double traceA = A[0][0] + A[1][1] + A[2][2];
    double m2 = A[0][0]*A[1][1] - A[0][1]*A[1][0]
              + A[0][0]*A[2][2] - A[0][2]*A[2][0]
              + A[1][1]*A[2][2] - A[1][2]*A[2][1];

    double T = traceA, M = m2, D = detA;
    double p_c = M - T*T/3.0;
    double q_c = -2.0*T*T*T/27.0 + T*M/3.0 - D;
    double disc = q_c*q_c/4.0 + p_c*p_c*p_c/27.0;

    double eigs[3];
    if (fabs(disc) < 1e-10) {
        double x = (fabs(q_c) < 1e-12) ? 0.0 :
                   -q_c > 0 ? cbrt(-q_c/2.0) : -cbrt(q_c/2.0);
        eigs[0] = eigs[1] = eigs[2] = x + T/3.0;
    } else if (disc < 0) {
        double rr = sqrt(-p_c*p_c*p_c/27.0);
        double theta = acos(-q_c/(2.0*rr));
        double m = 2.0 * cbrt(rr);
        eigs[0] = m * cos(theta/3.0) + T/3.0;
        eigs[1] = m * cos((theta + 2*M_PI)/3.0) + T/3.0;
        eigs[2] = m * cos((theta + 4*M_PI)/3.0) + T/3.0;
    } else {
        double sq = sqrt(disc);
        double u = cbrt(-q_c/2.0 + sq);
        double v = cbrt(-q_c/2.0 - sq);
        eigs[0] = u + v + T/3.0;
        eigs[1] = eigs[2] = -(u+v)/2.0 + T/3.0;
    }

    int pos = 0, neg = 0, zero = 0;
    for (int i = 0; i < 3; ++i) {
        if (fabs(eigs[i]) < 1e-8) zero++;
        else if (eigs[i] > 0) pos++;
        else neg++;
    }

    double Q[4][4] = {
        {a11, a12, a13, a1},
        {a12, a22, a23, a2},
        {a13, a23, a33, a3},
        {a1,  a2,  a3,  a0}
    };

    auto det4 = [&]() -> double {
        double d = 0;
        for (int j = 0; j < 4; ++j) {
            double minor[3][3];
            for (int r = 1; r < 4; ++r)
                for (int c = 0, kc = 0; c < 4; ++c) {
                    if (c == j) continue;
                    minor[r-1][kc] = Q[r][c]; kc++;
                }
            double m3 = minor[0][0]*(minor[1][1]*minor[2][2]-minor[1][2]*minor[2][1])
                      - minor[0][1]*(minor[1][0]*minor[2][2]-minor[1][2]*minor[2][0])
                      + minor[0][2]*(minor[1][0]*minor[2][1]-minor[1][1]*minor[2][0]);
            d += (j % 2 == 0 ? 1 : -1) * Q[0][j] * m3;
        }
        return d;
    };
    double detQ = det4();

    if (zero == 0) {
        if (pos == 3 && neg == 0) return (detQ < 0) ? 0 : 1;  // I4<0→实, I4>0→虚
        if (pos == 2 && neg == 1) return (detQ > 0) ? 2 : 3;  // I4>0→单叶, I4<0→双叶
        if (pos == 1 && neg == 2) return (detQ > 0) ? 2 : 3;  // 整体乘-1，I4符号同上
        if (pos == 2 && neg == 0) return 4;
        if (pos == 1 && neg == 1) return 5;
    }
    if (zero == 1) {
        if (pos == 2 && neg == 0) return (fabs(detQ) < 1e-8) ? 13 : 8;
        if (pos == 1 && neg == 1) return (fabs(detQ) < 1e-8) ? 6 : 10;
        if (pos == 0 && neg == 2) return 9;
    }
    if (zero == 2) {
        if (pos == 1 || neg == 1) return 11;
    }
    if (zero == 3) return 16;

    for (int i = 0; i < kClassCount; ++i)
        if (kClasses[i].p == pos && kClasses[i].q == neg && kClasses[i].r == zero)
            return i;
    return 0;
}

// ── Jacobi eigenvalue decomposition for 3×3 symmetric matrix ──

struct Eigen3 {
    double vals[3];
    QVector3D vecs[3]; // vecs[i] = eigenvector for vals[i]
};

static Eigen3 decomposeSym3(const double A[3][3]) {
    double S[3][3], V[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            S[i][j] = A[i][j];

    for (int iter = 0; iter < 100; ++iter) {
        int p = 0, q = 1;
        double mx = fabs(S[0][1]);
        if (fabs(S[0][2]) > mx) { p = 0; q = 2; mx = fabs(S[0][2]); }
        if (fabs(S[1][2]) > mx) { p = 1; q = 2; mx = fabs(S[1][2]); }
        if (mx < 1e-14) break;

        double tau = (S[q][q] - S[p][p]) / (2.0 * S[p][q]);
        double t = (tau >= 0 ? 1.0 : -1.0) / (fabs(tau) + sqrt(1.0 + tau * tau));
        double c = 1.0 / sqrt(1.0 + t * t);
        double s = t * c;

        double Spq = S[p][q];
        S[p][p] -= t * Spq;
        S[q][q] += t * Spq;
        S[p][q] = S[q][p] = 0;

        for (int r = 0; r < 3; ++r) {
            if (r == p || r == q) continue;
            double Srp = S[r][p], Srq = S[r][q];
            S[r][p] = S[p][r] = c * Srp - s * Srq;
            S[r][q] = S[q][r] = s * Srp + c * Srq;
        }
        for (int r = 0; r < 3; ++r) {
            double Vrp = V[r][p], Vrq = V[r][q];
            V[r][p] = c * Vrp - s * Vrq;
            V[r][q] = s * Vrp + c * Vrq;
        }
    }

    Eigen3 result;
    for (int i = 0; i < 3; ++i) {
        result.vals[i] = S[i][i];
        result.vecs[i] = QVector3D(V[0][i], V[1][i], V[2][i]).normalized();
    }
    if (QVector3D::dotProduct(QVector3D::crossProduct(result.vecs[0], result.vecs[1]),
                               result.vecs[2]) < 0)
        result.vecs[2] = -result.vecs[2];
    return result;
}

// ── Map classification ID → kClasses array index ──

static int classIdToIndex(int id) {
    for (int i = 0; i < kClassCount; ++i)
        if (kClasses[i].id == id) return i;
    return -1;
}

// ── Render quadric directly from 10 coefficients (no standard-form conversion) ──

static void renderDirectQuadric(QuadricWidget* w, const std::vector<double>& coeffs) {
    double a11=coeffs[0], a22=coeffs[1], a33=coeffs[2];
    double a12=coeffs[3], a13=coeffs[4], a23=coeffs[5];
    double a1 =coeffs[6], a2 =coeffs[7], a3 =coeffs[8], a0=coeffs[9];

    double A[3][3] = {{a11,a12,a13},{a12,a22,a23},{a13,a23,a33}};
    double bv[3] = {a1, a2, a3};

    Eigen3 eig = decomposeSym3(A);

    // d = P^T * b
    double d[3];
    for (int i = 0; i < 3; ++i)
        d[i] = QVector3D::dotProduct(eig.vecs[i], QVector3D(bv[0], bv[1], bv[2]));

    int cls = classifyQuadric(coeffs);

    // Count eigenvalue signs
    int posIdx[3], negIdx[3], zeroIdx[3];
    int posN=0, negN=0, zeroN=0;
    for (int i = 0; i < 3; ++i) {
        if (fabs(eig.vals[i]) < 1e-8) zeroIdx[zeroN++] = i;
        else if (eig.vals[i] > 0)     posIdx[posN++]   = i;
        else                          negIdx[negN++]   = i;
    }

    // Determine axis mapping: (idxX, idxY, idxZ) → which eigenvalue slot
    // maps to x, y, z in the standard-form parametric function
    int mx=0, my=1, mz=2; // default
    switch (cls) {
    case 2: case 3: case 6: // 2pos+1neg → axis=neg
        if (negN==1) { mx=posIdx[0]; my=posIdx[1]; mz=negIdx[0]; }
        else         { mx=negIdx[0]; my=negIdx[1]; mz=posIdx[0]; }
        break;
    case 5: // hyperbolic paraboloid: 1pos+1neg+1zero → axis=zero, x=pos, y=neg
        if (posN && negN && zeroN)
            { mx=posIdx[0]; my=negIdx[0]; mz=zeroIdx[0]; }
        break;
    case 4: case 8: // elliptic paraboloid / cylinder: 2pos+1zero → axis=zero
        if (zeroN) { mx=posIdx[0]; my=posIdx[1]; mz=zeroIdx[0]; }
        break;
    case 10: // hyperbolic cylinder: 1pos+1neg+1zero → axis=zero
        if (zeroN) { mx=posIdx[0]; my=negIdx[0]; mz=zeroIdx[0]; }
        break;
    case 11: // parabolic cylinder
        if (zeroN>=2) {
            mx = (posN ? posIdx[0] : negIdx[0]);
            my = zeroIdx[0]; mz = zeroIdx[1];
        }
        break;
    default: // ellipsoid etc.
        break;
    }

    // Rotation columns (remapped to standard-form order)
    QVector3D vx = eig.vecs[mx], vy = eig.vecs[my], vz = eig.vecs[mz];

    // Compute center
    double y0[3] = {0,0,0};
    for (int i = 0; i < 3; ++i)
        if (fabs(eig.vals[i]) > 1e-8) y0[i] = -d[i] / eig.vals[i];
    // For paraboloids, shift the zero-eigenvalue direction too
    if (zeroN == 1) {
        int zi = zeroIdx[0];
        double K = a0;
        for (int i = 0; i < 3; ++i)
            if (fabs(eig.vals[i]) > 1e-8) K += d[i]*d[i] / eig.vals[i];
        if (fabs(d[zi]) > 1e-10) y0[zi] = -K / (2.0 * d[zi]);
    }
    QVector3D center = eig.vecs[0]*y0[0] + eig.vecs[1]*y0[1] + eig.vecs[2]*y0[2];

    // Compute RHS = -k' (right-hand side of standard form)
    double kprime = a0;
    for (int i = 0; i < 3; ++i)
        if (fabs(eig.vals[i]) > 1e-8) kprime -= d[i]*d[i] / eig.vals[i];
    double RHS = -kprime;

    // Eigenvalues in the remapped order
    double lx = eig.vals[mx], ly = eig.vals[my], lz = eig.vals[mz];

    // Wrapper: transforms standard-form point z → original coordinates x = P*z + center
    auto makeTransformed = [vx, vy, vz, center](QuadricWidget::SurfaceFunc stdFn)
        -> QuadricWidget::SurfaceFunc {
        return [stdFn, vx, vy, vz, center](float u, float v) -> QVector3D {
            QVector3D z = stdFn(u, v);
            return vx*z.x() + vy*z.y() + vz*z.z() + center;
        };
    };

    auto pi = float(M_PI);

    // Build standard-form surface based on classification
    switch (cls) {
    case 0: { // Ellipsoid: all λ>0, RHS>0
        float fa = (float)sqrt(fabs(RHS / lx));
        float fb = (float)sqrt(fabs(RHS / ly));
        float fc = (float)sqrt(fabs(RHS / lz));
        w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{
            return {fa*sinf(u)*cosf(v), fb*sinf(u)*sinf(v), fc*cosf(u)};
        }), 0, pi, 0, 2*pi, 72, 72);
        w->setColor({0.30f,0.55f,1.0f}); w->setAlpha(0.90f);
        break;
    }
    case 2: { // One-sheet hyperboloid: λx,λy >0, λz<0, RHS>0
        float fa = (float)sqrt(fabs(RHS / lx));
        float fb = (float)sqrt(fabs(RHS / ly));
        float fc = (float)sqrt(fabs(RHS / fabs(lz)));
        w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{
            return {fa*coshf(u)*cosf(v), fb*coshf(u)*sinf(v), fc*sinhf(u)};
        }), -1.2f, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.20f,0.75f,0.55f}); w->setAlpha(0.85f);
        break;
    }
    case 3: { // Two-sheet hyperboloid: λx,λy >0, λz<0, RHS<0
        float fa = (float)sqrt(fabs(RHS / lx));
        float fb = (float)sqrt(fabs(RHS / ly));
        float fc = (float)sqrt(fabs(RHS / fabs(lz)));
        w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{
            return {fa*sinhf(u)*cosf(v), fb*sinhf(u)*sinf(v), fc*coshf(u)};
        }), 0, 1.2f, 0, 2*pi, 80, 72);
        w->setSecondSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{
            return {fa*sinhf(u)*cosf(v), fb*sinhf(u)*sinf(v), -fc*coshf(u)};
        }), 0, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.85f,0.40f,0.25f}); w->setAlpha(0.85f);
        break;
    }
    case 4: { // Elliptic paraboloid: λx,λy>0, λz≈0
        // Standard form: λx*zx² + λy*zy² = -2*dz*z3 (after centering)
        // → zx²/a² + zy²/b² = z3  with  a²=|−2dz/λx|,  b²=|−2dz/λy|
        float dz = (float)d[mz];
        float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx)));
        float fb = (float)sqrt(fabs(2.0*fabs(dz) / fabs(ly)));
        float sign = (dz < 0) ? 1.0f : -1.0f;
        w->setSurface(makeTransformed([fa,fb,sign](float u, float v)->QVector3D{
            float r = u;
            return {fa*r*cosf(v), fb*r*sinf(v), sign*r*r};
        }), 0, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.25f,0.65f,0.90f}); w->setAlpha(0.90f);
        break;
    }
    case 5: { // Hyperbolic paraboloid: λx>0, λy<0, λz≈0
        float dz = (float)d[mz];
        float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx)));
        float fb = (float)sqrt(fabs(2.0*fabs(dz) / fabs(ly)));
        float sign = (dz < 0) ? 1.0f : -1.0f;
        w->setSurface(makeTransformed([fa,fb,sign](float u, float v)->QVector3D{
            return {u, v, sign*(u*u/(fa*fa) - v*v/(fb*fb))};
        }), -2.0f, 2.0f, -2.0f, 2.0f, 72, 72);
        w->setColor({0.85f,0.55f,0.20f}); w->setAlpha(0.85f);
        break;
    }
    case 6: { // Real quadratic cone
        float fa = (float)sqrt(1.0 / fabs(lx));
        float fb = (float)sqrt(1.0 / fabs(ly));
        float fc = (float)sqrt(1.0 / fabs(lz));
        w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{
            float t = u;
            return {fa*t*cosf(v), fb*t*sinf(v), fc*t};
        }), -2.0f, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.75f,0.60f,0.30f}); w->setAlpha(0.80f);
        break;
    }
    case 8: { // Elliptic cylinder
        float fa = (float)sqrt(fabs(RHS / lx));
        float fb = (float)sqrt(fabs(RHS / ly));
        w->setSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{
            return {fa*cosf(u), fb*sinf(u), v};
        }), 0, 2*pi, -2.5f, 2.5f, 72, 40);
        w->setColor({0.40f,0.70f,0.80f}); w->setAlpha(0.85f);
        break;
    }
    case 10: { // Hyperbolic cylinder
        float fa = (float)sqrt(fabs(RHS / lx));
        float fb = (float)sqrt(fabs(RHS / fabs(ly)));
        w->setSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{
            return {fa*coshf(u), fb*sinhf(u), v};
        }), -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setSecondSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{
            return {-fa*coshf(u), fb*sinhf(u), v};
        }), -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setColor({0.60f,0.45f,0.75f}); w->setAlpha(0.85f);
        break;
    }
    case 11: { // Parabolic cylinder
        float dz = (float)d[mz];
        float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx)));
        w->setSurface(makeTransformed([fa](float u, float v)->QVector3D{
            return {v, u, fa*u*u};
        }), -2.0f, 2.0f, -2.0f, 2.0f, 72, 40);
        w->setColor({0.55f,0.75f,0.40f}); w->setAlpha(0.85f);
        break;
    }
    default: { // Unknown / unrenderable → placeholder sphere
        w->setSurface([](float u, float v)->QVector3D{
            return {0.3f*sinf(u)*cosf(v), 0.3f*sinf(u)*sinf(v), 0.3f*cosf(u)};
        }, 0, pi, 0, 2*pi, 32, 32);
        w->setColor({0.4f,0.4f,0.4f}); w->setAlpha(0.3f);
        break;
    }
    }
    w->resetView();
}

// ── Constructor ──

VisualizePage::VisualizePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // ── Left panel ──
    auto* leftPanel = new QWidget;
    leftPanel->setMinimumWidth(320);
    leftPanel->setMaximumWidth(400);
    auto* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);

    // Title bar
    auto* titleBar = new QWidget;
    titleBar->setFixedHeight(48);
    auto* titleLay = new QHBoxLayout(titleBar);
    titleLay->setContentsMargins(16, 0, 16, 0);
    auto* titleLbl = new QLabel(QStringLiteral("二次曲面分类与可视化"));
    titleLbl->setStyleSheet(QStringLiteral("font-size:16px; font-weight:700;"));
    titleLay->addWidget(titleLbl);
    titleLay->addStretch();
    leftLay->addWidget(titleBar);

    // Tab widget: 输入 / 预设
    auto* tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(createInputTab(), QStringLiteral("方程输入"));
    tabs->addTab(createPresetTab(), QStringLiteral("预设选择"));
    leftLay->addWidget(tabs, 1);

    // ── Right panel ──
    auto* rightPanel = new QWidget;
    auto* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);
    setupRenderArea(rightPanel);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    root->addWidget(splitter);

    // Initial render
    onPresetSelected(0);
}

// ── Create input tab ──

QWidget* VisualizePage::createInputTab() {
    auto* page = new QWidget;
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(16, 12, 16, 16);
    lay->setSpacing(10);

    auto* hint = new QLabel(QStringLiteral(
        "F(x,y,z) = a₁₁x² + a₂₂y² + a₃₃z²\n"
        "  + 2a₁₂xy + 2a₁₃xz + 2a₂₃yz\n"
        "  + 2a₁x + 2a₂y + 2a₃z + a₀ = 0"));
    hint->setStyleSheet(QStringLiteral("font-size:12px; color:#8A8FA3; padding:4px;"));
    lay->addWidget(hint);

    // Grid: label + line edit, 5 rows x 2 columns
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    const char* labels[] = {
        "a₁₁", "a₂₂", "a₃₃", "a₁₂", "a₁₃",
        "a₂₃", "a₁",  "a₂",  "a₃",  "a₀"
    };
    const char* defaults[] = {
        "1", "2", "-3", "0", "0", "0", "0", "0", "0", "-3"
    };

    auto lineEditStyle = QStringLiteral(
        "QLineEdit { background:#FFFFFF; border:1px solid #B0B0B0; border-radius:4px; "
        "padding:4px 8px; color:#333333; font-size:13px; } "
        "QLineEdit:focus { border-color:#4A90D9; }");

    for (int i = 0; i < 10; ++i) {
        int row = i / 2, col = (i % 2) * 2;

        auto* lbl = new QLabel(QString::fromUtf8(labels[i]));
        lbl->setStyleSheet(QStringLiteral("font-size:13px; font-weight:500;"));
        lbl->setFixedWidth(28);
        grid->addWidget(lbl, row, col);

        auto* edit = new QLineEdit(QString::fromUtf8(defaults[i]));
        edit->setStyleSheet(lineEditStyle);
        edit->setFixedWidth(90);
        coeffEdit_[i] = edit;
        grid->addWidget(edit, row, col + 1);
    }

    lay->addLayout(grid);
    lay->addSpacing(8);

    analyzeBtn_ = new QPushButton(QStringLiteral("分析并渲染"));
    analyzeBtn_->setCursor(Qt::PointingHandCursor);
    analyzeBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#4A90D9; color:white; border-radius:6px; "
        "padding:10px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#5BA0E9; }"));
    connect(analyzeBtn_, &QPushButton::clicked, this, &VisualizePage::onAnalyze);
    lay->addWidget(analyzeBtn_);

    lay->addStretch(1);
    scroll->setWidget(content);

    auto* wrapper = new QVBoxLayout(page);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(scroll);
    return page;
}

// ── Create preset tab ──

QWidget* VisualizePage::createPresetTab() {
    auto* page = new QWidget;
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget;
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(16, 12, 16, 16);
    lay->setSpacing(10);

    auto* comboLabel = new QLabel(QStringLiteral("选择曲面类型："));
    comboLabel->setStyleSheet(QStringLiteral("font-size:13px; font-weight:500;"));
    lay->addWidget(comboLabel);

    presetCombo_ = new QComboBox;
    for (int i = 0; i < kClassCount; ++i) {
        QString text = QString::fromUtf8(kClasses[i].name);
        if (i <= 5)       text += QStringLiteral("  [非退化]");
        else if (i <= 10) text += QStringLiteral("  [中心退化]");
        else              text += QStringLiteral("  [抛物退化]");
        presetCombo_->addItem(text);
    }
    presetCombo_->setCurrentIndex(0);
    presetCombo_->setStyleSheet(QStringLiteral(
        "QComboBox { background:#FFFFFF; border:1px solid #B0B0B0; border-radius:4px; "
        "padding:6px 10px; color:#333333; font-size:13px; } "
        "QComboBox::drop-down { border:none; } "
        "QComboBox QAbstractItemView { background:#FFFFFF; color:#333333; "
        "selection-background-color:#4A90D9; selection-color:white; }"));
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VisualizePage::onPresetSelected);
    lay->addWidget(presetCombo_);

    lay->addSpacing(8);

    // Parameters
    auto* paramLabel = new QLabel(QStringLiteral("参数调节："));
    paramLabel->setStyleSheet(QStringLiteral("font-size:13px; font-weight:500;"));
    lay->addWidget(paramLabel);

    auto* paramGrid = new QGridLayout;
    paramGrid->setHorizontalSpacing(8);
    paramGrid->setVerticalSpacing(6);

    auto spinStyle = QStringLiteral(
        "QDoubleSpinBox { background:#FFFFFF; border:1px solid #B0B0B0; border-radius:4px; "
        "padding:4px 8px; color:#333333; font-size:13px; } "
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:16px; }");

    auto makeParam = [&](const QString& name, double val, double min, double max) -> QPair<QLabel*, QDoubleSpinBox*> {
        auto* lbl = new QLabel(name);
        lbl->setStyleSheet(QStringLiteral("font-size:13px;"));
        lbl->setFixedWidth(20);
        auto* spin = new QDoubleSpinBox;
        spin->setRange(min, max);
        spin->setDecimals(2);
        spin->setSingleStep(0.1);
        spin->setValue(val);
        spin->setFixedWidth(90);
        spin->setStyleSheet(spinStyle);
        return {lbl, spin};
    };

    auto [la, sa] = makeParam(QStringLiteral("a"), 2.0, 0.1, 10.0);
    auto [lb, sb] = makeParam(QStringLiteral("b"), 3.0, 0.1, 10.0);
    auto [lc, sc] = makeParam(QStringLiteral("c"), 1.0, 0.1, 10.0);
    auto [lp, sp] = makeParam(QStringLiteral("p"), 1.0, 0.1, 10.0);

    paramGrid->addWidget(la, 0, 0); paramGrid->addWidget(sa, 0, 1);
    paramGrid->addWidget(lb, 1, 0); paramGrid->addWidget(sb, 1, 1);
    paramGrid->addWidget(lc, 2, 0); paramGrid->addWidget(sc, 2, 1);
    paramGrid->addWidget(lp, 3, 0); paramGrid->addWidget(sp, 3, 1);

    paramA_ = sa; paramB_ = sb; paramC_ = sc; paramP_ = sp;
    lay->addLayout(paramGrid);

    lay->addSpacing(8);

    presetBtn_ = new QPushButton(QStringLiteral("渲染预设曲面"));
    presetBtn_->setCursor(Qt::PointingHandCursor);
    presetBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:#2196F3; color:white; border-radius:6px; "
        "padding:10px 16px; font-size:14px; font-weight:600; } "
        "QPushButton:hover { background:#1976D2; }"));
    connect(presetBtn_, &QPushButton::clicked, this, [this]{
        onPresetSelected(presetCombo_->currentIndex());
    });
    lay->addWidget(presetBtn_);

    lay->addStretch(1);
    scroll->setWidget(content);

    auto* wrapper = new QVBoxLayout(page);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(scroll);
    return page;
}

// ── Setup render area ──

void VisualizePage::setupRenderArea(QWidget* parent) {
    auto* lay = qobject_cast<QVBoxLayout*>(parent->layout());

    // Info panel on top
    infoBrowser_ = new QTextBrowser;
    infoBrowser_->setOpenLinks(false);
    infoBrowser_->setMaximumHeight(100);
    infoBrowser_->setStyleSheet(QStringLiteral(
        "QTextBrowser { border:none; border-bottom:1px solid #3A3D4A; "
        "padding:8px 12px; background:#1A1B22; }"));
    lay->addWidget(infoBrowser_);

    // 3D viewport
    quadric_ = new QuadricWidget;
    lay->addWidget(quadric_, 1);

    // Reset view button
    resetViewBtn_ = new QPushButton(QStringLiteral("重置视角"));
    resetViewBtn_->setCursor(Qt::PointingHandCursor);
    resetViewBtn_->setStyleSheet(QStringLiteral(
        "QPushButton { background:rgba(0,0,0,0.5); color:#ccc; border:1px solid #555; "
        "border-radius:4px; padding:4px 12px; font-size:12px; } "
        "QPushButton:hover { background:rgba(60,60,80,0.7); }"));
    connect(resetViewBtn_, &QPushButton::clicked, this, &VisualizePage::onResetView);

    auto* overlayLayout = new QHBoxLayout;
    overlayLayout->addStretch();
    overlayLayout->addWidget(resetViewBtn_);
    overlayLayout->setContentsMargins(0, 0, 8, 8);

    auto* renderContainer = new QWidget;
    auto* renderLay = new QVBoxLayout(renderContainer);
    renderLay->setContentsMargins(0, 0, 0, 0);
    renderLay->setSpacing(0);
    renderLay->addWidget(quadric_, 1);
    renderLay->addLayout(overlayLayout);

    lay->addWidget(renderContainer, 1);
}

// ── Analyze custom input ──

void VisualizePage::onAnalyze() {
    std::vector<double> coeffs(10);
    for (int i = 0; i < 10; ++i)
        coeffs[i] = coeffEdit_[i]->text().toDouble();

    renderDirectQuadric(quadric_, coeffs);

    int cls = classifyQuadric(coeffs);
    int idx = classIdToIndex(cls);
    if (idx >= 0) updateInfo(idx);
}

// ── Preset selected ──

void VisualizePage::onPresetSelected(int index) {
    if (index < 0 || index >= kClassCount) return;

    int id = kClasses[index].id;
    double a = paramA_->value();
    double b = paramB_->value();
    double c = paramC_->value();
    double p = paramP_->value();

    renderPreset(id, a, b, c, p);
    updateInfo(index);
}

void VisualizePage::renderPreset(int id, double a, double b, double c, double p) {
    setPresetSurface(quadric_, id, a, b, c, p);
}

// ── Update info panel ──

void VisualizePage::updateInfo(int cls) {
    auto th = RenderTheme::forCurrent();
    auto* doc = infoBrowser_->document(); doc->clear();

    QString html = QStringLiteral(
        "<div style='padding:2px; line-height:1.8;'>"
        "<b style='font-size:15px; color:%1;'>%2</b>　"
        "<span style='color:#8A8FA3; font-size:12px;'>%3</span><br>"
        "<span style='font-size:13px;'>标准方程: </span>"
        "<span style='font-size:14px; color:%1;'>$%4$</span>　"
        "<span style='font-size:13px;'>惯性指数 (p,q,r) = (%5, %6, %7)</span>　"
        "<span style='font-size:13px;'>%8</span>"
        "</div>")
        .arg(th.text,
             QString::fromUtf8(kClasses[cls].name),
             QString::fromUtf8(kClasses[cls].cat),
             QString::fromUtf8(kClasses[cls].stdEq))
        .arg(kClasses[cls].p)
        .arg(kClasses[cls].q)
        .arg(kClasses[cls].r)
        .arg(QString::fromUtf8(kClasses[cls].geo));

    infoBrowser_->setHtml(html);
}

// ── Reset view ──

void VisualizePage::onResetView() {
    if (quadric_) quadric_->resetView();
}

} // namespace AlgeMate::Calculator::Visualize
