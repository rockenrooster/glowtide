#include "platform/platform.h"

#include <atomic>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "core/log.h"

#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <fcntl.h>
#include <jni.h>
#include <sched.h>
#include <unistd.h>
#endif

namespace gt::platform {

#if defined(__ANDROID__)

namespace {
jclass g_activityClass = nullptr;
jmethodID g_haptic = nullptr;
jmethodID g_refresh = nullptr;
jmethodID g_requestRefresh = nullptr;
jmethodID g_checkUpdate = nullptr;

// Present timestamps
bool g_presentOk = false;
EGLDisplay g_dpy = EGL_NO_DISPLAY;
EGLSurface g_surface = EGL_NO_SURFACE;
SDL_Window* g_window = nullptr;
PFNEGLGETNEXTFRAMEIDANDROIDPROC g_nextFrameId = nullptr;
PFNEGLGETFRAMETIMESTAMPSANDROIDPROC g_frameTimestamps = nullptr;
constexpr int kPending = 32;
EGLuint64KHR g_ids[kPending];
int g_idHead = 0, g_idCount = 0;

void enableTimestamps() {
    g_surface = eglGetCurrentSurface(EGL_DRAW);  // (SDL_EGL_GetWindowSurface is not implemented on Android)
    if (g_surface == EGL_NO_SURFACE) return;
    if (!eglSurfaceAttrib(g_dpy, g_surface, EGL_TIMESTAMPS_ANDROID, EGL_TRUE)) {
        GT_WARN("present: EGL_TIMESTAMPS_ANDROID refused (0x%x)", eglGetError());
        g_presentOk = false;
    }
    g_idHead = g_idCount = 0;
}
}  // namespace

bool isAndroid() { return true; }

namespace {
u64 readNumber(const char* path) {
    const int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return 0;
    char buf[32];
    const ssize_t n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0) return 0;
    buf[n] = 0;
    u64 v = 0;
    for (const char* c = buf; *c >= '0' && *c <= '9'; ++c) v = v * 10 + u64(*c - '0');
    return v;
}
}  // namespace

int pinToFastCores(char* desc, size_t cap) {
    if (cap) desc[0] = 0;
    constexpr int kMax = 32;
    const long configured = sysconf(_SC_NPROCESSORS_CONF);
    const int n = configured <= 0 ? 0 : (configured < kMax ? int(configured) : kMax);
    u64 score[kMax] = {};
    for (const char* source : {"cpu_capacity", "cpufreq/cpuinfo_max_freq"}) {
        bool complete = n > 0;
        for (int c = 0; c < n; ++c) {
            char path[96];
            std::snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/%s", c, source);
            score[c] = readNumber(path);
            complete = complete && score[c] != 0;
        }
        if (complete) break;
    }
    u64 lo = ~0ull, hi = 0;
    for (int c = 0; c < n; ++c) {
        if (!score[c]) continue;
        lo = score[c] < lo ? score[c] : lo;
        hi = score[c] > hi ? score[c] : hi;
    }
    if (hi == 0 || lo == hi) return 0;
    cpu_set_t set;
    CPU_ZERO(&set);
    int count = 0;
    size_t used = 0;
    for (int c = 0; c < n; ++c) {
        if (score[c] <= lo) continue;
        CPU_SET(c, &set);
        ++count;
        if (used + 4 < cap) used += size_t(std::snprintf(desc + used, cap - used, "%s%d", used ? "," : "", c));
    }
    if (sched_setaffinity(0, sizeof set, &set) != 0) return 0;
    return count;
}

void init(SDL_Window*) {
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!env || !activity) {
        GT_WARN("platform: no JNI environment");
        return;
    }
    jclass cls = env->GetObjectClass(activity);
    g_activityClass = static_cast<jclass>(env->NewGlobalRef(cls));
    g_haptic = env->GetStaticMethodID(g_activityClass, "haptic", "(I)V");
    g_refresh = env->GetStaticMethodID(g_activityClass, "refreshRate", "()F");
    g_requestRefresh = env->GetStaticMethodID(g_activityClass, "requestRefreshRate", "(F)V");
    g_checkUpdate = env->GetStaticMethodID(g_activityClass, "checkForUpdate", "()V");
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    GT_LOG("platform: haptic %s, refresh %s", g_haptic ? "ok" : "missing", g_refresh ? "ok" : "missing");
}

void haptic(int kind) {
    if (!g_haptic) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return;
    env->CallStaticVoidMethod(g_activityClass, g_haptic, jint(kind));
    if (env->ExceptionCheck()) env->ExceptionClear();
}

void checkForUpdate() {
    if (!g_checkUpdate) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return;
    env->CallStaticVoidMethod(g_activityClass, g_checkUpdate);
    if (env->ExceptionCheck()) env->ExceptionClear();
}

void requestRefreshRate(float hz) {
    if (!g_requestRefresh) return;
    JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) return;
    env->CallStaticVoidMethod(g_activityClass, g_requestRefresh, jfloat(hz));
    if (env->ExceptionCheck()) env->ExceptionClear();
}

float refreshRateHz(SDL_Window* window) {
    if (g_refresh) {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
        if (env) {
            const float hz = env->CallStaticFloatMethod(g_activityClass, g_refresh);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (hz > 1.0f) return hz;
        }
    }
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
    return mode && mode->refresh_rate > 0 ? mode->refresh_rate : 60.0f;
}

