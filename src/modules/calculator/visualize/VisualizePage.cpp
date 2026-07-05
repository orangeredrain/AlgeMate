#include "VisualizePage.h"
#include "QuadricWidget.h"
#include "core/ThemeManager.h" 
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

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

static const QuadricClass kClasses[] = {
    { 0,  "椭球面",       "x²/a² + y²/b² + z²/c² = 1",     3,0,0, "有界封闭卵形曲面",           "非退化"},
    { 2,  "单叶双曲面",   "x²/a² + y²/b² - z²/c² = 1",      2,1,0, "无界直纹曲面，单连通",       "非退化"},
    { 3,  "双叶双曲面",   "x²/a² + y²/b² - z²/c² = -1",     1,2,0, "无界双连通曲面",             "非退化"},
    { 4,  "椭圆抛物面",   "x²/a² + y²/b² = 2pz",            2,0,1, "开口抛物面",                 "非退化"},
    { 5,  "双曲抛物面",   "x²/a² - y²/b² = 2pz",            1,1,1, "马鞍形直纹面",               "非退化"},
    { 6,  "实二次锥面",   "x²/a² + y²/b² - z²/c² = 0",      2,1,0, "顶点在原点的锥面",           "中心退化"},
    { 8,  "椭圆柱面",     "x²/a² + y²/b² = 1",              2,0,1, "柱面",                       "中心退化"},
    {10,  "双曲柱面",     "x²/a² - y²/b² = 1",              1,1,1, "柱面",                       "中心退化"},
    {11,  "抛物柱面",     "y² = 2px",                       1,0,2, "柱面",                       "抛物退化"},
    };
static constexpr int kClassCount = sizeof(kClasses) / sizeof(kClasses[0]);

static void setPresetSurface(QuadricWidget* w, int id, double a, double b, double c, double p) {
    auto pi = float(M_PI);
    float fa = float(a), fb = float(b), fc = float(c), fp = float(p);
    switch (id) {
    case 0:
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D { return {fa * sinf(u) * cosf(v), fb * sinf(u) * sinf(v), fc * cosf(u)}; }, 0, pi, 0, 2*pi, 72, 72);
        w->setColor({0.30f, 0.55f, 1.0f}); w->setAlpha(0.90f); break;
    case 2:
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D { float ch = coshf(u), sh = sinhf(u); return {fa * ch * cosf(v), fb * ch * sinf(v), fc * sh}; }, -1.2f, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.20f, 0.75f, 0.55f}); w->setAlpha(0.85f); break;
    case 3:
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D { float sh = sinhf(u), ch = coshf(u); return {fa * sh * cosf(v), fb * sh * sinf(v), fc * ch}; }, 0, 1.2f, 0, 2*pi, 80, 72);
        w->setSecondSurface([fa, fb, fc](float u, float v) -> QVector3D { float sh = sinhf(u), ch = coshf(u); return {fa * sh * cosf(v), fb * sh * sinf(v), -fc * ch}; }, 0, 1.2f, 0, 2*pi, 80, 72);
        w->setColor({0.85f, 0.40f, 0.25f}); w->setAlpha(0.85f); break;
    case 4:
        w->setSurface([fa, fb, fp](float u, float v) -> QVector3D { float r = u; return {fa * r * cosf(v), fb * r * sinf(v), fp * r * r}; }, 0, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.25f, 0.65f, 0.90f}); w->setAlpha(0.90f); break;
    case 5:
        w->setSurface([fa, fb, fp](float u, float v) -> QVector3D { return {u, v, fp * (u * u / (fa * fa) - v * v / (fb * fb))}; }, -2.0f, 2.0f, -2.0f, 2.0f, 72, 72);
        w->setColor({0.85f, 0.55f, 0.20f}); w->setAlpha(0.85f); break;
    case 6:
        w->setSurface([fa, fb, fc](float u, float v) -> QVector3D { float t = u; return {fa * t * cosf(v), fb * t * sinf(v), fc * t}; }, -2.0f, 2.0f, 0, 2*pi, 72, 72);
        w->setColor({0.75f, 0.60f, 0.30f}); w->setAlpha(0.80f); break;
    case 8:
        w->setSurface([fa, fb](float u, float v) -> QVector3D { return {fa * cosf(u), fb * sinf(u), v}; }, 0, 2*pi, -2.5f, 2.5f, 72, 40);
        w->setColor({0.40f, 0.70f, 0.80f}); w->setAlpha(0.85f); break;
    case 10:
        w->setSurface([fa, fb](float u, float v) -> QVector3D { return {fa * coshf(u), fb * sinhf(u), v}; }, -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setSecondSurface([fa, fb](float u, float v) -> QVector3D { return {-fa * coshf(u), fb * sinhf(u), v}; }, -1.5f, 1.5f, -2.5f, 2.5f, 72, 40);
        w->setColor({0.60f, 0.45f, 0.75f}); w->setAlpha(0.85f); break;
    case 11:
        w->setSurface([fp](float u, float v) -> QVector3D { return {v, u, fp * u * u}; }, -2.0f, 2.0f, -2.0f, 2.0f, 72, 40);
        w->setColor({0.55f, 0.75f, 0.40f}); w->setAlpha(0.85f); break;
    default:
        w->setSurface([](float u, float v) -> QVector3D { return {0.3f * sinf(u) * cosf(v), 0.3f * sinf(u) * sinf(v), 0.3f * cosf(u)}; }, 0, pi, 0, 2*pi, 32, 32);
        w->setColor({0.4f, 0.4f, 0.4f}); w->setAlpha(0.3f); break;
    }
    w->resetView();
}

