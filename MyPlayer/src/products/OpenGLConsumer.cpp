#include "OpenGLConsumer.h"
#include <QWindow>
#include "factories/GenericFactory.h"

//OpenGLConsumer::OpenGLConsumer(const std::string& name, const Properties& config):m_name(name), m_config(config) {
//}
OpenGLConsumer::OpenGLConsumer(const Properties& config,std::shared_ptr<EventDispatcher> dispatcher): m_config(config), m_dispatcher(dispatcher) {
    if (!initialize(m_config)) {
        std::cout << "have error in OpenGLConsumer initialize" << std::endl;
    }
}
OpenGLConsumer::~OpenGLConsumer() {
	if (m_isRunning) {
		stop();
	}
}

bool OpenGLConsumer::start() {
    if (!m_dispatcher) {
        return false;
    }
    try {
        m_isRunning = true;
        m_thread = std::thread(&OpenGLConsumer::processingLoop, this);
        m_dispatcher->subscribe("DecodedFrame", [this](std::shared_ptr<DecodedFrame> frame) {
                if (frame)
                    m_frameQueue.push(*frame);
            });
    }
    catch (const std::system_error& e) {
        m_isRunning = false;
        return false;
        std::cerr << "have some error in OpenGLConsumer start" << e.what() << std::endl;
    }
    return true;
}

bool OpenGLConsumer::isliving() {
    return m_isRunning;
}

bool OpenGLConsumer::initialize(const Properties& config) {
	if (config.find("render_window_handle") == config.end()) {
		std::cerr << "OpenGLConsumer initialize error: missing window handle" << std::endl;
		return false;
	}

	void* handle = std::any_cast<void*>(config.at("render_window_handle"));
	m_winId = reinterpret_cast<WId>(handle);
	if (m_winId == 0) {
		std::cerr << "OpenGLConsumer initialize error: invalid window handle\n";
		return false;
	}

	std::cout << " OpenGLQtConsumer success\n";
	return true;
}

void OpenGLConsumer::initializeOpenGl() {
    QOpenGLFunctions* f = m_context->functions();
    f->glEnable(GL_DEPTH_TEST);

    static const GLfloat vertices[]{
        -1.0f,-1.0f, -1.0f,+1.0f, +1.0f,+1.0f, +1.0f,-1.0f, // Position
         0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f, // UV
    };

    m_vbo = std::make_unique<QOpenGLBuffer>();
    m_vbo->create();
    m_vbo->bind();
    m_vbo->allocate(vertices, sizeof(vertices));

    m_program = std::make_unique<QOpenGLShaderProgram>();
    const char* vsrc =
        "attribute vec4 vertexIn;\n"
        "attribute vec2 textureIn;\n"
        "varying vec2 textureOut;\n"
        "void main(void) {\n"
        "    gl_Position = vertexIn;\n"
        "    textureOut = textureIn;\n"
        "}\n";
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc);

    const char* fsrc =
        "varying vec2 textureOut;\n"
        "uniform sampler2D tex_y;\n"
        "uniform sampler2D tex_u;\n"
        "uniform sampler2D tex_v;\n"
        "void main(void) {\n"
        "    vec3 yuv;\n"
        "    vec3 rgb;\n"
        "    yuv.x = texture2D(tex_y, textureOut).r;\n"
        "    yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
        "    yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
        "    rgb = mat3(1.0, 1.0, 1.0,\n"
        "               0.0, -0.344, 1.770,\n"
        "               1.403, -0.714, 0.0) * yuv;\n"
        "    gl_FragColor = vec4(rgb, 1.0);\n"
        "}\n";
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc);

    m_program->bindAttributeLocation("vertexIn", 0);
    m_program->bindAttributeLocation("textureIn", 1);
    m_program->link();
    m_program->bind();

    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    m_program->setAttributeBuffer(1, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    m_textureY = std::make_unique <QOpenGLTexture>(QOpenGLTexture::Target2D);
    m_textureU = std::make_unique <QOpenGLTexture>(QOpenGLTexture::Target2D);
    m_textureV = std::make_unique <QOpenGLTexture>(QOpenGLTexture::Target2D);
    m_textureY->create();
    m_textureU->create();
    m_textureV->create();

    m_program->setUniformValue("tex_y", 0);
    m_program->setUniformValue("tex_u", 1);
    m_program->setUniformValue("tex_v", 2);

    m_program->release();

    f->glClearColor(0.0, 0.0, 0.0, 0.0);
}

