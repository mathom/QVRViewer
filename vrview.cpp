#include "vrview.h"
#include <QGL>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include "modelFormats.h"

#define NEAR_CLIP 0.1f
#define FAR_CLIP 10000.0f

VRView::VRView(QWidget *parent) : QGLWidget(parent),
    m_texture(0), m_vertCount(0),
    m_eyeWidth(0), m_eyeHeight(0), m_leftBuffer(0), m_rightBuffer(0),
    m_widgetWidth(0), m_widgetHeight(0), m_frames(0)
{
    connect(&m_frameTimer, SIGNAL(timeout()), this, SLOT(updateGL()));

    QTimer *fpsTimer = new QTimer(this);
    connect(fpsTimer, SIGNAL(timeout()), this, SLOT(updateFramerate()));
    fpsTimer->start(1000);
}

VRView::~VRView()
{
    shutdown();
}

void VRView::updateFramerate()
{
    if (m_frames > 0)
        emit framesPerSecond(m_frames);

    m_frames = 0;
}

void VRView::shutdown()
{
    makeCurrent();

    delete m_texture;

    m_vertexBuffer.destroy();
    m_vao.destroy();

    delete m_leftBuffer;
    delete m_rightBuffer;

    if (m_hmd)
    {
        vr::VR_Shutdown();
        m_hmd = 0;
    }

    qDebug() << "shutdown";
    doneCurrent();
}

void VRView::debugMessage(QOpenGLDebugMessage message)
{
    qDebug() << message;
}

void VRView::initializeGL()
{
    initializeOpenGLFunctions();

    m_logger = new QOpenGLDebugLogger(this);

    connect(m_logger, SIGNAL(messageLogged(QOpenGLDebugMessage)),
             this, SLOT(debugMessage(QOpenGLDebugMessage)), Qt::DirectConnection);

    if (m_logger->initialize())
    {
        m_logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        m_logger->enableMessages();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // compile our shader
    compileShader(m_shader, ":/shaders/unlit.vert", ":/shaders/unlit.frag");

    // build out sample geometry
    m_vao.create();
    m_vao.bind();

    QVector<GLfloat> points = readObj(":/models/monkey.obj");
    m_vertCount = points.length();

    m_vertexBuffer.create();
    m_vertexBuffer.setUsagePattern(QGLBuffer::StaticDraw);
    m_vertexBuffer.bind();

    m_vertexBuffer.allocate(points.data(), 5 * points.length() * sizeof(GLfloat));

    m_shader.bind();

    m_shader.setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    m_shader.enableAttributeArray("vertex");

    m_shader.setAttributeBuffer("texCoord", GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
    m_shader.enableAttributeArray("texCoord");

    m_shader.setUniformValue("diffuse", 0);

    m_texture = new QOpenGLTexture(QImage(":/textures/uvmap.png"));

    initVR();

    m_frameTimer.start();
}

void VRView::paintGL()
{ 
    updatePoses();

    glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
    glViewport(0, 0, m_eyeWidth, m_eyeHeight);

    glEnable(GL_MULTISAMPLE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_leftBuffer->renderFBO);
    renderEye(vr::Eye_Left);
    m_leftBuffer->blitResolve();

    glEnable(GL_MULTISAMPLE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_rightBuffer->renderFBO);
    renderEye(vr::Eye_Right);
    m_rightBuffer->blitResolve();

    if (m_hmd)
    {
        vr::Texture_t leftEyeTexture = { (void*)m_leftBuffer->resolveTexture, vr::API_OpenGL, vr::ColorSpace_Gamma };
        vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
        vr::Texture_t rightEyeTexture = { (void*)m_rightBuffer->resolveTexture, vr::API_OpenGL, vr::ColorSpace_Gamma };
        vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
    }

    glFinish();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, m_widgetWidth, m_widgetHeight);
    glDisable(GL_MULTISAMPLE);
    renderEye(vr::Eye_Right);

    m_frames++;
}

void VRView::renderEye(vr::Hmd_Eye eye)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_vao.bind();
    m_shader.bind();
    m_texture->bind(0);

    m_shader.setUniformValue("transform", viewProjection(eye));
    glDrawArrays(GL_TRIANGLES, 0, m_vertCount);

    QMatrix4x4 m;
    m.translate(2, 0, 0);
    m_shader.setUniformValue("transform", viewProjection(eye)*m);
    glDrawArrays(GL_TRIANGLES, 0, m_vertCount);
}

void VRView::resizeGL(int w, int h)
{
    m_widgetWidth = w;
    m_widgetHeight = qMax(h, 1);
}