static int classifyQuadric(const std::vector<double>& coeffs) {
    double a11=coeffs[0], a22=coeffs[1], a33=coeffs[2], a12=coeffs[3], a13=coeffs[4], a23=coeffs[5], a1=coeffs[6], a2=coeffs[7], a3=coeffs[8], a0=coeffs[9];
    double A[3][3] = {{a11, a12, a13}, {a12, a22, a23}, {a13, a23, a33}};
    double d1 = A[0][0];
    double d3 = d1 * (A[1][1]*A[2][2] - A[1][2]*A[2][1]) - A[0][1] * (A[1][0]*A[2][2] - A[1][2]*A[2][0]) + A[0][2] * (A[1][0]*A[2][1] - A[1][1]*A[2][0]);
    double detA = d3, traceA = A[0][0] + A[1][1] + A[2][2];
    double m2 = A[0][0]*A[1][1] - A[0][1]*A[1][0] + A[0][0]*A[2][2] - A[0][2]*A[2][0] + A[1][1]*A[2][2] - A[1][2]*A[2][1];
    double T = traceA, M = m2, D = detA;
    double p_c = M - T*T/3.0, q_c = -2.0*T*T*T/27.0 + T*M/3.0 - D, disc = q_c*q_c/4.0 + p_c*p_c*p_c/27.0;
    double eigs[3];
    if (fabs(disc) < 1e-10) { double x = (fabs(q_c) < 1e-12) ? 0.0 : -q_c > 0 ? cbrt(-q_c/2.0) : -cbrt(q_c/2.0); eigs[0] = eigs[1] = eigs[2] = x + T/3.0; }
    else if (disc < 0) { double rr = sqrt(-p_c*p_c*p_c/27.0); double theta = acos(-q_c/(2.0*rr)); double m = 2.0 * cbrt(rr); eigs[0] = m * cos(theta/3.0) + T/3.0; eigs[1] = m * cos((theta + 2*M_PI)/3.0) + T/3.0; eigs[2] = m * cos((theta + 4*M_PI)/3.0) + T/3.0; }
    else { double sq = sqrt(disc); double u = cbrt(-q_c/2.0 + sq); double v = cbrt(-q_c/2.0 - sq); eigs[0] = u + v + T/3.0; eigs[1] = eigs[2] = -(u+v)/2.0 + T/3.0; }
    int pos = 0, neg = 0, zero = 0;
    for (int i = 0; i < 3; ++i) { if (fabs(eigs[i]) < 1e-8) zero++; else if (eigs[i] > 0) pos++; else neg++; }
    double Q[4][4] = {{a11, a12, a13, a1}, {a12, a22, a23, a2}, {a13, a23, a33, a3}, {a1, a2, a3, a0}};
    auto det4 = [&]() -> double {
        double d = 0;
        for (int j = 0; j < 4; ++j) {
            double minor[3][3];
            for (int r = 1; r < 4; ++r) for (int c = 0, kc = 0; c < 4; ++c) { if (c == j) continue; minor[r-1][kc] = Q[r][c]; kc++; }
            double m3 = minor[0][0]*(minor[1][1]*minor[2][2]-minor[1][2]*minor[2][1]) - minor[0][1]*(minor[1][0]*minor[2][2]-minor[1][2]*minor[2][0]) + minor[0][2]*(minor[1][0]*minor[2][1]-minor[1][1]*minor[2][0]);
            d += (j % 2 == 0 ? 1 : -1) * Q[0][j] * m3;
        }
        return d;
    };
    double detQ = det4();
    if (zero == 0) {
        if (pos == 3 && neg == 0) return (detQ < 0) ? 0 : 1;
        if (pos == 2 && neg == 1) return (detQ > 0) ? 2 : 3;
        if (pos == 1 && neg == 2) return (detQ > 0) ? 2 : 3;
        if (pos == 2 && neg == 0) return 4;
        if (pos == 1 && neg == 1) return 5;
    }
    if (zero == 1) {
        if (pos == 2 && neg == 0) return (fabs(detQ) < 1e-8) ? 13 : 8;
        if (pos == 1 && neg == 1) return (fabs(detQ) < 1e-8) ? 6 : 10;
        if (pos == 0 && neg == 2) return 9;
    }
    if (zero == 2) { if (pos == 1 || neg == 1) return 11; }
    if (zero == 3) return 16;
    for (int i = 0; i < kClassCount; ++i) if (kClasses[i].p == pos && kClasses[i].q == neg && kClasses[i].r == zero) return i;
    return 0;
}

