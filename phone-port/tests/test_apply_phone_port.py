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
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments '-DANDROID_STL=c++_shared', '-DANDROID_PLATFORM=android-29'
            }
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
        manifest = (self.reference / "android/app/src/main/AndroidManifest.xml").read_text()
        self.assertNotIn("android.hardware.vr.headtracking", manifest)
        self.assertNotIn("IMMERSIVE_HMD", manifest)
        activity = (self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java").read_text()
        self.assertIn("SDVRLib.onTouch(handle", activity)
        self.assertNotIn("OPENXR_HMD", activity)
        lib = (self.reference / "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java").read_text()
        self.assertIn("native void onTouch", lib)
        race = (self.engine / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp").read_text()
        self.assertIn("eye < 1", race)
        self.assertFalse((self.reference / "src/vr/phone/phone_controls.cpp").stat().st_size == 0)

        # Applying the same source overlay twice must not add duplicate CMake,
        # touch listeners, OpenXR declarations, or alter an already-correct loop.
        self.assertEqual(phone_port.apply(self.reference, self.engine), [])


if __name__ == "__main__":
    unittest.main()
