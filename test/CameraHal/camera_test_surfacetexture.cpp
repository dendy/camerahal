#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <climits>

#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/ISurfaceComposerClient.h>
#include <surfaceflinger/SurfaceComposerClient.h>

#include <gui/SurfaceTexture.h>
#include <gui/SurfaceTextureClient.h>
#include <ui/GraphicBuffer.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <camera/CameraParameters.h>
#include <camera/ShotParameters.h>
#include <system/audio.h>
#include <system/camera.h>

#include <cutils/memory.h>
#include <utils/Log.h>

#include <sys/wait.h>

#include "camera_test.h"

#define ASSERT(X) \
    do { \
       if(!(X)) { \
           printf("error: %s():%d", __FUNCTION__, __LINE__); \
           return; \
       } \
    } while(0);

//temporarily define format here
#define HAL_PIXEL_FORMAT_TI_NV12 0x100
#define HAL_PIXEL_FORMAT_TI_NV12_1D 0x102

using namespace android;

static EGLint getSurfaceWidth() {
    return 512;
}

static EGLint getSurfaceHeight() {
    return 512;
}

static size_t calcBufSize(int format, int width, int height)
{
    int buf_size;

    switch (format) {
        case HAL_PIXEL_FORMAT_TI_NV12_1D:
        // add more formats later
        default:
            buf_size = width * height * 3 /2;
        break;
    }

    return buf_size;
}

void GLSurface::initialize(int display) {
    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_DISPLAY != mEglDisplay);

    EGLint majorVersion;
    EGLint minorVersion;
    ASSERT(eglInitialize(mEglDisplay, &majorVersion, &minorVersion));
    ASSERT(EGL_SUCCESS == eglGetError());

    EGLint numConfigs = 0;
    ASSERT(eglChooseConfig(mEglDisplay, getConfigAttribs(), &mGlConfig,
                1, &numConfigs));
    ASSERT(EGL_SUCCESS == eglGetError());

    if (display) {
        mComposerClient = new SurfaceComposerClient;
        ASSERT(NO_ERROR == mComposerClient->initCheck());
        mSurfaceControl = mComposerClient->createSurface(
                String8("Test Surface"), 0,
                800, 480, HAL_PIXEL_FORMAT_YCrCb_420_SP, 0);

        ASSERT(mSurfaceControl != NULL);
        ASSERT(mSurfaceControl->isValid());

        SurfaceComposerClient::openGlobalTransaction();
        ASSERT(NO_ERROR == mSurfaceControl->setLayer(0x7FFFFFFF));
        ASSERT(NO_ERROR == mSurfaceControl->show());
        SurfaceComposerClient::closeGlobalTransaction();

        sp<ANativeWindow> window = mSurfaceControl->getSurface();
        mEglSurface = eglCreateWindowSurface(mEglDisplay, mGlConfig,
                window.get(), NULL);
    } else {
        EGLint pbufferAttribs[] = {
            EGL_WIDTH, getSurfaceWidth(),
            EGL_HEIGHT, getSurfaceHeight(),
            EGL_NONE };
        mEglSurface = eglCreatePbufferSurface(mEglDisplay, mGlConfig,
                pbufferAttribs);
    }
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_SURFACE != mEglSurface);

    mEglContext = eglCreateContext(mEglDisplay, mGlConfig, EGL_NO_CONTEXT,
            getContextAttribs());
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(EGL_NO_CONTEXT != mEglContext);

    ASSERT(eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface,
            mEglContext));
    ASSERT(EGL_SUCCESS == eglGetError());

    EGLint w, h;
    ASSERT(eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &w));
    ASSERT(EGL_SUCCESS == eglGetError());
    ASSERT(eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &h));
    ASSERT(EGL_SUCCESS == eglGetError());

    glViewport(0, 0, w, h);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
}

void GLSurface::deinit() {
    if (mComposerClient != NULL) {
        mComposerClient->dispose();
    }

    if (mEglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(mEglDisplay, mEglContext);
    }

    if (mEglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(mEglDisplay, mEglSurface);
    }
    if (mEglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
               EGL_NO_CONTEXT);
        eglTerminate(mEglDisplay);
    }
    ASSERT(EGL_SUCCESS == eglGetError());
}

EGLint const* GLSurface::getConfigAttribs() {
    static EGLint sDefaultConfigAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE };

    return sDefaultConfigAttribs;
}

EGLint const* GLSurface::getContextAttribs() {
    static EGLint sDefaultContextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE };

    return sDefaultContextAttribs;
}