struct Eigen3 { double vals[3]; QVector3D vecs[3]; };
static Eigen3 decomposeSym3(const double A[3][3]) {
    double S[3][3], V[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) S[i][j] = A[i][j];
    for (int iter = 0; iter < 100; ++iter) {
        int p = 0, q = 1; double mx = fabs(S[0][1]);
        if (fabs(S[0][2]) > mx) { p = 0; q = 2; mx = fabs(S[0][2]); }
        if (fabs(S[1][2]) > mx) { p = 1; q = 2; mx = fabs(S[1][2]); }
        if (mx < 1e-14) break;
        double tau = (S[q][q] - S[p][p]) / (2.0 * S[p][q]);
        double t = (tau >= 0 ? 1.0 : -1.0) / (fabs(tau) + sqrt(1.0 + tau * tau));
        double c = 1.0 / sqrt(1.0 + t * t); double s = t * c;
        double Spq = S[p][q]; S[p][p] -= t * Spq; S[q][q] += t * Spq; S[p][q] = S[q][p] = 0;
        for (int r = 0; r < 3; ++r) { if (r == p || r == q) continue; double Srp = S[r][p], Srq = S[r][q]; S[r][p] = S[p][r] = c * Srp - s * Srq; S[r][q] = S[q][r] = s * Srp + c * Srq; }
        for (int r = 0; r < 3; ++r) { double Vrp = V[r][p], Vrq = V[r][q]; V[r][p] = c * Vrp - s * Vrq; V[r][q] = s * Vrp + c * Vrq; }
    }
    Eigen3 result;
    for (int i = 0; i < 3; ++i) { result.vals[i] = S[i][i]; result.vecs[i] = QVector3D(V[0][i], V[1][i], V[2][i]).normalized(); }
    if (QVector3D::dotProduct(QVector3D::crossProduct(result.vecs[0], result.vecs[1]), result.vecs[2]) < 0) result.vecs[2] = -result.vecs[2];
    return result;
}

static int classIdToIndex(int id) {
    for (int i = 0; i < kClassCount; ++i) if (kClasses[i].id == id) return i;
    return -1;
}

