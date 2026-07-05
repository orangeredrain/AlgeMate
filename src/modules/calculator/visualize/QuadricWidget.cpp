#include "QuadricWidget.h"

#include <QMouseEvent>
#include <QOpenGLFunctions>
#include <QWheelEvent>
#include <QtMath>

namespace AlgeMate::Calculator::Visualize {

static const char* kSurfVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vWorldPos  = (uModel * vec4(aPos, 1.0)).xyz;
    vNormal    = mat3(uModel) * aNormal;
}
)";

static const char* kSurfFrag = R"(
#version 330 core
in  vec3 vWorldPos;
in  vec3 vNormal;
out vec4 fragColor;

uniform vec3  uColor;
uniform float uAlpha;
uniform vec3  uLightDir;
uniform vec3  uEyePos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uEyePos - vWorldPos);
    if (!gl_FrontFacing) N = -N;

    vec3 L = normalize(uLightDir);
    vec3 H = normalize(L + V);

    // Hemisphere ambient: sky blue above, warm below
    float hemi = 0.5 + 0.5 * dot(N, vec3(0.0, 1.0, 0.0));
    vec3 ambColor = mix(vec3(0.12, 0.10, 0.08), vec3(0.18, 0.22, 0.30), hemi);
    float ambient = 0.45;

    // Main diffuse
    float diff = max(dot(N, L), 0.0) * 0.50;

    // Fill light (opposite side, softer)
    vec3 L2 = normalize(vec3(-0.4, -0.2, 0.6));
    float diff2 = max(dot(N, L2), 0.0) * 0.18;

    // Blinn-Phong specular
    float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.45;

    // Fresnel rim light
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 rim = vec3(0.6, 0.7, 1.0) * fresnel * 0.25;

    vec3 col = uColor * (ambient + diff + diff2) + vec3(1.0) * spec + rim;
    fragColor = vec4(col, uAlpha);
}
)";

static const char* kLineVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* kLineFrag = R"(
#version 330 core
in  vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 0.55);
}
)";

QuadricWidget::QuadricWidget(QWidget* parent) : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setSurface([](float u, float v) -> QVector3D {
        return {2.0f * sinf(u) * cosf(v),
                3.0f * sinf(u) * sinf(v),
                cosf(u)};
    });
}

void QuadricWidget::resetView() {
    rotation_ = QQuaternion();
    zoom_ = 5.0f;
    update();
}

void QuadricWidget::setSurface(SurfaceFunc fn,
                                float uMin, float uMax,
                                float vMin, float vMax,
                                int uSteps, int vSteps) {
    surface_ = std::move(fn);
    hasSurf2_ = false;  
    uMin_ = uMin; uMax_ = uMax;
    vMin_ = vMin; vMax_ = vMax;
    uSteps_ = uSteps; vSteps_ = vSteps;
    if (prog_) {
        makeCurrent();
        rebuildMesh();
        doneCurrent();
    }
    update();
}

void QuadricWidget::setSecondSurface(SurfaceFunc fn,
                                      float uMin, float uMax,
                                      float vMin, float vMax,
                                      int uSteps, int vSteps) {
    surface2_ = std::move(fn);
    u2Min_ = uMin; u2Max_ = uMax;
    v2Min_ = vMin; v2Max_ = vMax;
    u2Steps_ = uSteps; v2Steps_ = vSteps;
    hasSurf2_ = true;
    if (prog_) {
        makeCurrent();
        rebuildMesh();
        doneCurrent();
    }
    update();
}

void QuadricWidget::initializeGL() {
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glEnable(GL_LINE_SMOOTH);
    f->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    prog_ = new QOpenGLShaderProgram(this);
    prog_->addShaderFromSourceCode(QOpenGLShader::Vertex, kSurfVert);
    prog_->addShaderFromSourceCode(QOpenGLShader::Fragment, kSurfFrag);
    prog_->link();

    lineProg_ = new QOpenGLShaderProgram(this);
    lineProg_->addShaderFromSourceCode(QOpenGLShader::Vertex, kLineVert);
    lineProg_->addShaderFromSourceCode(QOpenGLShader::Fragment, kLineFrag);
    lineProg_->link();

    surfVao_.create(); axesVao_.create(); gridVao_.create();
    surfVbo_.create(); axesVbo_.create(); gridVbo_.create();
    surfEbo_.create();
    surf2Vao_.create(); surf2Vbo_.create(); surf2Ebo_.create();

    rebuildMesh();
    buildAxesMesh();
    buildGridMesh();
}

