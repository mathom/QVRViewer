#include "vrview.h"
#include <QGL>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include "modelFormats.h"

#define NEAR_CLIP 0.1f
#define FAR_CLIP 10000.0f

VRView::VRView(QWidget *parent) : QOpenGLWidget(parent),
    m_hmd(0), m_texture(0), m_vertCount(0),
    m_eyeWidth(0), m_eyeHeight(0), m_leftBuffer(0), m_rightBuffer(0),
    m_frames(0), m_mode(None)
{
    memset(m_inputNext, 0, sizeof(m_inputNext));
    memset(m_inputNext, 0, sizeof(m_inputPrev));

    QSizePolicy size;
    size.setHorizontalPolicy(QSizePolicy::Expanding);
    size.setVerticalPolicy(QSizePolicy::Expanding);
    setSizePolicy(size);

    QTimer *fpsTimer = new QTimer(this);
    connect(fpsTimer, SIGNAL(timeout()), this, SLOT(updateFramerate()));
    fpsTimer->start(1000);
}

VRView::~VRView()
{
    shutdown();
}

void VRView::loadPanorama(const QString &fileName, VRMode mode)
{
    QFileInfo info(fileName);

    if (info.exists())
    {
        qDebug() << "loading" << fileName;
        delete m_texture;
        m_texture = new QOpenGLTexture(QImage(fileName).mirrored(true, true));
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        qDebug() << "loaded texture" << m_texture->width() << "x" << m_texture->height();
        emit statusMessage(tr("Loaded %1 (%2x%3)").arg(info.fileName())
                           .arg(m_texture->width()).arg(m_texture->height()));
        m_mode = mode;

        m_currentImage = fileName;
    }
}

void VRView::loadImageRelative(int offset)
{
    QFileInfo info(m_currentImage);

    if (info.exists())
    {
        QDir dir = info.dir().path();
        QStringList filters;
        filters << "*.jpg" << "*.png";
        QFileInfoList files = dir.entryInfoList(filters, QDir::NoDotAndDotDot|QDir::Files);

        int index = files.indexOf(info);

        if (offset < 0)
            offset += files.length();

        QFileInfo selected = files.at((index+offset)%files.length());
        qDebug() << "loading relative image" << selected.fileName();
        loadPanorama(selected.absoluteFilePath(), m_mode);
    }
}

QSize VRView::minimumSizeHint() const
{
    return QSize(1,1);
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
    delete m_resolveBuffer;

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

#ifdef QT_DEBUG
    m_logger = new QOpenGLDebugLogger(this);

    connect(m_logger, SIGNAL(messageLogged(QOpenGLDebugMessage)),
             this, SLOT(debugMessage(QOpenGLDebugMessage)), Qt::DirectConnection);

    if (m_logger->initialize())
    {
        m_logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        m_logger->enableMessages();
    }
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // compile our shader
    compileShader(m_shader, ":/shaders/unlit.vert", ":/shaders/unlit.frag");

    // build out sample geometry
    m_vao.create();
    m_vao.bind();

    QVector<GLfloat> points = readObj(":/models/sphere.obj");
    m_vertCount = points.length();
    qDebug() << "loaded" << m_vertCount << "verts";

    m_vertexBuffer.create();
    m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vertexBuffer.bind();

    m_vertexBuffer.allocate(points.data(), points.length() * sizeof(GLfloat));

    m_shader.bind();

    m_shader.setAttributeBuffer("vertex", GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    m_shader.enableAttributeArray("vertex");

    m_shader.setAttributeBuffer("texCoord", GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
    m_shader.enableAttributeArray("texCoord");

    m_shader.setUniformValue("diffuse", 0);

    m_texture = new QOpenGLTexture(QImage(":/textures/uvmap.png"));

    initVR();
}

void VRView::paintGL()
{ 
    if (m_hmd)
    {
        updatePoses();
        updateInput();

        glClearColor(0.15f, 0.15f, 0.18f, 1.0f);
        glViewport(0, 0, m_eyeWidth, m_eyeHeight);

        QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);

        glEnable(GL_MULTISAMPLE);
        m_leftBuffer->bind();
        renderEye(vr::Eye_Left);
        m_leftBuffer->release();

        QRect targetLeft(0, 0, m_eyeWidth, m_eyeHeight);
        QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetLeft,
                                                  m_leftBuffer, sourceRect);

        glEnable(GL_MULTISAMPLE);
        m_rightBuffer->bind();
        renderEye(vr::Eye_Right);
        m_rightBuffer->release();
        QRect targetRight(m_eyeWidth, 0, m_eyeWidth, m_eyeHeight);
        QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetRight,
                                                  m_rightBuffer, sourceRect);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, width(), height());
    glDisable(GL_MULTISAMPLE);
    renderEye(vr::Eye_Right);

    if (m_hmd)
    {
        vr::VRTextureBounds_t leftRect = { 0.0f, 0.0f, 0.5f, 1.0f };
        vr::VRTextureBounds_t rightRect = { 0.5f, 0.0f, 1.0f, 1.0f };
        vr::Texture_t composite = { (void*)m_resolveBuffer->texture(), vr::API_OpenGL, vr::ColorSpace_Gamma };

        vr::VRCompositor()->Submit(vr::Eye_Left, &composite, &leftRect);
        vr::VRCompositor()->Submit(vr::Eye_Right, &composite, &rightRect);
    }

    //vr::VRCompositor()->PostPresentHandoff();

    m_frames++;

    update();
}