static void renderDirectQuadric(QuadricWidget* w, const std::vector<double>& coeffs) {
    double a11=coeffs[0], a22=coeffs[1], a33=coeffs[2], a12=coeffs[3], a13=coeffs[4], a23=coeffs[5], a1=coeffs[6], a2=coeffs[7], a3=coeffs[8], a0=coeffs[9];
    double A[3][3] = {{a11,a12,a13},{a12,a22,a23},{a13,a23,a33}};
    double bv[3] = {a1, a2, a3};
    Eigen3 eig = decomposeSym3(A);
    double d[3]; for (int i = 0; i < 3; ++i) d[i] = QVector3D::dotProduct(eig.vecs[i], QVector3D(bv[0], bv[1], bv[2]));
    int cls = classifyQuadric(coeffs);
    int posIdx[3], negIdx[3], zeroIdx[3]; int posN=0, negN=0, zeroN=0;
    for (int i = 0; i < 3; ++i) { if (fabs(eig.vals[i]) < 1e-8) zeroIdx[zeroN++] = i; else if (eig.vals[i] > 0) posIdx[posN++] = i; else negIdx[negN++] = i; }
    int mx=0, my=1, mz=2;
    switch (cls) {
    case 2: case 3: case 6: if (negN==1) { mx=posIdx[0]; my=posIdx[1]; mz=negIdx[0]; } else { mx=negIdx[0]; my=negIdx[1]; mz=posIdx[0]; } break;
    case 5: if (posN && negN && zeroN) { mx=posIdx[0]; my=negIdx[0]; mz=zeroIdx[0]; } break;
    case 4: case 8: if (zeroN) { mx=posIdx[0]; my=posIdx[1]; mz=zeroIdx[0]; } break;
    case 10: if (zeroN) { mx=posIdx[0]; my=negIdx[0]; mz=zeroIdx[0]; } break;
    case 11: if (zeroN>=2) { mx = (posN ? posIdx[0] : negIdx[0]); my = zeroIdx[0]; mz = zeroIdx[1]; } break;
    }
    QVector3D vx = eig.vecs[mx], vy = eig.vecs[my], vz = eig.vecs[mz];
    double y0[3] = {0,0,0};
    for (int i = 0; i < 3; ++i) if (fabs(eig.vals[i]) > 1e-8) y0[i] = -d[i] / eig.vals[i];
    if (zeroN == 1) { int zi = zeroIdx[0]; double K = a0; for (int i = 0; i < 3; ++i) if (fabs(eig.vals[i]) > 1e-8) K += d[i]*d[i] / eig.vals[i]; if (fabs(d[zi]) > 1e-10) y0[zi] = -K / (2.0 * d[zi]); }
    QVector3D center = eig.vecs[0]*y0[0] + eig.vecs[1]*y0[1] + eig.vecs[2]*y0[2];
    double kprime = a0; for (int i = 0; i < 3; ++i) if (fabs(eig.vals[i]) > 1e-8) kprime -= d[i]*d[i] / eig.vals[i];
    double RHS = -kprime; double lx = eig.vals[mx], ly = eig.vals[my], lz = eig.vals[mz];
    auto makeTransformed = [vx, vy, vz, center](QuadricWidget::SurfaceFunc stdFn) -> QuadricWidget::SurfaceFunc { return [stdFn, vx, vy, vz, center](float u, float v) -> QVector3D { QVector3D z = stdFn(u, v); return vx*z.x() + vy*z.y() + vz*z.z() + center; }; };
    auto pi = float(M_PI);
    switch (cls) {
    case 0: { float fa = (float)sqrt(fabs(RHS / lx)); float fb = (float)sqrt(fabs(RHS / ly)); float fc = (float)sqrt(fabs(RHS / lz)); w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{ return {fa*sinf(u)*cosf(v), fb*sinf(u)*sinf(v), fc*cosf(u)}; }), 0, pi, 0, 2*pi, 72, 72); w->setColor({0.30f,0.55f,1.0f}); w->setAlpha(0.90f); break; }
    case 2: { float fa = (float)sqrt(fabs(RHS / lx)); float fb = (float)sqrt(fabs(RHS / ly)); float fc = (float)sqrt(fabs(RHS / fabs(lz))); w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{ return {fa*coshf(u)*cosf(v), fb*coshf(u)*sinf(v), fc*sinhf(u)}; }), -1.2f, 1.2f, 0, 2*pi, 80, 72); w->setColor({0.20f,0.75f,0.55f}); w->setAlpha(0.85f); break; }
    case 3: { float fa = (float)sqrt(fabs(RHS / lx)); float fb = (float)sqrt(fabs(RHS / ly)); float fc = (float)sqrt(fabs(RHS / fabs(lz))); w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{ return {fa*sinhf(u)*cosf(v), fb*sinhf(u)*sinf(v), fc*coshf(u)}; }), 0, 1.2f, 0, 2*pi, 80, 72); w->setSecondSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{ return {fa*sinhf(u)*cosf(v), fb*sinhf(u)*sinf(v), -fc*coshf(u)}; }), 0, 1.2f, 0, 2*pi, 80, 72); w->setColor({0.85f,0.40f,0.25f}); w->setAlpha(0.85f); break; }
    case 4: { float dz = (float)d[mz]; float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx))); float fb = (float)sqrt(fabs(2.0*fabs(dz) / fabs(ly))); float sign = (dz < 0) ? 1.0f : -1.0f; w->setSurface(makeTransformed([fa,fb,sign](float u, float v)->QVector3D{ float r = u; return {fa*r*cosf(v), fb*r*sinf(v), sign*r*r}; }), 0, 2.0f, 0, 2*pi, 72, 72); w->setColor({0.25f,0.65f,0.90f}); w->setAlpha(0.90f); break; }
    case 5: { float dz = (float)d[mz]; float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx))); float fb = (float)sqrt(fabs(2.0*fabs(dz) / fabs(ly))); float sign = (dz < 0) ? 1.0f : -1.0f; w->setSurface(makeTransformed([fa,fb,sign](float u, float v)->QVector3D{ return {u, v, sign*(u*u/(fa*fa) - v*v/(fb*fb))}; }), -2.0f, 2.0f, -2.0f, 2.0f, 72, 72); w->setColor({0.85f,0.55f,0.20f}); w->setAlpha(0.85f); break; }
    case 6: { float fa = (float)sqrt(1.0 / fabs(lx)); float fb = (float)sqrt(1.0 / fabs(ly)); float fc = (float)sqrt(1.0 / fabs(lz)); w->setSurface(makeTransformed([fa,fb,fc](float u, float v)->QVector3D{ float t = u; return {fa*t*cosf(v), fb*t*sinf(v), fc*t}; }), -2.0f, 2.0f, 0, 2*pi, 72, 72); w->setColor({0.75f,0.60f,0.30f}); w->setAlpha(0.80f); break; }
    case 8: { float fa = (float)sqrt(fabs(RHS / lx)); float fb = (float)sqrt(fabs(RHS / ly)); w->setSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{ return {fa*cosf(u), fb*sinf(u), v}; }), 0, 2*pi, -2.5f, 2.5f, 72, 40); w->setColor({0.40f,0.70f,0.80f}); w->setAlpha(0.85f); break; }
    case 10:{ float fa = (float)sqrt(fabs(RHS / lx)); float fb = (float)sqrt(fabs(RHS / fabs(ly))); w->setSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{ return {fa*coshf(u), fb*sinhf(u), v}; }), -1.5f, 1.5f, -2.5f, 2.5f, 72, 40); w->setSecondSurface(makeTransformed([fa,fb](float u, float v)->QVector3D{ return {-fa*coshf(u), fb*sinhf(u), v}; }), -1.5f, 1.5f, -2.5f, 2.5f, 72, 40); w->setColor({0.60f,0.45f,0.75f}); w->setAlpha(0.85f); break; }
    case 11:{ float dz = (float)d[mz]; float fa = (float)sqrt(fabs(2.0*fabs(dz) / fabs(lx))); w->setSurface(makeTransformed([fa](float u, float v)->QVector3D{ return {v, u, fa*u*u}; }), -2.0f, 2.0f, -2.0f, 2.0f, 72, 40); w->setColor({0.55f,0.75f,0.40f}); w->setAlpha(0.85f); break; }
    default:{ w->setSurface([](float u, float v)->QVector3D{ return {0.3f*sinf(u)*cosf(v), 0.3f*sinf(u)*sinf(v), 0.3f*cosf(u)}; }, 0, pi, 0, 2*pi, 32, 32); w->setColor({0.4f,0.4f,0.4f}); w->setAlpha(0.3f); break; }
    }
    w->resetView();
}

