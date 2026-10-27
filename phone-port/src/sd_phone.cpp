/* Flat-screen Android entry point. No OpenXR runtime or headset APIs are used. */
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <signal.h>
#include <ucontext.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <GL/gl.h>

#include <SDL.h>
#include <gl4esinit.h>

#include "phone_input_adapter.h"
#include "vr_audio.h"
#include "vr_events.h"
#include "vr_paths.h"

#define PHONE_TAG "SpeedDreamsPhone"
#define PLOGI(...) __android_log_print(ANDROID_LOG_INFO, PHONE_TAG, __VA_ARGS__)
#define PLOGW(...) __android_log_print(ANDROID_LOG_WARN, PHONE_TAG, __VA_ARGS__)
#define PLOGE(...) __android_log_print(ANDROID_LOG_ERROR, PHONE_TAG, __VA_ARGS__)

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

extern int SdMain(int argc, char* argv[]);

struct PhoneAppState {
    JavaVM* vm = nullptr;
    jobject activity = nullptr;
    jmethodID shutdownMethod = nullptr;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    ANativeWindow* window = nullptr; // owns one reference while non-null
    pthread_t thread{};
    bool threadStarted = false;
    bool threadFinished = false;
    bool paused = false;
    bool quitting = false;
    char baseDir[1024] = "/sdcard/SpeedDreamsVR";
};

static void phoneFatalSignalHandler(int signalNumber, siginfo_t* info, void* context)
{
    uintptr_t pc = 0;
    if (context) {
        auto* userContext = static_cast<ucontext_t*>(context);
#if defined(__aarch64__)
        pc = static_cast<uintptr_t>(userContext->uc_mcontext.pc);
#elif defined(__arm__)
        pc = static_cast<uintptr_t>(userContext->uc_mcontext.arm_pc);
#elif defined(__x86_64__)
        pc = static_cast<uintptr_t>(userContext->uc_mcontext.gregs[REG_RIP]);
#elif defined(__i386__)
        pc = static_cast<uintptr_t>(userContext->uc_mcontext.gregs[REG_EIP]);
#endif
    }
    Dl_info symbolInfo{};
    const bool hasSymbol = pc != 0 &&
        dladdr(reinterpret_cast<void*>(pc), &symbolInfo) != 0;
    if (hasSymbol) {
        __android_log_print(ANDROID_LOG_ERROR, "SpeedDreamsCrash",
            "FATAL native signal=%d address=%p pc=%p object=%s symbol=%s offset=0x%lx pid=%d tid=%ld",
            signalNumber, info ? info->si_addr : nullptr, reinterpret_cast<void*>(pc),
            symbolInfo.dli_fname ? symbolInfo.dli_fname : "?",
            symbolInfo.dli_sname ? symbolInfo.dli_sname : "?",
            symbolInfo.dli_saddr ? static_cast<unsigned long>(pc -
                reinterpret_cast<uintptr_t>(symbolInfo.dli_saddr)) : 0UL,
            getpid(), (long)gettid());
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "SpeedDreamsCrash",
            "FATAL native signal=%d address=%p pc=%p pid=%d tid=%ld",
            signalNumber, info ? info->si_addr : nullptr,
            reinterpret_cast<void*>(pc), getpid(), (long)gettid());
    }
    signal(signalNumber, SIG_DFL);
    raise(signalNumber);
}

static void installPhoneCrashHandler()
{
    struct sigaction action{};
    action.sa_sigaction = phoneFatalSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
}

static PhoneAppState* gPhone = nullptr;
static EGLDisplay gDisplay = EGL_NO_DISPLAY;
static EGLConfig gConfig = nullptr;
static EGLContext gContext = EGL_NO_CONTEXT;
static EGLSurface gSurface = EGL_NO_SURFACE;
static ANativeWindow* gEglWindow = nullptr;
static int gSurfaceWidth = 0;
static int gSurfaceHeight = 0;