void VRView::renderEye(vr::Hmd_Eye eye)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_vao.bind();
    m_shader.bind();
    m_texture->bind(0);

    m_shader.setUniformValue("transform", viewProjection(eye));
    m_shader.setUniformValue("leftEye", eye==vr::Eye_Left);
    m_shader.setUniformValue("overUnder", m_mode==VRView::OverUnder);
    glDrawArrays(GL_TRIANGLES, 0, m_vertCount);
}

void VRView::resizeGL(int, int)
{
    // do nothing
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

    QString ident;
    ident.append("QVRViewer - ");
    ident.append(getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String));
    ident.append(" ");
    ident.append(getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String));
    emit deviceIdentifier(ident);

    // setup frame buffers for eyes
    m_hmd->GetRecommendedRenderTargetSize(&m_eyeWidth, &m_eyeHeight);

    QOpenGLFramebufferObjectFormat buffFormat;
    buffFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    buffFormat.setInternalTextureFormat(GL_RGBA8);
    buffFormat.setSamples(4);

    m_leftBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);
    m_rightBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);

    QOpenGLFramebufferObjectFormat resolveFormat;
    resolveFormat.setInternalTextureFormat(GL_RGBA8);
    buffFormat.setSamples(0);

    m_resolveBuffer = new QOpenGLFramebufferObject(m_eyeWidth*2, m_eyeHeight, resolveFormat);

    // turn on compositor
    if (!vr::VRCompositor())
    {
        QString message = "Compositor initialization failed. See log file for details";
        qCritical() << message;
        QMessageBox::critical(this, "Unable to init VR", message);
        return;
    }

#ifdef QT_DEBUG
    vr::VRCompositor()->ShowMirrorWindow();
#endif
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

void VRView::updateInput()
{
    vr::VREvent_t event;
    while(m_hmd->PollNextEvent(&event, sizeof(event)))
    {
        //ProcessVREvent( event );
    }

    for(vr::TrackedDeviceIndex_t i=0; i<vr::k_unMaxTrackedDeviceCount; i++ )
    {
        vr::VRControllerState_t state;
        if(m_hmd->GetControllerState(i, &state))
        {
            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad))
            {
                if (!m_inputNext[i])
                {
                    loadImageRelative(1);
                    m_inputNext[i] = true;
                }
            }
            else if (m_inputNext[i])
            {
                m_inputNext[i] = false;
            }

            if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip))
            {
                if (!m_inputPrev[i])
                {
                    loadImageRelative(-1);
                    m_inputPrev[i] = true;
                }
            }
            else if (m_inputPrev[i])
            {
                m_inputPrev[i] = false;
            }
        }
    }

}

bool VRView::compileShader(QOpenGLShaderProgram &shader, const QString &vertexShaderPath, const QString &fragmentShaderPath)
{
    bool result = shader.addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath);
    if (!result)
        qCritical() << shader.log();

    result = shader.addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderPath);
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
    s.scale(1000.0f);

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