VisualizePage::VisualizePage(QWidget* parent) : QWidget(parent) {
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal);

    auto* leftPanel = new QWidget;
    leftPanel->setMinimumWidth(320);
    leftPanel->setMaximumWidth(400);

    auto* leftLay = new QVBoxLayout(leftPanel);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(0);

    auto* titleBar = new QWidget;
    titleBar->setFixedHeight(56);
    auto* titleLay = new QHBoxLayout(titleBar);
    titleLay->setContentsMargins(20, 0, 16, 0);
    auto* titleLbl = new QLabel(QStringLiteral("二次曲面分类与可视化"));
    titleLay->addWidget(titleLbl);
    titleLay->addStretch();
    leftLay->addWidget(titleBar);

    auto* tabs = new QTabWidget;
    tabs->setDocumentMode(true);
    tabs->addTab(createInputTab(), QStringLiteral("方程输入"));
    tabs->addTab(createPresetTab(), QStringLiteral("预设选择"));
    leftLay->addWidget(tabs, 1);

    auto* rightPanel = new QWidget;
    auto* rightLay = new QVBoxLayout(rightPanel);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->setSpacing(0);
    setupRenderArea(rightPanel);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setHandleWidth(1);

    root->addWidget(splitter);

    auto applyTheme = [this, titleLbl, tabs]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;

        titleLbl->setStyleSheet(isDark ? "color: #E6E7F0; font-size: 18px; font-weight: bold;" : "color: #111827; font-size: 18px; font-weight: bold;");
        QString tabQss = isDark ?
                             // 暗色模式：
                             "QTabWidget { background: transparent; }" // 整个容器背景透明
                             "QTabWidget::pane { border-top: 1px solid #3B395A; background: transparent; }" // 面板背景透明
                             "QTabBar { background: transparent; }" // 标签栏背景透明
                             "QTabBar::tab { background: transparent; color: #7B7B96; padding: 10px 20px; font-weight: bold; font-size: 14px; border-bottom: 2px solid transparent; }"
                             "QTabBar::tab:selected { color: #8FA1FF; border-bottom: 2px solid #8FA1FF; }"
                             "QTabBar::tab:hover:!selected { color: #E6E7F0; }"
                                :
                             // 亮色模式：
                             "QTabWidget { background: transparent; }"
                             "QTabWidget::pane { border-top: 1px solid #E2E8F0; background: transparent; }"
                             "QTabBar { background: transparent; }"
                             "QTabBar::tab { background: transparent; color: #64748B; padding: 10px 20px; font-weight: bold; font-size: 14px; border-bottom: 2px solid transparent; }"
                             "QTabBar::tab:selected { color: #4F46E5; border-bottom: 2px solid #4F46E5; }"
                             "QTabBar::tab:hover:!selected { color: #1E293B; }";

        tabs->setStyleSheet(tabQss);

        QString inputQss = isDark ?
                               "QLineEdit, QDoubleSpinBox, QComboBox { background: #1C1B2E; border: 1px solid #3B395A; border-radius: 6px; padding: 6px 10px; color: #E6E7F0; font-size: 13px; } QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #8FA1FF; background: #28263F; } QComboBox::drop-down { border: none; width: 24px; } QComboBox QAbstractItemView { background: #28263F; color: #E6E7F0; selection-background-color: #312F4A; border: 1px solid #3B395A; }"
                                  :
                               "QLineEdit, QDoubleSpinBox, QComboBox { background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; color: #1E293B; font-size: 13px; } QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #4F46E5; background: #FFFFFF; } QComboBox::drop-down { border: none; width: 24px; } QComboBox QAbstractItemView { background: #FFFFFF; color: #1E293B; selection-background-color: #EEF2F6; border: 1px solid #CBD5E1; }";

        for(int i=0; i<10; ++i) if(coeffEdit_[i]) coeffEdit_[i]->setStyleSheet(inputQss);
        if(presetCombo_) presetCombo_->setStyleSheet(inputQss);
        if(paramA_) { paramA_->setStyleSheet(inputQss); paramB_->setStyleSheet(inputQss); paramC_->setStyleSheet(inputQss); paramP_->setStyleSheet(inputQss); }

        QString primaryBtn = isDark ?
                                 "QPushButton { background: #5046E5; color: white; border: none; border-radius: 8px; padding: 10px 16px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #6366F1; } QPushButton:pressed { background: #4338CA; }"
                                    :
                                 "QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; padding: 10px 16px; font-size: 14px; font-weight: bold; } QPushButton:hover { background: #6366F1; } QPushButton:pressed { background: #4338CA; }";
        if(analyzeBtn_) analyzeBtn_->setStyleSheet(primaryBtn);
        if(presetBtn_) presetBtn_->setStyleSheet(primaryBtn);

        if(infoBrowser_) {
            infoBrowser_->setStyleSheet(isDark ?
                                            "QTextBrowser { border:none; border-bottom:1px solid #3B395A; padding:12px; background:transparent; color: #FFFFFF; }"
                                               :
                                            "QTextBrowser { border:none; border-bottom:1px solid #E2E8F0; padding:12px; background:transparent; color: #1E293B; }");
        }
        if (tabs->currentIndex() == 0) {
            // 如果用户停留在“方程输入”页面
            std::vector<double> coeffs(10);
            for (int i = 0; i < 10; ++i) {
                if (coeffEdit_[i]) coeffs[i] = coeffEdit_[i]->text().toDouble();
            }
            int cls = classifyQuadric(coeffs);
            int idx = classIdToIndex(cls);
            if (idx >= 0) updateInfo(idx);
        } else {
            // 如果用户停留在“预设选择”页面
            if (presetCombo_) {
                updateInfo(presetCombo_->currentIndex());
            }
        }
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });

    onPresetSelected(0);
}