void GLSurface::loadShader(GLenum shaderType, const char* pSource, GLuint* outShader) {
    GLuint shader = glCreateShader(shaderType);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glCompileShader(shader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            ASSERT(GLenum(GL_NO_ERROR) == glGetError());
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                }
            } else {
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    printf("Shader compile log:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    ASSERT(shader != 0);
    *outShader = shader;
}

void GLSurface::createProgram(const char* pVertexSource, const char* pFragmentSource,
            GLuint* outPgm) {
    GLuint vertexShader, fragmentShader;
    {
        loadShader(GL_VERTEX_SHADER, pVertexSource, &vertexShader);
    }
    {
        loadShader(GL_FRAGMENT_SHADER, pFragmentSource, &fragmentShader);
    }

    GLuint program = glCreateProgram();
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    if (program) {
        glAttachShader(program, vertexShader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glAttachShader(program, fragmentShader);
        ASSERT(GLenum(GL_NO_ERROR) == glGetError());
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    printf("Program link log:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    ASSERT(program != 0);
    *outPgm = program;
}

// SurfaceTexture specific
sp<SurfaceTexture> SurfaceTextureBase::getST() {
     return mST;
}

void SurfaceTextureBase::initialize(int tex_id, EGLenum tex_target) {
    mTexId = tex_id;
    mST = new SurfaceTexture(tex_id, true, tex_target);
    mSTC = new SurfaceTextureClient(mST);
    mANW = mSTC;
}

void SurfaceTextureBase::deinit() {
    mANW.clear();
    mSTC.clear();
    mST.clear();
}

// SurfaceTexture with GL specific

void SurfaceTextureGL::initialize(int display, int tex_id) {
    GLSurface::initialize(display);
    SurfaceTextureBase::initialize(tex_id, GL_TEXTURE_EXTERNAL_OES);

    const char vsrc[] =
        "attribute vec4 vPosition;\n"
        "varying vec2 texCoords;\n"
        "uniform mat4 texMatrix;\n"
        "void main() {\n"
        "  vec2 vTexCoords = 0.5 * (vPosition.xy + vec2(1.0, 1.0));\n"
        "  texCoords = (texMatrix * vec4(vTexCoords, 0.0, 1.0)).xy;\n"
        "  gl_Position = vPosition;\n"
        "}\n";

    const char fsrc[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES texSampler;\n"
        "varying vec2 texCoords;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(texSampler, texCoords);\n"
        "}\n";

    {
        createProgram(vsrc, fsrc, &mPgm);
    }

    mPositionHandle = glGetAttribLocation(mPgm, "vPosition");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mPositionHandle);
    mTexSamplerHandle = glGetUniformLocation(mPgm, "texSampler");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mTexSamplerHandle);
    mTexMatrixHandle = glGetUniformLocation(mPgm, "texMatrix");
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    ASSERT(-1 != mTexMatrixHandle);
}

void SurfaceTextureGL::deinit() {
    SurfaceTextureBase::deinit();
    GLSurface::deinit();
}

// drawTexture draws the SurfaceTexture over the entire GL viewport.
void SurfaceTextureGL::drawTexture() {
    const GLfloat triangleVertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
    };

    glVertexAttribPointer(mPositionHandle, 2, GL_FLOAT, GL_FALSE, 0,
            triangleVertices);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glEnableVertexAttribArray(mPositionHandle);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    glUseProgram(mPgm);
    glUniform1i(mTexSamplerHandle, 0);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexId);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    // XXX: These calls are not needed for GL_TEXTURE_EXTERNAL_OES as
    // they're setting the defautls for that target, but when hacking things
    // to use GL_TEXTURE_2D they are needed to achieve the same behavior.
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    GLfloat texMatrix[16];
    mST->getTransformMatrix(texMatrix);
    glUniformMatrix4fv(mTexMatrixHandle, 1, GL_FALSE, texMatrix);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    ASSERT(GLenum(GL_NO_ERROR) == glGetError());

    eglSwapBuffers(mEglDisplay, mEglSurface);
}

// buffer source stuff
void BufferSourceThread::handleBuffer(sp<GraphicBuffer> &graphic_buffer) {
    int size;
    int fd = -1;
    char fn[256];
    uint8_t *buffer = NULL;

    if (!graphic_buffer.get()) {
        return;
    }

    graphic_buffer->lock(GRALLOC_USAGE_SW_READ_RARELY, (void**) &buffer);

    size = calcBufSize((int)graphic_buffer->getPixelFormat(),
                              graphic_buffer->getWidth(),
                              graphic_buffer->getHeight());
    if ((size <= 0) || !buffer) {
        goto out;
    }

    fn[0] = 0;
    sprintf(fn, "/sdcard/img%03d.raw", mCounter);
    fd = open(fn, O_CREAT | O_WRONLY | O_TRUNC, 0777);

    if (fd < 0)
        goto out;

    if (size != write(fd, buffer, size))
        printf("Bad Write int a %s error (%d)%s\n", fn, errno, strerror(errno));

    printf("%s: buffer=%08X, size=%d stored at %s\n",
           __FUNCTION__, (int)buffer, size, fn);
out:
    if (fd >= 0)
        close(fd);

    graphic_buffer->unlock();
}