void OpenGLConsumer::processingLoop() {
	m_window = QWindow::fromWinId(m_winId);
	if (!m_window) {
		std::cerr << "QWindow::fromWinId failed. Is the handle valid?\n";
		return;
	}
    // 必须为窗口设置OpenGL Surface类型
	m_window->setSurfaceType(QWindow::OpenGLSurface);

    // 在渲染线程中创建上下文
    m_context = std::make_unique<QOpenGLContext>();
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    m_context->setFormat(format);
    if (!m_context->create()) {
        std::cerr << "QOpenGLContext create failed.\n";
        return;
    }

	if (!m_context->makeCurrent(m_window)) {
		std::cerr << "Render thread: m_context->makeCurrent() failed.\n";
		return;
	}
	initializeOpenGl();

	QOpenGLFunctions* f = m_context->functions();
    while (m_isRunning) {
        DecodedFrame frame;
        if (!m_frameQueue.pop(frame) || !m_isRunning) {
            break;
        }
        std::cout << "OpenGLConsumer paintOpenGL" << std::endl;
        if (!m_context->makeCurrent(m_window)) {
            std::cerr << "Render thread: m_context->makeCurrent() failed.\n";
            continue;
        }
        if (!m_vbo) {
            initializeOpenGl();
        }

        f->glViewport(0, 0, m_window->width(), m_window->height());
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        paintOpenGL(frame);
        m_context->swapBuffers(m_window);
        m_context->doneCurrent();
    }
    m_context->makeCurrent(m_window);
    m_vbo.reset();
    m_program.reset();
    m_textureY.reset();
    m_textureU.reset();
    m_textureV.reset();
    m_context->doneCurrent();
    std::cout << "OpenGLConsumer resources released in render thread.\n";
}

void OpenGLConsumer::paintOpenGL(const DecodedFrame& frame) {
    if (frame.data.empty()) return;

    m_program->bind();

    QOpenGLFunctions* f = m_context->functions();

    const unsigned char* yData = frame.data.data();
    const unsigned char* uData = yData + frame.width * frame.height;
    const unsigned char* vData = uData + (frame.width * frame.height / 4);

    f->glActiveTexture(GL_TEXTURE0);
    m_textureY->bind();
    if (frame.width != m_frameWidth || frame.height != m_frameHeight) {
        m_textureY->destroy();
        m_textureY->create();
        m_textureY->setSize(frame.width, frame.height);
        m_textureY->setFormat(QOpenGLTexture::R8_UNorm);
        m_textureY->allocateStorage();
        m_textureY->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureY->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureY->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    m_textureY->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, yData);

    f->glActiveTexture(GL_TEXTURE1);
    m_textureU->bind();
    if (frame.width != m_frameWidth || frame.height != m_frameHeight) {
        m_textureU->destroy();
        m_textureU->create();
        m_textureU->setSize(frame.width / 2, frame.height / 2);
        m_textureU->setFormat(QOpenGLTexture::R8_UNorm);
        m_textureU->allocateStorage();
        m_textureU->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureU->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureU->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    m_textureU->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, uData);

    f->glActiveTexture(GL_TEXTURE2);
    m_textureV->bind();
    if (frame.width != m_frameWidth || frame.height != m_frameHeight) {
        m_textureV->destroy();
        m_textureV->create();
        m_textureV->setSize(frame.width / 2, frame.height / 2);
        m_textureV->setFormat(QOpenGLTexture::R8_UNorm);
        m_textureV->allocateStorage();
        m_textureV->setMinificationFilter(QOpenGLTexture::Linear);
        m_textureV->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureV->setWrapMode(QOpenGLTexture::ClampToEdge);
    }
    m_textureV->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, vData);
    
    if (frame.width != m_frameWidth || frame.height != m_frameHeight) {
        m_frameWidth = frame.width;
        m_frameHeight = frame.height;
    }

    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_textureY->release();
    m_textureU->release();
    m_textureV->release();
    m_program->release();
}

void OpenGLConsumer::stop() {
	if (!m_isRunning) {
		return;
	}
	m_isRunning = false;
    m_frameQueue.shutdown();

	if (m_thread.joinable()) {
		m_thread.join();
	}
	std::cout << "OPenglConsumer have down" << std::endl;
}

namespace {
    Registrar<OpenGLConsumer, IDataConsumer, const Properties&, std::shared_ptr<EventDispatcher>> registrar("opengl_consumer");
}