QWidget* VisualizePage::createInputTab() {
    auto* page = new QWidget;
    page->setObjectName("inputTabMainPage");
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } "
                          "QScrollArea > QWidget > QWidget { background: transparent; }");
    auto* content = new QWidget;
    content->setAttribute(Qt::WA_TranslucentBackground);
    auto* lay = new QVBoxLayout(content);
    lay->setContentsMargins(16, 12, 16, 16);
    lay->setSpacing(10);

    auto* hint = new QLabel(QStringLiteral("F(x,y,z) = a₁₁x² + a₂₂y² + a₃₃z²\n  + 2a₁₂xy + 2a₁₃xz + 2a₂₃yz\n  + 2a₁x + 2a₂y + 2a₃z + a₀ = 0"));
    hint->setStyleSheet(QStringLiteral("font-size:12px; color:#8A8FA3; padding:4px;"));
    lay->addWidget(hint);

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);
    const char* labels[] = { "a₁₁", "a₂₂", "a₃₃", "a₁₂", "a₁₃", "a₂₃", "a₁",  "a₂",  "a₃",  "a₀" };
    const char* defaults[] = { "1", "2", "-3", "0", "0", "0", "0", "0", "0", "-3" };

    for (int i = 0; i < 10; ++i) {
        int row = i / 2, col = (i % 2) * 2;
        auto* lbl = new QLabel(QString::fromUtf8(labels[i]));
        lbl->setStyleSheet(QStringLiteral("font-size:13px; font-weight:500;"));
        lbl->setFixedWidth(28);
        grid->addWidget(lbl, row, col);
        auto* edit = new QLineEdit(QString::fromUtf8(defaults[i]));
        edit->setFixedWidth(90);
        coeffEdit_[i] = edit;
        grid->addWidget(edit, row, col + 1);
    }
    lay->addLayout(grid);
    lay->addSpacing(8);

    analyzeBtn_ = new QPushButton(QStringLiteral("分析并渲染"));
    analyzeBtn_->setCursor(Qt::PointingHandCursor);
    connect(analyzeBtn_, &QPushButton::clicked, this, &VisualizePage::onAnalyze);
    lay->addWidget(analyzeBtn_);
    lay->addStretch(1);
    scroll->setWidget(content);
    auto* wrapper = new QVBoxLayout(page);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(scroll);
    return page;
}