void VRView::initVR()
{
    vr::EVRInitError error = vr::VRInitError_None;
    m_hmd = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None)
    {
        m_hmd = 0;

        QString message = vr::VR_GetVRInitErrorAsEnglishDescription(error);
        qCritical() << message;
        QMessageBox::critical(this, "Unable to init VR", message);
        return;
    }

    // get eye matrices
    m_rightProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Right, NEAR_CLIP, FAR_CLIP, vr::API_OpenGL));
    m_rightPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Right)).inverted();

    m_leftProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Left, NEAR_CLIP, FAR_CLIP, vr::API_OpenGL));
    m_leftPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Left)).inverted();
    qDebug() << "right eye" << m_rightProjection << m_rightPose;
    qDebug() << "left eye" << m_leftProjection << m_leftPose;

    QString ident;
    ident.append("QVRViewer - ");
    ident.append(getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String));
    ident.append(" ");
    ident.append(getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String));
    emit deviceIdentifier(ident);

    // setup frame buffers for eyes
    m_hmd->GetRecommendedRenderTargetSize(&m_eyeWidth, &m_eyeHeight);

    m_leftBuffer = new FBOHandle(this, m_eyeWidth, m_eyeHeight);
    m_rightBuffer = new FBOHandle(this, m_eyeWidth, m_eyeHeight);

    // turn on compositor
    if (!vr::VRCompositor())
    {
        QString message = "Compositor initialization failed. See log file for details";
        qCritical() << message;
        QMessageBox::critical(this, "Unable to init VR", message);
        return;
    }

    vr::VRCompositor()->ShowMirrorWindow();
}

void VRView::updatePoses()
{
    vr::VRCompositor()->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    for (unsigned int i=0; i<vr::k_unMaxTrackedDeviceCount; i++)
    {
        if (m_trackedDevicePose[i].bPoseIsValid)
        {
            m_matrixDevicePose[i] =  vrMatrixToQt(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
    }

    if (m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
        m_hmdPose = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd].inverted();
    }
}

bool VRView::compileShader(QGLShaderProgram &shader, const QString &vertexShaderPath, const QString &fragmentShaderPath)
{
    bool result = shader.addShaderFromSourceFile(QGLShader::Vertex, vertexShaderPath);
    if (!result)
        qCritical() << shader.log();

    result = shader.addShaderFromSourceFile(QGLShader::Fragment, fragmentShaderPath);
    if (!result)
        qCritical() << shader.log();

    result = shader.link();
    if (!result)
        qCritical() << "Could not link shader program:" << shader.log();

    return result;
}

QMatrix4x4 VRView::vrMatrixToQt(const vr::HmdMatrix34_t &mat)
{
    return QMatrix4x4(
        mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
        mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
        mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
        0.0,         0.0,         0.0,         1.0f
    );
}

QMatrix4x4 VRView::vrMatrixToQt(const vr::HmdMatrix44_t &mat)
{
    return QMatrix4x4(
        mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
        mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
        mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
        mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]
    );
}

void VRView::glUniformMatrix4(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    glUniformMatrix4fv(location, count, transpose, value);
}

void VRView::glUniformMatrix4(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
{
    glUniformMatrix4dv(location, count, transpose, value);
}

QMatrix4x4 VRView::viewProjection(vr::Hmd_Eye eye)
{
    QMatrix4x4 s;
    s.scale(.5f);

    if (eye == vr::Eye_Left)
        return m_leftProjection * m_leftPose * m_hmdPose * s;
    else
        return m_rightProjection * m_rightPose * m_hmdPose * s;
}

QString VRView::getTrackedDeviceString(vr::TrackedDeviceIndex_t device, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *error)
{
    uint32_t len = m_hmd->GetStringTrackedDeviceProperty(device, prop, NULL, 0, error);
    if(len == 0)
        return "";

    char *buf = new char[len];
    m_hmd->GetStringTrackedDeviceProperty(device, prop, buf, len, error);

    QString result = QString::fromLocal8Bit(buf);
    delete [] buf;

    return result;
}

FBOHandle::FBOHandle(QOpenGLFunctions_4_1_Core *gl, int width, int height) :
    gl(gl), ok(false), width(width), height(height),
    depthBuffer(0), renderTexture(0), renderFBO(0),
    resolveTexture(0), resolveFBO(0)
{
    gl->glGenFramebuffers(1, &renderFBO );
    gl->glBindFramebuffer(GL_FRAMEBUFFER, renderFBO);

    gl->glGenRenderbuffers(1, &depthBuffer);
    gl->glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    gl->glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, width, height);
    gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,	depthBuffer);

    gl->glGenTextures(1, &renderTexture);
    gl->glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, renderTexture);
    gl->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, width, height, true);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, renderTexture, 0);

    gl->glGenFramebuffers(1, &resolveFBO);
    gl->glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    gl->glGenTextures(1, &resolveTexture);
    gl->glBindTexture(GL_TEXTURE_2D, resolveTexture);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);

    // check FBO status
    GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        ok = false;
        return;
    }

    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ok = true;
}

FBOHandle::~FBOHandle()
{
    gl->glDeleteRenderbuffers(1, &depthBuffer);
    gl->glDeleteTextures(1, &renderTexture);
    gl->glDeleteFramebuffers(1, &renderFBO);
    gl->glDeleteTextures(1, &resolveTexture);
    gl->glDeleteFramebuffers(1, &resolveFBO);
}

void FBOHandle::blitResolve()
{
    gl->glDisable(GL_MULTISAMPLE);
    gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, renderFBO);
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);

    gl->glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    gl->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0 );
}