void outputPath(const char* in, char* out, size_t n) {
    if (in[0] == '/') {
        std::snprintf(out, n, "%s", in);
        return;
    }
    const char* dir = SDL_GetAndroidExternalStoragePath();
    std::snprintf(out, n, "%s/%s", dir ? dir : ".", in);
}

void presentInit(SDL_Window* window) {
    g_window = window;
    g_dpy = eglGetCurrentDisplay();
    const char* ext = g_dpy != EGL_NO_DISPLAY ? eglQueryString(g_dpy, EGL_EXTENSIONS) : nullptr;
    if (!ext || !std::strstr(ext, "EGL_ANDROID_get_frame_timestamps")) {
        GT_WARN("present: EGL_ANDROID_get_frame_timestamps is missing; grading falls back to swap intervals");
        return;
    }
    g_nextFrameId = reinterpret_cast<PFNEGLGETNEXTFRAMEIDANDROIDPROC>(eglGetProcAddress("eglGetNextFrameIdANDROID"));
    g_frameTimestamps = reinterpret_cast<PFNEGLGETFRAMETIMESTAMPSANDROIDPROC>(eglGetProcAddress("eglGetFrameTimestampsANDROID"));
    g_presentOk = g_nextFrameId && g_frameTimestamps;
    if (g_presentOk) enableTimestamps();
    auto supported = reinterpret_cast<PFNEGLGETFRAMETIMESTAMPSUPPORTEDANDROIDPROC>(
        eglGetProcAddress("eglGetFrameTimestampSupportedANDROID"));
    if (g_presentOk && supported && g_surface != EGL_NO_SURFACE) {
        const bool present = supported(g_dpy, g_surface, EGL_DISPLAY_PRESENT_TIME_ANDROID);
        GT_LOG("present: display present time %s", present ? "supported" : "NOT supported");
        if (!present) g_presentOk = false;
    }
    GT_LOG("present: frame timestamps %s", g_presentOk ? "on" : "unavailable");
}

bool presentAvailable() { return g_presentOk; }

void presentBeforeSwap() {
    if (!g_presentOk) return;
    const EGLSurface s = eglGetCurrentSurface(EGL_DRAW);
    if (s != g_surface) enableTimestamps();  // a new surface after the app came back
    if (g_surface == EGL_NO_SURFACE) return;
    EGLuint64KHR id = 0;
    if (!g_nextFrameId(g_dpy, g_surface, &id)) return;
    if (g_idCount == kPending) {  // the oldest never resolved: drop it
        g_idHead = (g_idHead + 1) % kPending;
        --g_idCount;
    }
    g_ids[(g_idHead + g_idCount) % kPending] = id;
    ++g_idCount;
}

int presentCollect(double* out, int max) {
    int n = 0;
    if (!g_presentOk || g_surface == EGL_NO_SURFACE) return 0;
    while (g_idCount > 0 && n < max) {
        const EGLint name = EGL_DISPLAY_PRESENT_TIME_ANDROID;
        EGLnsecsANDROID value = 0;
        if (!g_frameTimestamps(g_dpy, g_surface, g_ids[g_idHead], 1, &name, &value)) {
            static int warned = 0;
            if (warned++ < 3) GT_WARN("present: eglGetFrameTimestampsANDROID failed (0x%x)", eglGetError());
            // An id the driver no longer knows: drop it.
            g_idHead = (g_idHead + 1) % kPending;
            --g_idCount;
            continue;
        }
        if (value == EGL_TIMESTAMP_PENDING_ANDROID) break;
        if (value != EGL_TIMESTAMP_INVALID_ANDROID && value > 0) out[n++] = double(value) * 1e-9;
        g_idHead = (g_idHead + 1) % kPending;
        --g_idCount;
    }
    return n;
}

#else  // desktop

bool isAndroid() { return false; }
int pinToFastCores(char* desc, size_t cap) {
    if (cap) desc[0] = 0;
    return 0;
}
void init(SDL_Window*) {}
void haptic(int) {}
void requestRefreshRate(float) {}
void checkForUpdate() {}
float refreshRateHz(SDL_Window* window) {
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window));
    return mode && mode->refresh_rate > 0 ? mode->refresh_rate : 60.0f;
}
void outputPath(const char* in, char* out, size_t n) { std::snprintf(out, n, "%s", in); }
void presentInit(SDL_Window*) {}
bool presentAvailable() { return false; }
void presentBeforeSwap() {}
int presentCollect(double*, int) { return 0; }

#endif

namespace {
std::atomic<int> g_latestBuild{0};
}  // namespace

int latestBuild() { return g_latestBuild.load(std::memory_order_relaxed); }

#if defined(__ANDROID__)
// GlowtideActivity.nativeUpdateFound(int): the latest GitHub release's build number, from its update thread.
extern "C" JNIEXPORT void JNICALL Java_dev_glowtide_game_GlowtideActivity_nativeUpdateFound(JNIEnv*, jclass, jint build) {
    g_latestBuild.store(static_cast<int>(build), std::memory_order_relaxed);
}
#endif

}  // namespace gt::platform