QWidget* VisualizePage::createPresetTab() {
    auto* page = new QWidget;
    page->setObjectName("presetTabMainPage");
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } "
                          "QScrollArea > QWidget > QWidget { background: transparent; }");
    auto* content = new QWidget;
    content->setAttribute(Qt::WA_TranslucentBackground);
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
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VisualizePage::onPresetSelected);
    lay->addWidget(presetCombo_);
    lay->addSpacing(8);

    auto* paramLabel = new QLabel(QStringLiteral("参数调节："));
    paramLabel->setStyleSheet(QStringLiteral("font-size:13px; font-weight:500;"));
    lay->addWidget(paramLabel);

    auto* paramGrid = new QGridLayout;
    paramGrid->setHorizontalSpacing(8);
    paramGrid->setVerticalSpacing(6);

    auto makeParam = [&](const QString& name, double val, double min, double max) -> QPair<QLabel*, QDoubleSpinBox*> {
        auto* lbl = new QLabel(name); lbl->setStyleSheet(QStringLiteral("font-size:13px;")); lbl->setFixedWidth(20);
        auto* spin = new QDoubleSpinBox; spin->setRange(min, max); spin->setDecimals(2); spin->setSingleStep(0.1); spin->setValue(val); spin->setFixedWidth(90);
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
    connect(presetBtn_, &QPushButton::clicked, this, [this]{ onPresetSelected(presetCombo_->currentIndex()); });
    lay->addWidget(presetBtn_);

    lay->addStretch(1);
    scroll->setWidget(content);
    auto* wrapper = new QVBoxLayout(page);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(scroll);
    return page;
}

void VisualizePage::setupRenderArea(QWidget* parent) {
    auto* lay = qobject_cast<QVBoxLayout*>(parent->layout());
    if (!lay) return;

    infoBrowser_ = new QTextBrowser;
    infoBrowser_->setOpenLinks(false);
    infoBrowser_->setMaximumHeight(115); 
    lay->addWidget(infoBrowser_);

    quadric_ = new QuadricWidget;
    auto* renderContainer = new QWidget;
    auto* renderLay = new QVBoxLayout(renderContainer);
    renderLay->setContentsMargins(0, 0, 0, 0);
    renderLay->setSpacing(0);
    renderLay->addWidget(quadric_, 1);

    resetViewBtn_ = new QPushButton(QStringLiteral("重置视角"));
    resetViewBtn_->setCursor(Qt::PointingHandCursor);
    resetViewBtn_->setStyleSheet(QStringLiteral("QPushButton { background:rgba(0,0,0,0.5); color:#ccc; border:1px solid #555; border-radius:4px; padding:4px 12px; font-size:12px; } QPushButton:hover { background:rgba(60,60,80,0.7); }"));
    connect(resetViewBtn_, &QPushButton::clicked, this, &VisualizePage::onResetView);
    auto* overlayLayout = new QHBoxLayout;
    overlayLayout->addStretch();
    overlayLayout->addWidget(resetViewBtn_);
    overlayLayout->setContentsMargins(0, 0, 8, 8);
    renderLay->addLayout(overlayLayout);

    lay->addWidget(renderContainer, 1);
}

void VisualizePage::onAnalyze() {
    std::vector<double> coeffs(10);
    for (int i = 0; i < 10; ++i) coeffs[i] = coeffEdit_[i]->text().toDouble();
    renderDirectQuadric(quadric_, coeffs);
    int cls = classifyQuadric(coeffs);
    int idx = classIdToIndex(cls);
    if (idx >= 0) updateInfo(idx);
}

void VisualizePage::onPresetSelected(int index) {
    if (index < 0 || index >= kClassCount) return;
    int id = kClasses[index].id;
    double a = paramA_->value(), b = paramB_->value(), c = paramC_->value(), p = paramP_->value();
    renderPreset(id, a, b, c, p);
    updateInfo(index);
}

void VisualizePage::renderPreset(int id, double a, double b, double c, double p) {
    setPresetSurface(quadric_, id, a, b, c, p);
}