static void detachEglSurface()
{
    if (gDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(gDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (gSurface != EGL_NO_SURFACE) eglDestroySurface(gDisplay, gSurface);
    }
    gSurface = EGL_NO_SURFACE;
    gSurfaceWidth = 0;
    gSurfaceHeight = 0;
    if (gEglWindow) {
        ANativeWindow_release(gEglWindow);
        gEglWindow = nullptr;
    }
}

static void destroyEgl()
{
    detachEglSurface();
    if (gDisplay != EGL_NO_DISPLAY) {
        if (gContext != EGL_NO_CONTEXT) eglDestroyContext(gDisplay, gContext);
        eglTerminate(gDisplay);
    }
    gDisplay = EGL_NO_DISPLAY;
    gConfig = nullptr;
    gContext = EGL_NO_CONTEXT;
}

static void setPhoneState(bool paused, bool quitting)
{
    PhoneAppState* state = gPhone;
    if (!state) return;
    pthread_mutex_lock(&state->mutex);
    state->paused = paused;
    if (quitting) state->quitting = true;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
}

static bool waitForWindowAndAcquire(ANativeWindow** result)
{
    *result = nullptr;
    PhoneAppState* state = gPhone;
    if (!state) return false;
    pthread_mutex_lock(&state->mutex);
    while (!state->quitting && (state->paused || state->window == nullptr)) {
        pthread_cond_wait(&state->cond, &state->mutex);
    }
    if (state->quitting) {
        pthread_mutex_unlock(&state->mutex);
        return false;
    }
    *result = state->window;
    ANativeWindow_acquire(*result);
    pthread_mutex_unlock(&state->mutex);
    return true;
}

static bool createEglContext()
{
    if (gDisplay != EGL_NO_DISPLAY && gContext != EGL_NO_CONTEXT && gConfig != nullptr)
        return true;
    // Clean partially initialized EGL state before retrying; otherwise repeated
    // eglInitialize calls can leak display reference counts after driver failures.
    destroyEgl();
    gDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gDisplay == EGL_NO_DISPLAY) {
        PLOGE("eglGetDisplay failed: 0x%x", eglGetError());
        return false;
    }
    EGLint major = 0, minor = 0;
    if (eglInitialize(gDisplay, &major, &minor) != EGL_TRUE) {
        PLOGE("eglInitialize failed: 0x%x", eglGetError());
        gDisplay = EGL_NO_DISPLAY;
        return false;
    }

    const EGLint attrs3[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE
    };
    EGLint count = 0;
    if (eglChooseConfig(gDisplay, attrs3, &gConfig, 1, &count) != EGL_TRUE || count < 1) {
        const EGLint attrs2[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE
        };
        if (eglChooseConfig(gDisplay, attrs2, &gConfig, 1, &count) != EGL_TRUE || count < 1) {
            PLOGE("No compatible EGL window config: 0x%x", eglGetError());
            destroyEgl();
            return false;
        }
    }
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        PLOGE("eglBindAPI failed: 0x%x", eglGetError());
        destroyEgl();
        return false;
    }

    const EGLint ctx3[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    gContext = eglCreateContext(gDisplay, gConfig, EGL_NO_CONTEXT, ctx3);
    if (gContext == EGL_NO_CONTEXT) {
        const EGLint ctx2[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        gContext = eglCreateContext(gDisplay, gConfig, EGL_NO_CONTEXT, ctx2);
    }
    if (gContext == EGL_NO_CONTEXT) {
        PLOGE("eglCreateContext (ES3/ES2) failed: 0x%x", eglGetError());
        destroyEgl();
        return false;
    }
    return true;
}

extern "C" int SDPhoneEnsureSurface(void)
{
    unsigned backoffMs = 40;
    for (;;) {
        ANativeWindow* desired = nullptr;
        if (!waitForWindowAndAcquire(&desired)) return 0;

        if (!createEglContext()) {
            ANativeWindow_release(desired);
            usleep(backoffMs * 1000);
            if (backoffMs < 1000) backoffMs = std::min(backoffMs * 2, 1000u);
            continue;
        }
        backoffMs = 40;

        if (desired != gEglWindow || gSurface == EGL_NO_SURFACE) {
            detachEglSurface();
            gEglWindow = desired; // transfer the acquired reference to this thread
            gSurface = eglCreateWindowSurface(gDisplay, gConfig, gEglWindow, nullptr);
            if (gSurface == EGL_NO_SURFACE) {
                PLOGE("eglCreateWindowSurface failed: 0x%x", eglGetError());
                ANativeWindow_release(gEglWindow);
                gEglWindow = nullptr;
                usleep(100000);
                continue;
            }
        } else {
            ANativeWindow_release(desired);
        }

        if (eglMakeCurrent(gDisplay, gSurface, gSurface, gContext) != EGL_TRUE) {
            PLOGE("eglMakeCurrent failed: 0x%x", eglGetError());
            detachEglSurface();
            usleep(100000);
            continue;
        }
        gSurfaceWidth = ANativeWindow_getWidth(gEglWindow);
        gSurfaceHeight = ANativeWindow_getHeight(gEglWindow);
        if (gSurfaceWidth > 0 && gSurfaceHeight > 0) return 1;
        PLOGW("Waiting for a valid surface size (%dx%d)", gSurfaceWidth, gSurfaceHeight);
        usleep(100000);
    }
}

static void phoneGetMainFramebufferSize(int* width, int* height)
{
    SDPhoneGetSurfaceSize(width, height);
}
static void* phoneGetProcAddress(const char* name)
{
    return reinterpret_cast<void*>(eglGetProcAddress(name));
}
static void initializeGl4es()
{
    setenv("LIBGL_ES", "2", 1);
    setenv("LIBGL_GL", "21", 1);
    setenv("LIBGL_NOBANNER", "1", 1);
    setenv("LIBGL_MIPMAP", "3", 1);
    setenv("LIBGL_NPOT", "1", 1);
    setenv("LIBGL_FB", "0", 1);
    setenv("LIBGL_NOERROR", "1", 1);
    setenv("LIBGL_PSA_FOLDER", gPhone ? gPhone->baseDir : "/sdcard/SpeedDreamsVR", 1);
    set_getprocaddress(phoneGetProcAddress);
    set_getmainfbsize(phoneGetMainFramebufferSize);
    initialize_gl4es();
    const auto* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    PLOGI("gl4es initialized; GL_VERSION=%s", version ? version : "(unavailable)");
}

extern "C" void SDPhoneGetSurfaceSize(int* width, int* height)
{
    int w = 0, h = 0;
    PhoneAppState* state = gPhone;
    if (state) {
        pthread_mutex_lock(&state->mutex);
        if (!state->paused && state->window) {
            w = ANativeWindow_getWidth(state->window);
            h = ANativeWindow_getHeight(state->window);
        }
        pthread_mutex_unlock(&state->mutex);
    }
    if (width) *width = w;
    if (height) *height = h;
}

extern "C" int SDPhonePresent(void)
{
    if (!SDPhoneEnsureSurface()) return -1;
    if (eglSwapBuffers(gDisplay, gSurface) != EGL_TRUE) {
        PLOGW("eglSwapBuffers failed: 0x%x display=%p surface=%p", eglGetError(), (void*)gDisplay, (void*)gSurface);
        detachEglSurface();
        return -1;
    }
    return 0;
}

static void callActivityShutdown(PhoneAppState* state)
{
    if (!state || !state->vm || !state->activity || !state->shutdownMethod) return;
    JNIEnv* env = nullptr;
    bool attached = false;
    if (state->vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (state->vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    env->CallVoidMethod(state->activity, state->shutdownMethod);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (attached) state->vm->DetachCurrentThread();
}

extern "C" void VR_Shutdown(void)
{
    PhoneAppState* state = gPhone;
    setPhoneState(true, true);
    callActivityShutdown(state);
}

static void* phoneMainThread(void* argument)
{
    installPhoneCrashHandler();
    auto* state = static_cast<PhoneAppState*>(argument);
    PLOGI("BOOT stage=thread-start pid=%d tid=%ld", getpid(), (long)gettid());
    prctl(PR_SET_NAME, "SDPhone_Game", 0, 0, 0);
    VrLogRedirectStdio();
    setenv("SDL_VIDEODRIVER", "dummy", 1);
    setenv("SDL_AUDIODRIVER", "openslES", 1);
    setenv("SDVR_DATA_DIR", state->baseDir, 1);

    PLOGI("BOOT stage=waiting-for-egl-surface");
    if (SDPhoneEnsureSurface()) {
        PLOGI("BOOT stage=egl-surface-ready size=%dx%d", gSurfaceWidth, gSurfaceHeight);
        initializeGl4es();
        PLOGI("BOOT stage=gl4es-initialized");
        const int pathsStatus = VrPathsInit(state->baseDir);
        PLOGI("BOOT stage=paths-init result=%d base=%s", pathsStatus, state->baseDir);
        if (pathsStatus == 0) {
            PLOGI("BOOT stage=audio-init-begin");
            VrAudioInit();
            PLOGI("BOOT stage=audio-init-done");
            PLOGI("BOOT stage=sdl-main-ready-begin");
            SDL_SetMainReady();
            PLOGI("BOOT stage=sdl-main-ready-done");
            static char arg0[] = "speed-dreams-2";
            static char optDD[] = "-dd";
            static char optLC[] = "-lc";
            static char optLD[] = "-ld";
            static char optBD[] = "-bd";
            char dataDir[1024], localDir[1024], baseDir[1024];
            snprintf(dataDir, sizeof(dataDir), "%s", VrDataDir());
            snprintf(localDir, sizeof(localDir), "%s", VrLocalDir());
            snprintf(baseDir, sizeof(baseDir), "%s", VrBaseDir());
            char* argv[] = {arg0, optDD, dataDir, optLC, localDir, optLD, dataDir, optBD, baseDir, nullptr};
            PLOGI("BOOT stage=sdmain-enter data=%s local=%s base=%s", dataDir, localDir, baseDir);
            SdMain(9, argv);
            PLOGI("BOOT stage=sdmain-return");
            VrAudioShutdown();
            PLOGI("BOOT stage=audio-shutdown-done");
        } else {
            PLOGE("Game data missing at %s/data; include a legal data package in the APK", state->baseDir);
        }
    }

    PLOGI("BOOT stage=thread-cleanup-begin");
    destroyEgl();
    PLOGI("BOOT stage=thread-cleanup-egl-done");
    pthread_mutex_lock(&state->mutex);
    state->threadFinished = true;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    return nullptr;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*)
{
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_speeddreamsvr_SDVRLib_onCreate(JNIEnv* env, jclass, jobject activity, jstring dataDir)
{
    if (gPhone != nullptr) return 0;
    auto* state = new PhoneAppState();
    pthread_mutex_init(&state->mutex, nullptr);
    pthread_cond_init(&state->cond, nullptr);
    env->GetJavaVM(&state->vm);
    state->activity = env->NewGlobalRef(activity);
    jclass activityClass = env->GetObjectClass(activity);
    state->shutdownMethod = env->GetMethodID(activityClass, "shutdown", "()V");
    env->DeleteLocalRef(activityClass);
    if (!state->activity || !state->shutdownMethod) {
        if (state->activity) env->DeleteGlobalRef(state->activity);
        pthread_cond_destroy(&state->cond);
        pthread_mutex_destroy(&state->mutex);
        delete state;
        return 0;
    }
    if (dataDir) {
        const char* path = env->GetStringUTFChars(dataDir, nullptr);
        if (path) {
            snprintf(state->baseDir, sizeof(state->baseDir), "%s", path);
            env->ReleaseStringUTFChars(dataDir, path);
        }
    }
    gPhone = state;
    if (pthread_create(&state->thread, nullptr, phoneMainThread, state) != 0) {
        PLOGE("Failed to start Android game thread");
        gPhone = nullptr;
        env->DeleteGlobalRef(state->activity);
        pthread_cond_destroy(&state->cond);
        pthread_mutex_destroy(&state->mutex);
        delete state;
        return 0;
    }
    state->threadStarted = true;
    return static_cast<jlong>(reinterpret_cast<intptr_t>(state));
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onStart(JNIEnv*, jclass, jlong, jobject) {}
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onResume(JNIEnv*, jclass, jlong) { setPhoneState(false, false); }
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onPause(JNIEnv*, jclass, jlong) { setPhoneState(true, false); }
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onStop(JNIEnv*, jclass, jlong) { setPhoneState(true, false); }

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onDestroy(JNIEnv* env, jclass, jlong handle)
{
    auto* state = reinterpret_cast<PhoneAppState*>(static_cast<intptr_t>(handle));
    if (!state) return;
    pthread_mutex_lock(&state->mutex);
    state->quitting = true;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    VrQueueEvent(VR_EV_QUIT, 0, 0, 0, 0);

    if (state->threadStarted) {
        struct timespec deadline{};
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 8;
        pthread_mutex_lock(&state->mutex);
        while (!state->threadFinished) {
            const int rc = pthread_cond_timedwait(&state->cond, &state->mutex, &deadline);
            if (rc != 0) break;
        }
        const bool finished = state->threadFinished;
        pthread_mutex_unlock(&state->mutex);
        if (finished) pthread_join(state->thread, nullptr);
        else {
            // Never free state still referenced by the game thread.
            pthread_detach(state->thread);
            PLOGW("Game thread did not stop; retaining native state for process lifetime");
            gPhone = nullptr;
            return;
        }
    }
    if (state->window) ANativeWindow_release(state->window);
    if (state->activity) env->DeleteGlobalRef(state->activity);
    pthread_cond_destroy(&state->cond);
    pthread_mutex_destroy(&state->mutex);
    if (gPhone == state) gPhone = nullptr;
    delete state;
}

static void replaceWindow(JNIEnv* env, PhoneAppState* state, jobject surface)
{
    ANativeWindow* next = surface ? ANativeWindow_fromSurface(env, surface) : nullptr;
    pthread_mutex_lock(&state->mutex);
    ANativeWindow* old = state->window;
    state->window = next;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    if (old) ANativeWindow_release(old);
}

extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onSurfaceCreated(JNIEnv* env, jclass, jlong handle, jobject surface)
{
    auto* state = reinterpret_cast<PhoneAppState*>(static_cast<intptr_t>(handle));
    if (state) replaceWindow(env, state, surface);
}
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onSurfaceChanged(JNIEnv* env, jclass, jlong handle, jobject surface)
{
    auto* state = reinterpret_cast<PhoneAppState*>(static_cast<intptr_t>(handle));
    if (state) replaceWindow(env, state, surface);
}
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onSurfaceDestroyed(JNIEnv*, jclass, jlong handle)
{
    auto* state = reinterpret_cast<PhoneAppState*>(static_cast<intptr_t>(handle));
    if (!state) return;
    pthread_mutex_lock(&state->mutex);
    ANativeWindow* old = state->window;
    state->window = nullptr;
    pthread_cond_broadcast(&state->cond);
    pthread_mutex_unlock(&state->mutex);
    if (old) ANativeWindow_release(old);
}
extern "C" JNIEXPORT void JNICALL
Java_com_speeddreamsvr_SDVRLib_onTouch(JNIEnv*, jclass, jlong, jint phase, jint pointerId, jfloat x, jfloat y)
{
    SDPhoneTouchEvent(phase, pointerId, x, y);
}