void QuadricWidget::rebuildMesh() {
    if (!surface_) return;

    struct Vertex { float x, y, z, nx, ny, nz; };
    std::vector<Vertex> verts;
    std::vector<GLuint> indices;

    for (int i = 0; i <= uSteps_; ++i) {
        float u = uMin_ + (uMax_ - uMin_) * i / uSteps_;
        for (int j = 0; j <= vSteps_; ++j) {
            float v = vMin_ + (vMax_ - vMin_) * j / vSteps_;
            QVector3D p = surface_(u, v);
            float eps = 1e-3f;
            QVector3D pu = surface_(qMin(u + eps, uMax_), v)
                          - surface_(qMax(u - eps, uMin_), v);
            QVector3D pv = surface_(u, qMin(v + eps, vMax_))
                          - surface_(u, qMax(v - eps, vMin_));
            QVector3D n = QVector3D::crossProduct(pu, pv).normalized();
            verts.push_back({p.x(), p.y(), p.z(), n.x(), n.y(), n.z()});
        }
    }

    for (int i = 0; i < uSteps_; ++i) {
        for (int j = 0; j < vSteps_; ++j) {
            GLuint a = GLuint(i * (vSteps_ + 1) + j);
            GLuint b = a + GLuint(vSteps_ + 1);
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(b); indices.push_back(b + 1); indices.push_back(a + 1);
        }
    }

    surfVao_.bind();
    surfVbo_.bind();
    surfVbo_.allocate(verts.data(), int(verts.size() * sizeof(Vertex)));
    surfEbo_.bind();
    surfEbo_.allocate(indices.data(), int(indices.size() * sizeof(GLuint)));
    surfIndexCount_ = GLsizei(indices.size());

    prog_->bind();
    prog_->enableAttributeArray(0);
    prog_->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));
    prog_->enableAttributeArray(1);
    prog_->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, nx), 3, sizeof(Vertex));
    surfVao_.release();

    if (hasSurf2_ && surface2_) {
        struct Vertex { float x, y, z, nx, ny, nz; };
        std::vector<Vertex> verts;
        std::vector<GLuint> indices;

        for (int i = 0; i <= u2Steps_; ++i) {
            float u = u2Min_ + (u2Max_ - u2Min_) * i / u2Steps_;
            for (int j = 0; j <= v2Steps_; ++j) {
                float v = v2Min_ + (v2Max_ - v2Min_) * j / v2Steps_;
                QVector3D p = surface2_(u, v);
                float eps = 1e-3f;
                QVector3D pu = surface2_(qMin(u + eps, u2Max_), v)
                             - surface2_(qMax(u - eps, u2Min_), v);
                QVector3D pv = surface2_(u, qMin(v + eps, v2Max_))
                             - surface2_(u, qMax(v - eps, v2Min_));
                QVector3D n = QVector3D::crossProduct(pu, pv).normalized();
                verts.push_back({p.x(), p.y(), p.z(), n.x(), n.y(), n.z()});
            }
        }

        for (int i = 0; i < u2Steps_; ++i) {
            for (int j = 0; j < v2Steps_; ++j) {
                GLuint a = GLuint(i * (v2Steps_ + 1) + j);
                GLuint b = a + GLuint(v2Steps_ + 1);
                indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
                indices.push_back(b); indices.push_back(b + 1); indices.push_back(a + 1);
            }
        }

        surf2Vao_.bind();
        surf2Vbo_.bind();
        surf2Vbo_.allocate(verts.data(), int(verts.size() * sizeof(Vertex)));
        surf2Ebo_.bind();
        surf2Ebo_.allocate(indices.data(), int(indices.size() * sizeof(GLuint)));
        surf2IndexCount_ = GLsizei(indices.size());

        prog_->bind();
        prog_->enableAttributeArray(0);
        prog_->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));
        prog_->enableAttributeArray(1);
        prog_->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, nx), 3, sizeof(Vertex));
        surf2Vao_.release();
    } else {
        surf2IndexCount_ = 0;
    }
}