static QString getTyporaStyleEquation(int id,const QString& color) {
    QStringList tops, bots, ops;

    switch (id) {
    case 0:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "<i>z</i><sup>2</sup>", "1"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", "<i>c</i><sup>2</sup>", ""};
        ops = {"+", "+", "="};
        break;
    case 2:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "<i>z</i><sup>2</sup>", "1"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", "<i>c</i><sup>2</sup>", ""};
        ops = {"+", "&minus;", "="};
        break;
    case 3:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "<i>z</i><sup>2</sup>", "&minus;1"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", "<i>c</i><sup>2</sup>", ""};
        ops = {"+", "&minus;", "="};
        break;
    case 4:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "2<i>pz</i>"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", ""};
        ops = {"+", "="};
        break;
    case 5:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "2<i>pz</i>"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", ""};
        ops = {"&minus;", "="};
        break;
    case 6:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "<i>z</i><sup>2</sup>", "0"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", "<i>c</i><sup>2</sup>", ""};
        ops = {"+", "&minus;", "="};
        break;
    case 8:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "1"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", ""};
        ops = {"+", "="};
        break;
    case 10:
        tops = {"<i>x</i><sup>2</sup>", "<i>y</i><sup>2</sup>", "1"};
        bots = {"<i>a</i><sup>2</sup>", "<i>b</i><sup>2</sup>", ""};
        ops = {"&minus;", "="};
        break;
    case 11:
        tops = {"<i>y</i><sup>2</sup>", "2<i>px</i>"};
        bots = {"", ""};
        ops = {"="};
        break;
    default:
        return QString("<span style='font-style:italic;'>%1</span>")
            .arg(QString::fromUtf8(kClasses[classIdToIndex(id)].stdEq));
    }

    QString row1 = "<tr>", row2 = "<tr>";
    for (int i = 0; i < tops.size(); ++i) {
        if (bots[i].isEmpty()) {
            row1 += QString("<td rowspan='2' valign='middle' align='center' style='padding:0 2px;'>%1</td>").arg(tops[i]);
        } else {
            row1 += QString("<td align='center' style='border-bottom:1px solid %1; padding:0 4px;'>%2</td>").arg(color, tops[i]);

            row2 += QString("<td align='center' style='padding:0 4px;'>%1</td>").arg(bots[i]);
        }

        if (i < ops.size()) {
            row1 += QString("<td rowspan='2' valign='middle' align='center' style='padding:0 6px;'>%1</td>").arg(ops[i]);
        }
    }
    row1 += "</tr>"; row2 += "</tr>";

    return QString("<table border='0' cellpadding='0' cellspacing='0' "
                   "style='font-family:\"Cambria Math\", \"Times New Roman\", serif; font-size:16px; font-weight:bold; margin:0; color:%1;'>%2%3</table>")
        .arg(color, row1, row2);
}

void VisualizePage::updateInfo(int cls) {
    bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
    // 暗色模式下使用亮白色 #FFFFFF，亮色模式下使用深色 #1E293B
    QString textColor = isDark ? QStringLiteral("#FFFFFF") : QStringLiteral("#1E293B");

    auto* doc = infoBrowser_->document();
    doc->clear();

    QString eqHtml = getTyporaStyleEquation(kClasses[cls].id, textColor);

    QString html = QStringLiteral(
                       "<div style='padding:2px;'>"

                       "<div style='margin-bottom:8px;'>"
                       "<b style='font-size:16px; color:%1;'>%2</b>&nbsp;&nbsp;&nbsp;&nbsp;"
                       "<span style='color:#8A8FA3; font-size:12px;'>%3</span>"
                       "</div>"

                       "<table border='0' cellpadding='0' cellspacing='0' style='margin-bottom:8px;'><tr>"
                       "<td valign='middle'><span style='font-size:13px; color:#A0AEC0;'>标准方程：</span></td>" // 暗色下这里也可以调亮一点
                       "<td valign='middle' style='color:%1;'>%4</td>"
                       "</tr></table>"

                       "<div>"
                       "<span style='font-size:13px; color:#A0AEC0;'>惯性指数 (p,q,r) = (%5, %6, %7)</span>&nbsp;&nbsp;&nbsp;&nbsp;"
                       "<span style='font-size:13px; color:#8A8FA3;'>%8</span>"
                       "</div>"
                       "</div>"
                       ).arg(textColor, // 替换这里
                            QString::fromUtf8(kClasses[cls].name),
                            QString::fromUtf8(kClasses[cls].cat),
                            eqHtml)
                       .arg(kClasses[cls].p)
                       .arg(kClasses[cls].q)
                       .arg(kClasses[cls].r)
                       .arg(QString::fromUtf8(kClasses[cls].geo));

    infoBrowser_->setHtml(html);
}

void VisualizePage::onResetView() { if (quadric_) quadric_->resetView(); }

} 
