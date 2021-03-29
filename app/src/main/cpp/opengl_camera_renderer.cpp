#include <jni.h>
#include <android/log.h>
#include <android/window.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <vector>

namespace {
    // 顶点坐标程序
    const char *VERTEX_SHADER_SRC = R"SRC(
        #version 300 es
        // 顶点坐标
        layout (location = 0) in vec2 aPos;
        // 纹理坐标
        layout (location = 1) in vec2 aTexCoord;
        // 纹理坐标输出
        out vec2 TexCoord;
         uniform mat4 vMatrix;

        void main(){
            gl_Position = vec4(aPos,0.0,1.0);
            TexCoord =  (vMatrix * vec4( aTexCoord,0.0,1.0)).xy;
        }
    )SRC";
    // 片元着色器程序
    const char *FRAGMENT_SHADER_SRC = R"SRC(
        #version 300 es
        // 处理相机采集的纹理数据
        #extension GL_OES_EGL_image_external_essl3 :require
        in vec2 TexCoord;
        uniform samplerExternalOES vTexture;
        out vec4 FragColor;
        void main(){
            // 对纹理进行采样输出
            FragColor = texture(vTexture,TexCoord)+vec4(0.2,0.2,0.0,0.0);
//            FragColor = vec4(1.0,0.0,0.0,1.0);
        }
    )SRC";

    struct NativeContext {
        EGLDisplay eglDisplay;
        EGLConfig eglConfig;
        EGLContext eglContext;
        std::pair<ANativeWindow *, EGLSurface> windowSurface;
        GLuint programId;
        GLint samplerHandle;
        GLuint textureId;

        NativeContext(EGLDisplay display, EGLConfig config, EGLContext context,
                      ANativeWindow *window, EGLSurface eglSurface) :
                eglDisplay(display), eglConfig(config), eglContext(context),
                windowSurface(std::make_pair(window, eglSurface)),
                programId(0), samplerHandle(-1), textureId(0) {}
    };


}

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    return JNI_VERSION_1_6;
}

JNIEXPORT jlong JNICALL
Java_com_cci_glcamerafilters_OpenGLRenderer_initContext(JNIEnv *env, jclass clazz) {
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint majorVer;
    EGLint minorVer;
    eglInitialize(eglDisplay, &majorVer, &minorVer);

    EGLConfig confg;
    EGLint confNum;
    int configAttris[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_NONE
    };

    eglChooseConfig(eglDisplay, configAttris, &confg, 1, &confNum);

    int contextAttris[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    EGLContext eglContext = eglCreateContext(eglDisplay, confg, EGL_NO_CONTEXT, contextAttris);

    auto *nativeContext = new NativeContext(eglDisplay, confg, eglContext, nullptr, nullptr);

    eglMakeCurrent(nativeContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext);
    // 创建程序
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SRC, nullptr);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_SRC, nullptr);
    glCompileShader(fragmentShader);
    nativeContext->programId = glCreateProgram();
    glAttachShader(nativeContext->programId, vertexShader);
    glAttachShader(nativeContext->programId, fragmentShader);
    glLinkProgram(nativeContext->programId);

    glGenTextures(1, &(nativeContext->textureId));

    return reinterpret_cast<jlong>(nativeContext);
}


JNIEXPORT void JNICALL
Java_com_cci_glcamerafilters_OpenGLRenderer_setWindowSurface(JNIEnv *env, jclass clazz,
                                                             jlong native_context,
                                                             jobject surface) {

    auto *nativeContext = reinterpret_cast<NativeContext *>(native_context);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    EGLSurface eglSurface = eglCreateWindowSurface(nativeContext->eglDisplay,
                                                   nativeContext->eglConfig, nativeWindow,
                                                   nullptr);
    nativeContext->windowSurface = std::make_pair(nativeWindow, eglSurface);

//    eglMakeCurrent(nativeContext->eglDisplay, eglSurface, eglSurface, nativeContext->eglContext);


    glViewport(0, 0, ANativeWindow_getWidth(nativeWindow), ANativeWindow_getHeight(nativeWindow));
//    glScissor(0,0,ANativeWindow_getWidth(nativeWindow),ANativeWindow_getHeight(nativeWindow));

}
JNIEXPORT jint JNICALL
Java_com_cci_glcamerafilters_OpenGLRenderer_getTexName(JNIEnv *env, jclass clazz,
                                                       jlong native_context) {
    return reinterpret_cast<NativeContext *>(native_context)->textureId;
}

JNIEXPORT void JNICALL
Java_com_cci_glcamerafilters_OpenGLRenderer_renderTexture(JNIEnv *env, jclass clazz,
                                                          jlong native_context, jlong timestamp,
                                                          jfloatArray jtexTransformArray) {
    GLfloat *texTransformArray =
            env->GetFloatArrayElements(jtexTransformArray, nullptr);
    auto *nativeContext = reinterpret_cast<NativeContext *>(native_context);
    eglMakeCurrent(nativeContext->eglDisplay, nativeContext->windowSurface.second,
                   nativeContext->windowSurface.second, nativeContext->eglContext);

    constexpr GLfloat vertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, 1.0f
    };
    constexpr GLfloat texCoords[] = {
            0.0f, 0.0f, // Lower-left
            1.0f, 0.0f, // Lower-right
            0.0f, 1.0f, // Upper-left (order must match the vertices)
            1.0f, 1.0f  // Upper-right
    };


    glUseProgram(nativeContext->programId);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), vertices);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), texCoords);
    glEnableVertexAttribArray(1);


    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeContext->textureId);
//    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glFrontFace(GL_CW);
    glUniform1i(glGetUniformLocation(nativeContext->programId, "vTexture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(nativeContext->programId, "vMatrix"), 1, GL_FALSE,
                       texTransformArray);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, nativeContext->textureId);


    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(nativeContext->eglDisplay, nativeContext->windowSurface.second);
    env->ReleaseFloatArrayElements(jtexTransformArray, texTransformArray,
                                   JNI_ABORT);
}


// 释放资源
JNIEXPORT void JNICALL
Java_com_cci_glcamerafilters_OpenGLRenderer_closeContext(JNIEnv *env, jclass clazz,
                                                         jlong native_context) {

    auto *nativeContext = reinterpret_cast<NativeContext *>(native_context);
    if (nativeContext->programId) {
        glDeleteProgram(nativeContext->programId);
        nativeContext->programId = 0;
    }

    eglDestroySurface(nativeContext->eglDisplay, nativeContext->windowSurface.second);
    nativeContext->windowSurface.second = nullptr;
    ANativeWindow_release(nativeContext->windowSurface.first);
    nativeContext->windowSurface.first = nullptr;

    eglMakeCurrent(nativeContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(nativeContext->eglDisplay);
    delete nativeContext;
}
}