void QuadricWidget::buildAxesMesh() {
    struct V { float x, y, z, r, g, b; };
    std::vector<V> v;
    float len = 3.0f;

    v.push_back({0, 0, 0, 0.85f, 0.30f, 0.25f});
    v.push_back({len, 0, 0, 0.85f, 0.30f, 0.25f});

    v.push_back({0, 0, 0, 0.30f, 0.80f, 0.35f});
    v.push_back({0, len, 0, 0.30f, 0.80f, 0.35f});

    v.push_back({0, 0, 0, 0.35f, 0.50f, 1.0f});
    v.push_back({0, 0, len, 0.35f, 0.50f, 1.0f});

    float t = 0.15f, s = 0.06f;

    v.push_back({len, 0, 0, 0.85f, 0.30f, 0.25f}); v.push_back({len-t, s, 0, 0.85f, 0.30f, 0.25f});
    v.push_back({len, 0, 0, 0.85f, 0.30f, 0.25f}); v.push_back({len-t, -s, 0, 0.85f, 0.30f, 0.25f});
    v.push_back({len, 0, 0, 0.85f, 0.30f, 0.25f}); v.push_back({len-t, 0, s, 0.85f, 0.30f, 0.25f});
    v.push_back({len, 0, 0, 0.85f, 0.30f, 0.25f}); v.push_back({len-t, 0, -s, 0.85f, 0.30f, 0.25f});

    v.push_back({0, len, 0, 0.30f, 0.80f, 0.35f}); v.push_back({s, len-t, 0, 0.30f, 0.80f, 0.35f});
    v.push_back({0, len, 0, 0.30f, 0.80f, 0.35f}); v.push_back({-s, len-t, 0, 0.30f, 0.80f, 0.35f});
    v.push_back({0, len, 0, 0.30f, 0.80f, 0.35f}); v.push_back({0, len-t, s, 0.30f, 0.80f, 0.35f});
    v.push_back({0, len, 0, 0.30f, 0.80f, 0.35f}); v.push_back({0, len-t, -s, 0.30f, 0.80f, 0.35f});

    v.push_back({0, 0, len, 0.35f, 0.50f, 1.0f}); v.push_back({s, 0, len-t, 0.35f, 0.50f, 1.0f});
    v.push_back({0, 0, len, 0.35f, 0.50f, 1.0f}); v.push_back({-s, 0, len-t, 0.35f, 0.50f, 1.0f});
    v.push_back({0, 0, len, 0.35f, 0.50f, 1.0f}); v.push_back({0, s, len-t, 0.35f, 0.50f, 1.0f});
    v.push_back({0, 0, len, 0.35f, 0.50f, 1.0f}); v.push_back({0, -s, len-t, 0.35f, 0.50f, 1.0f});

    axesVertCount_ = GLsizei(v.size());

    axesVao_.bind();
    axesVbo_.bind();
    axesVbo_.allocate(v.data(), int(v.size() * sizeof(V)));
    lineProg_->bind();
    lineProg_->enableAttributeArray(0);
    lineProg_->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(V));
    lineProg_->enableAttributeArray(1);
    lineProg_->setAttributeBuffer(1, GL_FLOAT, offsetof(V, r), 3, sizeof(V));
    axesVao_.release();
}

void QuadricWidget::buildGridMesh() {
    struct V { float x, y, z, r, g, b; };
    std::vector<V> v;
    float gLen = 3.0f;
    float step = 0.5f;
    float c = 0.20f;

    for (float x = -gLen; x <= gLen + 0.01f; x += step) {
        float bright = (fabs(x) < 0.01f) ? c * 1.5f : c;
        v.push_back({x, 0, -gLen, bright, bright, bright});
        v.push_back({x, 0, gLen, bright, bright, bright});
    }
    for (float z = -gLen; z <= gLen + 0.01f; z += step) {
        float bright = (fabs(z) < 0.01f) ? c * 1.5f : c;
        v.push_back({-gLen, 0, z, bright, bright, bright});
        v.push_back({gLen, 0, z, bright, bright, bright});
    }

    gridVertCount_ = GLsizei(v.size());

    gridVao_.bind();
    gridVbo_.bind();
    gridVbo_.allocate(v.data(), int(v.size() * sizeof(V)));
    lineProg_->bind();
    lineProg_->enableAttributeArray(0);
    lineProg_->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(V));
    lineProg_->enableAttributeArray(1);
    lineProg_->setAttributeBuffer(1, GL_FLOAT, offsetof(V, r), 3, sizeof(V));
    gridVao_.release();
}

