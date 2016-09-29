#ifndef VRVIEW_H
#define VRVIEW_H

#include <QOpenGLFunctions_4_1_Core>
#include <QGLWidget>
#include <QGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QGLShaderProgram>
#include <QGLFramebufferObject>
#include <QOpenGLDebugMessage>
#include <QOpenGLDebugLogger>
#include <QOpenGLTexture>
#include <QTimer>
#include <openvr.h>

class FBOHandle
{
public:
    FBOHandle(QOpenGLFunctions_4_1_Core *gl, int width, int height);
    virtual ~FBOHandle();

    void blitResolve();

    QOpenGLFunctions_4_1_Core *gl;

    bool ok;

    int width, height;

    GLuint depthBuffer;
    GLuint renderTexture;
    GLuint renderFBO;
    GLuint resolveTexture;
    GLuint resolveFBO;
};

class VRView : public QGLWidget, protected QOpenGLFunctions_4_1_Core
{
    Q_OBJECT
public:
    explicit VRView(QWidget *parent = 0);
    virtual ~VRView();

signals:
    void framesPerSecond(float);
    void deviceIdentifier(const QString&);
    void frameSwap();

public slots:

protected slots:
    void updateFramerate();
    void shutdown();
    void debugMessage(QOpenGLDebugMessage message);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    void initVR();

    void renderEye(vr::Hmd_Eye eye);

    void updatePoses();

    bool compileShader(QGLShaderProgram &shader,
                       const QString& vertexShaderPath,
                       const QString& fragmentShaderPath);

    QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix34_t &mat);
    QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix44_t &mat);

    // QMatrix is using qreal, so we need to overload to handle both platform cases
    void glUniformMatrix4(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    void glUniformMatrix4(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);

    QMatrix4x4 viewProjection(vr::Hmd_Eye eye);

    QString getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
                                   vr::TrackedDeviceProperty prop,
                                   vr::TrackedPropertyError *error = 0);

    vr::IVRSystem *m_hmd;
    vr::TrackedDevicePose_t m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    QMatrix4x4 m_matrixDevicePose[vr::k_unMaxTrackedDeviceCount];

    QMatrix4x4 m_leftProjection, m_leftPose;
    QMatrix4x4 m_rightProjection, m_rightPose;
    QMatrix4x4 m_hmdPose;

    QOpenGLDebugLogger *m_logger;

    QGLShaderProgram m_shader;
    QGLBuffer m_vertexBuffer;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLTexture *m_texture;
    int m_vertCount;

    uint32_t m_eyeWidth, m_eyeHeight;
    FBOHandle *m_leftBuffer, *m_rightBuffer;
    //QGLFramebufferObject *m_leftBuffer;
    //QGLFramebufferObject *m_rightBuffer;
    int m_widgetWidth, m_widgetHeight;

    int m_frames;
    QTimer m_frameTimer;
};

#endif // VRVIEW_H
