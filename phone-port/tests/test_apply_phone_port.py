import importlib.util
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "apply_phone_port.py"
spec = importlib.util.spec_from_file_location("apply_phone_port", SCRIPT)
phone_port = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(phone_port)


class ApplyPhonePortTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.reference = self.root / "SpeedDreams-VR"
        self.engine = self.root / "speed-dreams-code"
        self._write(self.reference / "cmake/sd.cmake", """function(sd_static_lib name dir)
    set(srcs "")
    foreach(f ${ARGN})
        list(APPEND srcs ${dir}/${f})
    endforeach()
endfunction()
function(sd_module name dir)
    set(srcs "")
    foreach(f ${ARGN})
        list(APPEND srcs ${dir}/${f})
    endforeach()
endfunction()
""")
        self._write(self.reference / "CMakeLists.txt", """cmake_minimum_required(VERSION 3.22)
project(sdvr C CXX)
find_package(OpenXR REQUIRED CONFIG)
add_library(sdvr SHARED
    ${VR_DIR}/tbxr/TBXR_Common.c
    ${VR_DIR}/sd_vr.cpp
    ${VR_DIR}/vr_paths.cpp
    ${VR_DIR}/vr_events.cpp
    ${VR_DIR}/vr_audio.cpp
    ${VR_DIR}/vr_stereo.cpp
    ${VR_DIR}/glu_shim.cpp
    ${SD_SRC}/main/main.cpp
)
set_source_files_properties(${SD_SRC}/main/main.cpp PROPERTIES COMPILE_DEFINITIONS "main=SdMain")
include(cmake/sd.cmake)
target_link_libraries(sdvr PRIVATE
    sd_headers
    ${SD_CORE_LIBS}
    OpenXR::openxr_loader
    EGL
    OpenSLES
    android
    log
)
""")
        self._write(self.reference / "android/app/build.gradle", """plugins { id 'com.android.application' }
android {
    namespace 'com.speeddreamsvr'
    defaultConfig {
        applicationId 'com.speeddreamsvr'
        minSdk 29
        targetSdk 32
        versionCode 6
        versionName '0.10.0-beta'
        ndk {
            abiFilters 'arm64-v8a'
        }
        externalNativeBuild {
            cmake {
                arguments '-DANDROID_STL=c++_shared', '-DANDROID_PLATFORM=android-29'
            }
        }
    }
    signingConfigs {
        debug {
            keyPassword 'android'
        }
    }
}
dependencies {
    implementation 'org.khronos.openxr:openxr_loader_for_android:1.1.63'
}
""")
        self._write(self.reference / "android/app/src/main/AndroidManifest.xml", """<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" xmlns:tools="http://schemas.android.com/tools">
    <uses-feature android:glEsVersion="0x00030001" android:required="true" />
    <uses-feature android:name="android.hardware.vr.headtracking" android:version="1" android:required="true" />
    <uses-permission android:name="org.khronos.openxr.permission.OPENXR_SYSTEM" />
    <queries><provider android:authorities="org.khronos.openxr.runtime_broker" /></queries>
    <application android:label="@string/app_name">
        <meta-data android:name="com.oculus.supportedDevices" android:value="quest" />
        <meta-data android:name="pvr.app.type" android:value="vr" />
        <activity android:name="com.speeddreamsvr.SDVRActivity" android:exported="true" android:excludeFromRecents="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
                <category android:name="org.khronos.openxr.intent.category.IMMERSIVE_HMD" />
            </intent-filter>
        </activity>
    </application>
</manifest>
""")
        self._write(self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java", """package com.speeddreamsvr;
import static android.system.Os.setenv;
import android.os.Build;
import java.util.Locale;
import android.view.SurfaceView;
public class SDVRActivity {
    private long mNativeHandle;
    static {
        String manufacturer = Build.MANUFACTURER.toLowerCase(Locale.ROOT);
        try { setenv("OPENXR_HMD", manufacturer, true); } catch (Exception ignored) {}
        System.loadLibrary("openxr_loader");
        System.loadLibrary("sdvr");
    }
    void create() {
        SurfaceView view = new SurfaceView(this);
        setContentView(view);
        view.getHolder().addCallback(this);
    }
    public void shutdown()
    {
        Log.v(TAG, "shutdown() requested by native code");
        finishAffinity();
        System.exit(0);
    }
}
""")
        self._write(self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java", """package com.speeddreamsvr;
public class SDVRLib {
    public static native long onCreate(Object activity, String dataDir);
}
""")
        self._write(self.reference / "android/app/src/main/res/values/strings.xml", """<resources><string name="app_name">Speed Dreams VR</string></resources>""")
        self._write(self.engine / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp", """void drawRace() {
    VrRaceFrameBegin();
    for (int eye = 0; eye < 2; ++eye) {
        VrRaceEyeBegin(eye);
        draw();
        VrRaceEyeEnd(eye);
    }
    VrRaceFrameEnd();
}
""")

    def tearDown(self):
        self.tmp.cleanup()

    def _write(self, path, content):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")

    def test_patch_is_phone_only_and_idempotent(self):
        changed = phone_port.apply(self.reference, self.engine)
        self.assertGreater(len(changed), 0)
        cmake = (self.reference / "CMakeLists.txt").read_text()
        self.assertIn("option(SD_PHONE", cmake)
        self.assertIn("phone/sd_phone.cpp", cmake)
        self.assertIn("if(NOT SD_PHONE)", cmake)
        self.assertIn("${SDVR_OPTIONAL_LIBS}", cmake)
        gradle = (self.reference / "android/app/build.gradle").read_text()
        self.assertIn("-DSD_PHONE=ON", gradle)
        self.assertNotIn("openxr_loader_for_android", gradle)
        self.assertIn("applicationId 'com.speeddreams.mobile'", gradle)
        self.assertIn("versionCode 8", gradle)
        self.assertIn("versionName '0.10.0-phone2'", gradle)
        self.assertIn("abiFilters 'arm64-v8a', 'armeabi-v7a'", gradle)
        self.assertIn("enableV1Signing = true", gradle)
        self.assertIn("enableV2Signing = true", gradle)
        manifest = (self.reference / "android/app/src/main/AndroidManifest.xml").read_text()
        self.assertNotIn("android.hardware.vr.headtracking", manifest)
        self.assertNotIn("IMMERSIVE_HMD", manifest)
        self.assertIn('android:glEsVersion="0x00020000"', manifest)
        activity = (self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java").read_text()
        self.assertIn("SDVRLib.onTouch(handle", activity)
        self.assertIn("import android.os.Build;", activity)
        self.assertNotIn("OPENXR_HMD", activity)
        lib = (self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java").read_text()
        self.assertIn("native void onTouch", lib)
        race = (self.engine / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp").read_text()
        self.assertIn("eye < 1", race)
        self.assertFalse((self.reference / "src/vr/phone/phone_controls.cpp").stat().st_size == 0)

        # Applying the same source overlay twice must not add duplicate CMake,
        # touch listeners, OpenXR declarations, or alter an already-correct loop.
        self.assertEqual(phone_port.apply(self.reference, self.engine), [])


    def test_patch_public_upstream_main_uses_phone_egl_and_sdl_events(self):
        # The public upstream main branch has no VrRaceEyeBegin hooks; its GUI
        # has SDL screen setup and a mono rmRedisplay callback instead.
        self._write(self.engine / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp", """static void rmRedisplay() {
    GfuiSwapBuffers();
}
""")
        self._write(self.engine / "src/libs/tgfclient/guiscreen.cpp", """#include <SDL.h>
SDL_Window* GfuiWindow = nullptr;
static SDL_GLContext GLContext = nullptr;
SDL_Surface* PScreenSurface = nullptr;
bool GfScrResizable = true;

SDL_Surface* gfScrCreateWindow(int nWinWidth, int nWinHeight, int nTotalDepth, int bfVideoMode)
{
    GfuiWindow = SDL_CreateWindow("game", 0, 0, nWinWidth, nWinHeight,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);

    /* Create OpenGL context */
    if (!(GLContext = SDL_GL_CreateContext(GfuiWindow)))
    {
        GfLogError("SDL_GL_CreateContext failed");
        return nullptr;
    }

    // If specified, try best possible settings.
    PScreenSurface = SDL_CreateRGBSurface(0, nWinWidth, nWinHeight, nTotalDepth, 0, 0, 0, 0);
    if (bfVideoMode & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);
    return PScreenSurface;
}

int GfScrGetAttachedDisplays() { return 1; }

bool GfScrInit(int nWinWidth, int nWinHeight, int nFullScreen)
{
    GfScrResizable = GfScrGetResizable();
    return GfScrInitSDL2(nWinWidth, nWinHeight, nFullScreen);
}

void GfScrShutdown(void)
{
    SDL_GL_MakeCurrent(GfuiWindow, GLContext);
    SDL_GL_DeleteContext(GLContext);
    GLContext = NULL;
}
""")
        self._write(self.engine / "src/libs/tgfclient/gui.cpp", """void GfuiRedraw(void) {}
void GfuiDisplay(void)
{
    GfuiRedraw();
    GfuiSwapBuffers();
}
void GfuiSwapBuffers(void)
{
    SDL_GL_SwapWindow(GfuiWindow);
}
""")
        self._write(self.engine / "src/libs/tgfclient/control.cpp", """#include <string>
#define GFCTRL_JOY_PRESENT 1
#define GFCTRL_JOY_NUMBER 8
#define GFCTRL_JOY_MAX_BUTTONS 32
#define GFCTRL_JOY_MAX_AXES 12
typedef struct {
    int oldb[GFCTRL_JOY_NUMBER];
    float ax[GFCTRL_JOY_MAX_AXES * GFCTRL_JOY_NUMBER];
    int edgeup[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];
    int edgedn[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];
    int levelup[GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER];
} tCtrlJoyInfo;
int gfctrlJoyPresent = 0;
std::string& GfctrlJoyName(int joy) { static std::string name = "none"; return name; }
int GfctrlJoyIsAnyPresent(void) { return gfctrlJoyPresent; }
int GfctrlJoyNumber(void) { return gfctrlJoyPresent; }
int GfctrlJoyGetCurrentStates(tCtrlJoyInfo *joyInfo) { return 0; }
""")

        changed = phone_port.apply(self.reference, self.engine)
        self.assertGreater(len(changed), 0)

        cmake = (self.reference / "CMakeLists.txt").read_text()
        self.assertIn("SD_PHONE_UPSTREAM_MAIN=1", cmake)

        screen = (self.engine / "src/libs/tgfclient/guiscreen.cpp").read_text()
        self.assertIn("SDPhoneGetSurfaceSize(&phoneWidth, &phoneHeight)", screen)
        self.assertIn("#ifdef SD_PHONE_UPSTREAM_MAIN\n        SDL_WINDOW_HIDDEN);\n#else\n        SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);\n#endif", screen)
        self.assertNotIn("} else SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);", screen)
        self.assertIn("else {\n#ifndef SD_PHONE_UPSTREAM_MAIN", screen)
        self.assertIn("GfScrResizable = false;", screen)
        self.assertIn("GLContext = NULL;", screen)
        self.assertIn("#ifndef SD_PHONE_UPSTREAM_MAIN", screen)

        gui = (self.engine / "src/libs/tgfclient/gui.cpp").read_text()
        self.assertIn("SDPhoneUpdateInput(0);", gui)
        self.assertIn("SDPhoneDrawControls(phoneWidth, phoneHeight);", gui)
        self.assertIn("SDPhonePresent();", gui)
        self.assertIn("SDL_GL_SwapWindow(GfuiWindow);", gui)

        race = (self.engine / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp").read_text()
        self.assertIn("SDPhoneUpdateInput(1);", race)
        self.assertIn("SD_PHONE_UPSTREAM_MAIN", race)
        self.assertIn('extern "C" void SDPhoneUpdateInput(int racing);', race)

        control = (self.engine / "src/libs/tgfclient/control.cpp").read_text()
        self.assertIn("SDPhoneReadJoystick(&phoneButtons, phoneAxes)", control)
        self.assertIn("return GFCTRL_JOY_PRESENT;", control)
        self.assertIn("return 1;", control)
        self.assertIn('static std::string phoneControlsName = "Touch Controls";', control)

        # All injected screen, GUI, joystick and race changes must be idempotent.
        self.assertEqual(phone_port.apply(self.reference, self.engine), [])


if __name__ == "__main__":
    unittest.main()