void QuadricWidget::resizeGL(int, int) {}

void QuadricWidget::paintGL() {
    auto* f = QOpenGLContext::currentContext()->functions();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    qreal dpr = devicePixelRatio();
    int w = int(width() * dpr);
    int h = int(std::max(height(), 1) * dpr);

    QMatrix4x4 proj;
    proj.perspective(40.0f, float(w) / std::max(h, 1), 0.1f, 100.0f);

    view_.setToIdentity();
    view_.translate(0, 0, -zoom_);
    view_.rotate(rotation_);

    QMatrix4x4 model;
    QMatrix4x4 mvp = proj * view_ * model;

    QVector3D eyePos(0, 0, zoom_);

    if (drawGrid_ && gridVertCount_ > 0) {
        f->glLineWidth(1.0f);
        lineProg_->bind();
        lineProg_->setUniformValue("uMVP", mvp);
        gridVao_.bind();
        f->glDrawArrays(GL_LINES, 0, gridVertCount_);
        gridVao_.release();
        lineProg_->release();
    }

    if (drawAxes_ && axesVertCount_ > 0) {
        f->glLineWidth(2.0f);
        lineProg_->bind();
        lineProg_->setUniformValue("uMVP", mvp);
        axesVao_.bind();
        f->glDrawArrays(GL_LINES, 0, axesVertCount_);
        axesVao_.release();
        lineProg_->release();
    }

    f->glEnable(GL_BLEND);

    auto drawSurf = [&](GLsizei count, QOpenGLVertexArrayObject& vao) {
        f->glCullFace(GL_FRONT);
        f->glEnable(GL_CULL_FACE);
        f->glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        f->glCullFace(GL_BACK);
        f->glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        f->glDisable(GL_CULL_FACE);
    };

    if (surfIndexCount_ > 0) {
        prog_->bind();
        prog_->setUniformValue("uMVP", mvp);
        prog_->setUniformValue("uModel", model);
        prog_->setUniformValue("uLightDir", QVector3D(0.5f, 0.8f, 0.6f));
        prog_->setUniformValue("uEyePos", eyePos);
        prog_->setUniformValue("uColor", color_);
        prog_->setUniformValue("uAlpha", alpha_);
        surfVao_.bind();
        drawSurf(surfIndexCount_, surfVao_);
        surfVao_.release();
        prog_->release();
    }

    if (surf2IndexCount_ > 0) {
        prog_->bind();
        prog_->setUniformValue("uMVP", mvp);
        prog_->setUniformValue("uModel", model);
        prog_->setUniformValue("uLightDir", QVector3D(0.5f, 0.8f, 0.6f));
        prog_->setUniformValue("uEyePos", eyePos);
        prog_->setUniformValue("uColor", color_);
        prog_->setUniformValue("uAlpha", alpha_);
        surf2Vao_.bind();
        drawSurf(surf2IndexCount_, surf2Vao_);
        surf2Vao_.release();
        prog_->release();
    }
}

void QuadricWidget::mousePressEvent(QMouseEvent* ev) {
    lastMouse_ = ev->pos();
    currentRot_ = rotation_;
}

void QuadricWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (!(ev->buttons() & Qt::LeftButton)) return;
    QPoint delta = ev->pos() - lastMouse_;
    if (delta.manhattanLength() < 2) return;

    float w = float(width());
    float h = float(height());
    QVector3D axis(float(delta.y()) / h, float(delta.x()) / w, 0.0f);
    float angle = axis.length() * 180.0f;
    if (angle < 0.3f) return;
    axis.normalize();

    rotation_ = QQuaternion::fromAxisAndAngle(axis, angle) * currentRot_;
    update();
}

void QuadricWidget::wheelEvent(QWheelEvent* ev) {
    zoom_ -= ev->angleDelta().y() * 0.003f;
    zoom_ = qBound(0.5f, zoom_, 30.0f);
    update();
}

} 
