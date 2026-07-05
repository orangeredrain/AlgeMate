#pragma once

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QVector3D>
#include <functional>
#include <vector>

namespace AlgeMate::Calculator::Visualize {

class QuadricWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit QuadricWidget(QWidget* parent = nullptr);

    using SurfaceFunc = std::function<QVector3D(float u, float v)>;

    void setSurface(SurfaceFunc fn,
                    float uMin = 0, float uMax = 3.14159265f,
                    float vMin = 0, float vMax = 6.28318531f,
                    int uSteps = 64, int vSteps = 64);

    void setSecondSurface(SurfaceFunc fn,
                          float uMin, float uMax,
                          float vMin, float vMax,
                          int uSteps = 64, int vSteps = 64);
    void setColor(const QVector3D& color) { color_ = color; update(); }
    void setAlpha(float a) { alpha_ = a; update(); }
    void setDrawAxes(bool on) { drawAxes_ = on; update(); }
    void setDrawGrid(bool on) { drawGrid_ = on; update(); }
    void resetView();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;

private:
    void rebuildMesh();
    void buildAxesMesh();
    void buildGridMesh();

    QOpenGLShaderProgram* prog_ = nullptr;
    QOpenGLShaderProgram* lineProg_ = nullptr;

    QOpenGLVertexArrayObject surfVao_;
    QOpenGLBuffer surfVbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer surfEbo_{QOpenGLBuffer::IndexBuffer};
    GLsizei surfIndexCount_ = 0;

    bool       hasSurf2_ = false;
    QOpenGLVertexArrayObject surf2Vao_;
    QOpenGLBuffer surf2Vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer surf2Ebo_{QOpenGLBuffer::IndexBuffer};
    GLsizei surf2IndexCount_ = 0;

    QOpenGLVertexArrayObject axesVao_;
    QOpenGLBuffer axesVbo_{QOpenGLBuffer::VertexBuffer};
    GLsizei axesVertCount_ = 0;

    QOpenGLVertexArrayObject gridVao_;
    QOpenGLBuffer gridVbo_{QOpenGLBuffer::VertexBuffer};
    GLsizei gridVertCount_ = 0;

    QMatrix4x4 view_;
    QVector3D  color_{0.30f, 0.55f, 1.0f};
    float      alpha_  = 0.85f;
    float      zoom_   = 5.0f;
    bool       drawAxes_ = true;
    bool       drawGrid_ = true;

    QPoint     lastMouse_;
    QQuaternion rotation_;
    QQuaternion currentRot_;

    SurfaceFunc surface_;
    float uMin_ = 0, uMax_ = 3.14159265f;
    float vMin_ = 0, vMax_ = 6.28318531f;
    int uSteps_ = 64, vSteps_ = 64;

    SurfaceFunc surface2_;
    float u2Min_ = 0, u2Max_ = 3.14159265f;
    float v2Min_ = 0, v2Max_ = 6.28318531f;
    int u2Steps_ = 64, v2Steps_ = 64;
};

} 
