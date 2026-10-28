#!/usr/bin/env python3
"""Apply the flat-screen phone backend to a clean SpeedDreams-VR checkout."""
from __future__ import annotations
import argparse, re, shutil, sys, xml.etree.ElementTree as ET
from pathlib import Path

ANDROID_NS = "{http://schemas.android.com/apk/res/android}"
SOURCE_ROOT = Path(__file__).resolve().parents[1] / "src"
PHONE_FILES = ("phone_controls.h", "phone_controls.cpp", "phone_input_adapter.h",
               "phone_input_adapter.cpp", "phone_stereo.cpp", "sd_phone.cpp")


class PortError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise PortError(message)


def read(path: Path) -> str:
    require(path.is_file(), f"Required file is missing: {path}")
    return path.read_text(encoding="utf-8")


def write_if_changed(path: Path, value: str) -> bool:
    old = path.read_text(encoding="utf-8") if path.exists() else None
    if old == value:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8", newline="\n")
    return True


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    require(count == 1, f"Expected exactly one {label} marker, found {count}")
    return text.replace(old, new, 1)


def patch_cmake(path: Path, upstream_main: bool = False) -> str:
    text = read(path)
    dollar = chr(36)
    vdir = dollar + "{VR_DIR}"
    sroot = dollar + "{SD_SRC}"
    if "option(SD_PHONE" not in text:
        text = replace_once(text, "project(sdvr C CXX)",
            'project(sdvr C CXX)\n\noption(SD_PHONE "Build the flat-screen Android phone port" OFF)',
            "project declaration")

    original_find = "find_package(OpenXR REQUIRED CONFIG)"
    if original_find in text and "if(NOT SD_PHONE)\n    find_package(OpenXR REQUIRED CONFIG)\nendif()" not in text:
        text = replace_once(text, original_find,
            "if(NOT SD_PHONE)\n    find_package(OpenXR REQUIRED CONFIG)\nendif()",
            "OpenXR find_package")
    require("if(NOT SD_PHONE)\n    find_package(OpenXR REQUIRED CONFIG)" in text,
            "OpenXR dependency is not guarded for phone builds")

    start = text.find("add_library(sdvr SHARED\n")
    if start >= 0:
        end = text.find("\nset_source_files_properties(", start)
        require(end > start, "Could not locate the end of the sdvr source list")
        old_block = text[start:end]
        require("sd_vr.cpp" in old_block and "TBXR_Common.c" in old_block,
                "The current source list is not the expected SpeedDreams-VR layout")
        source_block = (
            "set(SDVR_PLATFORM_SOURCES\n"
            f"    {vdir}/vr_paths.cpp\n"
            f"    {vdir}/vr_events.cpp\n"
            f"    {vdir}/vr_audio.cpp\n"
            f"    {vdir}/vr_config.cpp\n"
            f"    {vdir}/vr_rtti.cpp\n"
            f"    {vdir}/glu_shim.cpp\n"
            ")\n"
            "if(SD_PHONE)\n"
            "    list(APPEND SDVR_PLATFORM_SOURCES\n"
            f"        {vdir}/phone/sd_phone.cpp\n"
            f"        {vdir}/phone/phone_controls.cpp\n"
            f"        {vdir}/phone/phone_input_adapter.cpp\n"
            f"        {vdir}/phone/phone_stereo.cpp\n"
            "    )\n"
            "else()\n"
            "    list(APPEND SDVR_PLATFORM_SOURCES\n"
            f"        {vdir}/tbxr/TBXR_Common.c\n"
            f"        {vdir}/tbxr/OpenXrInput.c\n"
            f"        {vdir}/sd_vr.cpp\n"
            f"        {vdir}/vr_input.cpp\n"
            f"        {vdir}/vr_stereo.cpp\n"
            f"        {vdir}/vr_haptics.cpp\n"
            f"        {vdir}/vr_keyboard.cpp\n"
            f"        {vdir}/vr_perf.c\n"
            "    )\n"
            "endif()\n"
            f"add_library(sdvr SHARED {dollar}{{SDVR_PLATFORM_SOURCES}} {sroot}/main/main.cpp)\n"
        )
        text = text[:start] + source_block + text[end:]
    else:
        require("phone/phone_stereo.cpp" in text and "phone/sd_phone.cpp" in text,
                "Could not find original or already-patched sdvr source list")

    include_marker = "include(cmake/sd.cmake)"
    if "target_compile_definitions(sd_headers INTERFACE SD_PHONE=1)" not in text:
        text = replace_once(text, include_marker,
            include_marker + '\nif(SD_PHONE)\n'
            f'    target_include_directories(sdvr PRIVATE {vdir}/phone)\n'
            '    add_compile_definitions(SD_PHONE=1)\n'
            '    target_compile_definitions(sdvr PRIVATE SD_PHONE=1)\n'
            '    target_compile_definitions(sd_headers INTERFACE SD_PHONE=1)\n'
            'endif()',
            "Speed Dreams module CMake include")
    if upstream_main and "SD_PHONE_UPSTREAM_MAIN=1" not in text:
        marker = "    target_compile_definitions(sd_headers INTERFACE SD_PHONE=1)"
        text = replace_once(text, marker,
            marker + '\n    add_compile_definitions(SD_PHONE_UPSTREAM_MAIN=1)'
            + '\n    target_compile_definitions(sdvr PRIVATE SD_PHONE_UPSTREAM_MAIN=1)'
            + '\n    target_compile_definitions(sd_headers INTERFACE SD_PHONE_UPSTREAM_MAIN=1)',
            "upstream-main phone compatibility definitions")

    if "OpenXR::openxr_loader" in text and "SDVR_OPTIONAL_LIBS" not in text:
        text = replace_once(text, "target_link_libraries(sdvr PRIVATE",
            "set(SDVR_OPTIONAL_LIBS)\n"
            "if(NOT SD_PHONE)\n"
            "    list(APPEND SDVR_OPTIONAL_LIBS OpenXR::openxr_loader)\n"
            "endif()\n\n"
            "target_link_libraries(sdvr PRIVATE",
            "native link block")
        text = replace_once(text, "    OpenXR::openxr_loader\n",
                            f"    {dollar}{{SDVR_OPTIONAL_LIBS}}\n", "OpenXR link entry")
    else:
        require("SDVR_OPTIONAL_LIBS" in text, "Missing conditional OpenXR link setup")
    require("phone/sd_phone.cpp" in text and "phone/phone_controls.cpp" in text,
            "Phone sources were not installed in CMake")
    return text


def patch_gradle(path: Path) -> str:
    text = read(path)
    args = "arguments '-DANDROID_STL=c++_shared', '-DANDROID_PLATFORM=android-29'"
    if "-DSD_PHONE=ON" not in text:
        text = replace_once(text, args,
            "arguments '-DANDROID_STL=c++_shared', '-DANDROID_PLATFORM=android-29', '-DSD_PHONE=ON'",
            "Android CMake argument list")
    text = re.sub(
        r"\s*implementation\s+['\"]org\.khronos\.openxr:openxr_loader_for_android:[^'\"]+['\"]\s*",
        "\n", text)
    require("org.khronos.openxr:openxr_loader_for_android" not in text,
            "OpenXR Gradle dependency remains in phone mode")

    # Keep the flat-screen app separate from the upstream VR package and
    # from earlier experimental APKs, which were signed by ephemeral CI keys.
    if "applicationId 'com.speeddreams.mobile'" not in text:
        text = replace_once(text, "applicationId 'com.speeddreamsvr'",
            "applicationId 'com.speeddreams.mobile'", "phone application ID")
    if "versionCode 8" not in text:
        text = replace_once(text, "versionCode 6", "versionCode 8",
            "phone version code")
    if "versionName '0.10.0-phone2'" not in text:
        text = replace_once(text, "versionName '0.10.0-beta'",
            "versionName '0.10.0-phone2'", "phone version name")

    # Ship both ARM ABIs for phones and x86_64 for native testing on the Ramus emulator.
    universal_abi = "abiFilters 'arm64-v8a', 'armeabi-v7a', 'x86_64'"
    if universal_abi not in text:
        text = replace_once(text, "abiFilters 'arm64-v8a'", universal_abi,
            "universal ARM plus x86_64 ABI filters")
    require(text.count(universal_abi) == 1 and
            "abiFilters 'arm64-v8a'" not in text.replace(universal_abi, "", 1),
            "Android ABI filters are not configured for ARM and x86_64")

    old_signing_note = ("// Signed with the debug key: this is a sideloaded build, not a Store\n"
                        "// submission, and a throwaway key would only make upgrades harder.")
    new_signing_note = ("// Stable development certificate for sideload tests. Never use it for a Store release.")
    if old_signing_note in text:
        text = text.replace(old_signing_note, new_signing_note, 1)

    # Explicitly enable both APK signature schemes on the signing config.
    # enableV1Signing/enableV2Signing are the signing-config properties used
    # by the Android Gradle Plugin version in this pinned upstream project.
    has_v1 = "enableV1Signing = true" in text
    has_v2 = "enableV2Signing = true" in text
    if not has_v1 and not has_v2:
        text = replace_once(text, "keyPassword 'android'",
            "keyPassword 'android'\n            enableV1Signing = true\n            enableV2Signing = true",
            "v1/v2 APK signing settings")
    elif not (has_v1 and has_v2):
        raise PortError("Only one of the required v1/v2 signing settings is present")
    require("enableV1Signing = true" in text and "enableV2Signing = true" in text,
            "APK v1/v2 signing is not enabled")
    return text


def patch_manifest(path: Path) -> str:
    ET.register_namespace("android", "http://schemas.android.com/apk/res/android")
    ET.register_namespace("tools", "http://schemas.android.com/tools")
    root = ET.parse(path).getroot()
    remove_permissions = {"com.oculus.permission.HAND_TRACKING",
                          "org.khronos.openxr.permission.OPENXR_SYSTEM"}
    for child in list(root):
        tag = child.tag.rsplit("}", 1)[-1]
        name = child.attrib.get(ANDROID_NS + "name", "")
        if tag == "queries":
            root.remove(child)
        elif tag == "uses-feature":
            if name in {"android.hardware.vr.headtracking", "oculus.software.handtracking"}:
                root.remove(child)
            elif ANDROID_NS + "glEsVersion" in child.attrib:
                # The phone EGL backend has an ES3-to-ES2 fallback. Keeping the
                # VR reference's GLES 3.1 feature requirement is unnecessarily strict.
                child.set(ANDROID_NS + "glEsVersion", "0x00020000")
                child.set(ANDROID_NS + "required", "true")
        elif tag == "uses-permission" and name in remove_permissions:
            root.remove(child)

    app = root.find("application")
    require(app is not None, "Manifest has no application node")
    vr_meta = {"com.samsung.android.vr.application.mode", "com.oculus.supportedDevices",
               "pvr.app.type", "com.oculus.vr.focusaware",
               "com.oculus.handtracking.frequency", "com.oculus.handtracking.version"}
    for child in list(app):
        kind = child.tag.rsplit("}", 1)[-1]
        if kind == "meta-data":
            name = child.attrib.get(ANDROID_NS + "name", "")
            if name in vr_meta or "openxr" in name.lower() or "handtracking" in name.lower():
                app.remove(child)
        elif kind == "activity":
            for nested in list(child):
                nkind = nested.tag.rsplit("}", 1)[-1]
                if nkind == "meta-data":
                    name = nested.attrib.get(ANDROID_NS + "name", "")
                    if "vr" in name.lower() or "oculus" in name.lower() or "pico" in name.lower():
                        child.remove(nested)
                elif nkind == "intent-filter":
                    for category in list(nested):
                        if category.tag.rsplit("}", 1)[-1] == "category":
                            name = category.attrib.get(ANDROID_NS + "name", "")
                            if name in {"com.oculus.intent.category.VR",
                                        "org.khronos.openxr.intent.category.IMMERSIVE_HMD",
                                        "com.picovr.intent.category.VR"}:
                                nested.remove(category)
            child.attrib.pop(ANDROID_NS + "excludeFromRecents", None)

    value = ET.tostring(root, encoding="unicode", xml_declaration=True)
    require("android.hardware.vr.headtracking" not in value,
            "VR headtracking feature remained in phone manifest")
    require("org.khronos.openxr" not in value and "IMMERSIVE_HMD" not in value,
            "OpenXR manifest entries remained in phone manifest")
    require('android:glEsVersion="0x00020000"' in value,
            "Phone manifest still requires GLES 3.1 instead of GLES 2.0")
    return value + "\n"


def patch_activity(path: Path) -> str:
    text = read(path)
    if "import android.view.MotionEvent;" not in text:
        text = replace_once(text, "import android.view.SurfaceView;",
                            "import android.view.SurfaceView;\nimport android.view.MotionEvent;",
                            "SurfaceView import")
    for unused in ("import static android.system.Os.setenv;\n",
                   "import java.util.Locale;\n"):
        text = text.replace(unused, "")
    static_pattern = re.compile(
        r"\s*static\s*\{\s*String manufacturer.*?System\.loadLibrary\(\"sdvr\"\);\s*\}",
        re.DOTALL)
    if 'System.loadLibrary("openxr_loader")' in text or "OPENXR_HMD" in text:
        text, count = static_pattern.subn('\n    static {\n        System.loadLibrary("sdvr");\n    }', text, count=1)
        require(count == 1, "Could not replace VR-only static native library loader")

    # Emit explicit Java-side startup markers. The current runtime reports only
    # process exits on the emulator, so these markers distinguish class loading,
    # storage permission handling, data unpacking, and entry into JNI.
    load_block_pattern = re.compile(
        r"static\s*\{\s*System\.loadLibrary\(\"sdvr\"\);\s*\}")
    if "DIAG stage=loadlibrary-sdvr" not in text:
        diagnostic_loader = '''static {
        try {
            System.loadLibrary("sdvr");
            Log.i(TAG, "DIAG stage=loadlibrary-sdvr-ok");
        } catch (Throwable e) {
            Log.e(TAG, "DIAG stage=loadlibrary-sdvr-failed", e);
            throw new ExceptionInInitializerError(e);
        }
    }'''
        text, count = load_block_pattern.subn(diagnostic_loader, text, count=1)
        require(count == 1, "Could not instrument SDVR native library loading")

    diagnostics = [
        ('Log.v(TAG, "SDVRActivity::onCreate()");',
         '        Log.i(TAG, "DIAG stage=activity-onCreate sdk=" + Build.VERSION.SDK_INT + " package=" + getPackageName());',
         "activity creation"),
        ("private void checkPermissionsAndInitialize()\\n    {",
         '        Log.i(TAG, "DIAG stage=permission-check sdk=" + Build.VERSION.SDK_INT + " allFiles=" + (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager()));',
         "storage permission check"),
        ("private void onActivityResult", None, "permission result method"),
        ("private void unpackGameData()\\n    {",
         '        Log.i(TAG, "DIAG stage=unpack-start dataDir=" + mDataDir);',
         "game-data unpack"),
        ("private void create()\\n    {",
         '        Log.i(TAG, "DIAG stage=create nativeHandle=" + mNativeHandle + " unpacking=" + mUnpacking + " dataDir=" + mDataDir);',
         "activity create dispatch"),
        ("private void startNative()\\n    {",
         '        Log.i(TAG, "DIAG stage=native-start dataDir=" + mDataDir);',
         "native startup"),
    ]
    for marker, statement, label in diagnostics:
        if statement is not None and statement not in text and marker in text:
            text = text.replace(marker, marker + "\\n" + statement, 1)

    permission_result_marker = "        super.onActivityResult(requestCode, resultCode, data);"
    permission_result_log = '        Log.i(TAG, "DIAG stage=permission-result request=" + requestCode + " result=" + resultCode + " allFiles=" + (Build.VERSION.SDK_INT < Build.VERSION_CODES.R || Environment.isExternalStorageManager()));'
    if permission_result_log not in text and permission_result_marker in text:
        text = text.replace(permission_result_marker,
                            permission_result_marker + "\\n" + permission_result_log, 1)

    native_call = "        mNativeHandle = SDVRLib.onCreate(this, mDataDir);"
    native_enter = '        Log.i(TAG, "DIAG stage=jni-onCreate-enter dataDir=" + mDataDir);'
    native_return = '        Log.i(TAG, "DIAG stage=jni-onCreate-return handle=" + mNativeHandle);'
    if native_enter not in text and native_call in text:
        text = text.replace(native_call,
            native_enter + "\\n" + native_call + "\\n" + native_return, 1)

    listener = """        view.setOnTouchListener((v, event) -> {
            final long handle = mNativeHandle;
            if (handle == 0L) return true;
            final int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                final int index = event.getActionIndex();
                SDVRLib.onTouch(handle, 0, event.getPointerId(index), event.getX(index), event.getY(index));
            } else if (action == MotionEvent.ACTION_MOVE) {
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    SDVRLib.onTouch(handle, 2, event.getPointerId(i), event.getX(i), event.getY(i));
                }
            } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
                final int index = event.getActionIndex();
                SDVRLib.onTouch(handle, 1, event.getPointerId(index), event.getX(index), event.getY(index));
            } else if (action == MotionEvent.ACTION_CANCEL) {
                SDVRLib.onTouch(handle, 3, -1, 0.0f, 0.0f);
            }
            return true;
        });
"""
    if "SDVRLib.onTouch(handle" not in text:
        text = replace_once(text, "setContentView(view);",
                            "setContentView(view);\n\n" + listener.rstrip(),
                            "content view insertion point")
    old_shutdown = """public void shutdown()
    {
        Log.v(TAG, "shutdown() requested by native code");
        finishAffinity();
        System.exit(0);
    }"""
    new_shutdown = """public void shutdown()
    {
        Log.v(TAG, "shutdown() requested by native code");
        runOnUiThread(() -> {
            finishAffinity();
            System.exit(0);
        });
    }"""
    if old_shutdown in text:
        text = text.replace(old_shutdown, new_shutdown, 1)
    require("SDVRLib.onTouch(handle" in text, "Phone touch listener is missing")
    require('System.loadLibrary("openxr_loader")' not in text,
            "OpenXR is still loaded by the phone activity")
    return text


def patch_lib(path: Path) -> str:
    text = read(path)
    if "native void onTouch(" not in text:
        close = text.rfind("}")
        require(close >= 0, "Could not find end of SDVRLib.java")
        text = text[:close] + "    public static native void onTouch(long handle, int phase, int pointerId, float x, float y);\n" + text[close:]
    return text


def patch_app_name(path: Path) -> str:
    tree = ET.parse(path)
    found = False
    for node in tree.getroot().iter():
        if node.tag.rsplit("}", 1)[-1] == "string" and node.attrib.get("name") == "app_name":
            node.text = "Speed Dreams Mobile"
            found = True
    require(found, "Could not find app_name string resource")
    ET.indent(tree, space="    ")
    return ET.tostring(tree.getroot(), encoding="unicode", xml_declaration=True) + "\n"


def patch_single_eye_loop(engine_root: Path) -> str:
    path = engine_root / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp"
    text = read(path)
    calls = list(re.finditer(r"VrRaceEyeBegin\s*\(\s*(\w+)\s*\)", text))

    # The public upstream main branch has no VR eye hooks. Its race callback
    # is mono and ends at GfuiSwapBuffers(), so update touch controls there.
    if not calls:
        # Upstream's race callback is compiled in a separate translation unit
        # from gui.cpp, so it needs its own C-linkage declaration for the phone
        # input bridge. Keep this before the early-return/idempotence check.
        declaration = (
            "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
            'extern "C" void SDPhoneUpdateInput(int racing);\n'
            "#endif\n\n"
        )
        if 'extern "C" void SDPhoneUpdateInput(int racing);' not in text:
            text = declaration + text
        if "SDPhoneUpdateInput(1);" in text:
            return text
        marker = re.compile(r"(static\s+void\s+rmRedisplay\s*\(\s*\)\s*\{)")
        text, count = marker.subn(
            r"\1\n#ifdef SD_PHONE_UPSTREAM_MAIN\n\tSDPhoneUpdateInput(1);\n#endif",
            text, count=1)
        require(count == 1,
                "Engine has no VR eye hooks and no recognizable rmRedisplay() callback")
        return text

    for call in reversed(calls):
        var = re.escape(call.group(1))
        loop_re = re.compile(r"for\s*\(\s*(?:int|unsigned\s+int)\s+" + var +
            r"\s*=\s*0\s*;\s*" + var + r"\s*<\s*([12])\s*;\s*(?:\+\+\s*" +
            var + r"|" + var + r"\s*\+\+)\s*\)")
        preceding = list(loop_re.finditer(text, 0, call.start()))
        require(preceding, f"Could not find eye loop for VrRaceEyeBegin({call.group(1)})")
        match = preceding[-1]
        if match.group(1) == "2":
            text = text[:match.start(1)] + "1" + text[match.end(1):]
    require(not re.search(r"for\s*\(\s*(?:int|unsigned\s+int)\s+eye\s*=\s*0\s*;\s*eye\s*<\s*2", text),
            "A two-eye loop remains in the phone race renderer")
    return text


def patch_guiscreen(path: Path) -> str:
    """Use the already-current EGL surface instead of creating an SDL GL context."""
    text = read(path)
    function_start = text.find("SDL_Surface* gfScrCreateWindow(")
    function_end = text.find("\nint GfScrGetAttachedDisplays()", function_start)
    require(function_start >= 0 and function_end > function_start,
            "Could not locate gfScrCreateWindow() in guiscreen.cpp")

    phone_proto = (
        "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
        'extern "C" void SDPhoneGetSurfaceSize(int* width, int* height);\n'
        "#endif\n\n"
    )
    if 'extern "C" void SDPhoneGetSurfaceSize(int* width, int* height);' not in text:
        function_marker = re.search(r"(?m)^SDL_Surface\*\s*gfScrCreateWindow\s*\(", text)
        require(function_marker is not None,
                "Could not locate gfScrCreateWindow() declaration insertion point")
        text = text[:function_marker.start()] + phone_proto + text[function_marker.start():]
        function_start = text.find("SDL_Surface* gfScrCreateWindow(")

    # Recompute both offsets after inserting the declaration: the inserted
    # prototype shifts the function and its end marker by the same amount.
    function_start = text.find("SDL_Surface* gfScrCreateWindow(")
    function_end = text.find("\nint GfScrGetAttachedDisplays()", function_start)
    require(function_start >= 0 and function_end > function_start,
            "Could not re-locate gfScrCreateWindow() after adding the phone prototype")
    block = text[function_start:function_end]

    if ("SDPhoneGetSurfaceSize(&phoneWidth, &phoneHeight)" not in block
            and "SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);" in block):
        open_brace = block.find("{")
        require(open_brace >= 0, "Malformed gfScrCreateWindow()")
        prologue = """
#ifdef SD_PHONE_UPSTREAM_MAIN
    int phoneWidth = 0;
    int phoneHeight = 0;
    SDPhoneGetSurfaceSize(&phoneWidth, &phoneHeight);
    if (phoneWidth > 0 && phoneHeight > 0) {
        nWinWidth = phoneWidth;
        nWinHeight = phoneHeight;
    }
    bfVideoMode &= ~SDL_WINDOW_FULLSCREEN;
#endif
"""
        block = block[:open_brace + 1] + prologue + block[open_brace + 1:]

        # SDL_VIDEODRIVER=dummy is required because EGL is managed by the phone
        # thread. The SDL dummy backend cannot create an OpenGL window. Mask out
        # SDL_WINDOW_OPENGL from the last SDL_CreateWindow argument only in the
        # phone build; this also works when upstream computes flags in a variable.
        create_match = re.search(r"(?m)^([ \t]*)GfuiWindow\s*=\s*SDL_CreateWindow\s*\(", block)
        require(create_match is not None,
                "Could not locate SDL_CreateWindow in the upstream screen initializer")
        line_start = create_match.start()
        open_paren = create_match.end() - 1
        depth = 0
        quote = None
        escaped = False
        close_paren = -1
        for pos in range(open_paren, len(block)):
            ch = block[pos]
            if quote is not None:
                if escaped:
                    escaped = False
                elif ch == "\\":
                    escaped = True
                elif ch == quote:
                    quote = None
                continue
            if ch in ("'", '"'):
                quote = ch
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    close_paren = pos
                    break
        require(close_paren > open_paren,
                "Could not match SDL_CreateWindow parentheses")

        args_text = block[open_paren + 1:close_paren]
        arg_spans = []
        part_start = 0
        depth = 0
        quote = None
        escaped = False
        for pos, ch in enumerate(args_text):
            if quote is not None:
                if escaped:
                    escaped = False
                elif ch == "\\":
                    escaped = True
                elif ch == quote:
                    quote = None
                continue
            if ch in ("'", '"'):
                quote = ch
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
            elif ch == "," and depth == 0:
                arg_spans.append((part_start, pos))
                part_start = pos + 1
        arg_spans.append((part_start, len(args_text)))
        require(len(arg_spans) >= 6,
                "SDL_CreateWindow does not have the expected six or more arguments")

        last_start, last_end = arg_spans[-1]
        last_raw = args_text[last_start:last_end]
        flags_expr = last_raw.strip()
        require(flags_expr, "SDL_CreateWindow flags argument is empty")
        leading = re.match(r"\s*", last_raw).group(0)
        trailing = re.search(r"\s*$", last_raw).group(0)
        phone_flags = leading + "(" + flags_expr + ") & ~SDL_WINDOW_OPENGL" + trailing
        phone_args = args_text[:last_start] + phone_flags + args_text[last_end:]

        call_prefix = block[line_start:open_paren + 1]
        phone_call = call_prefix + phone_args + ")"
        statement_end = block.find(";", close_paren)
        require(statement_end >= 0,
                "Could not find the end of SDL_CreateWindow statement")
        original_call = block[line_start:statement_end + 1]
        indent = create_match.group(1)
        conditional_call = (
            "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
            + phone_call + ";\n"
            "#else\n"
            + original_call + "\n"
            "#endif"
        )
        block = block[:line_start] + conditional_call + block[statement_end + 1:]
        # Keep the complete upstream if/else structure intact: removing the
        # whole context block left its fullscreen "else" outside the if.
        context_start = block.find("    /* Create OpenGL context */")
        context_end = block.find("\n    // If specified, try best possible settings.", context_start)
        require(context_start >= 0 and context_end > context_start,
                "Could not isolate the SDL OpenGL context setup")
        original_context = block[context_start:context_end]
        require("SDL_GL_CreateContext" in original_context and "return nullptr;" in original_context,
                "Could not locate the complete SDL GL context failure check")
        phone_context = (
            "    /* Create OpenGL context */\n"
            "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
            "    // The phone EGL context is already current; never ask SDL dummy video for GL.\n"
            "    GLContext = NULL;\n"
            "#else\n"
            "    if (!(GLContext = SDL_GL_CreateContext(GfuiWindow)))\n"
            "    {\n"
            "        GfLogError(\"SDL_GL_CreateContext failed\");\n"
            "        return nullptr;\n"
            "    }\n"
            "#endif\n"
        )
        block = block[:context_start] + phone_context + block[context_end:]
        # Preserve the upstream if/else braces; only disable the fullscreen call on phones.
        # Match the upstream line by syntax, not exact whitespace.
        fullscreen_else_re = re.compile(
            r"(?m)^([ \t]*})[ \t]*else[ \t]+SDL_SetWindowFullscreen\(GfuiWindow,[ \t]*SDL_WINDOW_FULLSCREEN\);"
        )
        block, count = fullscreen_else_re.subn(
            lambda match: match.group(1) + " else {\n#ifndef SD_PHONE_UPSTREAM_MAIN\n"
            "        SDL_SetWindowFullscreen(GfuiWindow, SDL_WINDOW_FULLSCREEN);\n"
            "#endif\n    }",
            block, count=1)
        require(count == 1,
                "Could not patch the upstream fullscreen else branch")
        text = text[:function_start] + block + text[function_end:]

    resizable_marker = "    GfScrResizable = GfScrGetResizable();"
    if "GfScrResizable = false;" not in text and resizable_marker in text:
        text = replace_once(text, resizable_marker,
            "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
            "    GfScrResizable = false;\n"
            "#else\n"
            "    GfScrResizable = GfScrGetResizable();\n"
            "#endif",
            "phone screen resize-mode selection")

    shutdown_context_re = re.compile(
        r"    SDL_GL_MakeCurrent\s*\(\s*GfuiWindow\s*,\s*GLContext\s*\);\s*\n"
        r"    SDL_GL_DeleteContext\s*\(\s*GLContext\s*\);\s*\n"
        r"    GLContext\s*=\s*NULL;"
    )
    if "phone EGL context remains current on the game thread" not in text:
        text, count = shutdown_context_re.subn(
            "#ifndef SD_PHONE_UPSTREAM_MAIN\n"
            "    SDL_GL_MakeCurrent(GfuiWindow, GLContext);\n"
            "    SDL_GL_DeleteContext(GLContext);\n"
            "#endif\n"
            "    // phone EGL context remains current on the game thread.\n"
            "    GLContext = NULL;",
            text, count=1)
        require(count == 1,
                "Could not isolate the SDL context destruction in GfScrShutdown()")

    require("SDPhoneGetSurfaceSize(&phoneWidth, &phoneHeight)" in text,
            "Phone EGL surface size was not wired into screen initialization")
    return text


def patch_gui(path: Path) -> str:
    """Route GUI/menu input and swapping through the phone EGL surface."""
    text = read(path)
    display_re = re.compile(r"(void\s+GfuiDisplay\s*\(void\)\s*\{\s*)(GfuiRedraw\s*\(\s*\)\s*;)")
    if "SDPhoneUpdateInput(0);" not in text:
        text, count = display_re.subn(
            r"\1#ifdef SD_PHONE_UPSTREAM_MAIN\n\tSDPhoneUpdateInput(0);\n#endif\n\t\2",
            text, count=1)
        require(count == 1, "Could not locate GfuiDisplay() for phone menu input")

    swap_re = re.compile(r"(void\s+GfuiSwapBuffers\s*\(void\)\s*\{\s*)SDL_GL_SwapWindow\s*\(\s*GfuiWindow\s*\)\s*;")
    if "SDPhonePresent();" not in text:
        replacement = (
            r"\1#ifdef SD_PHONE_UPSTREAM_MAIN\n"
            "\tint phoneWidth = 0, phoneHeight = 0;\n"
            "\tSDPhoneGetSurfaceSize(&phoneWidth, &phoneHeight);\n"
            "\tSDPhoneDrawControls(phoneWidth, phoneHeight);\n"
            "\tSDPhonePresent();\n"
            "#else\n"
            "\tSDL_GL_SwapWindow(GfuiWindow);\n"
            "#endif"
        )
        text, count = swap_re.subn(replacement, text, count=1)
        require(count == 1, "Could not locate GfuiSwapBuffers() for EGL presentation")

    proto = (
        "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
        "extern \"C\" void SDPhoneUpdateInput(int racing);\n"
        "extern \"C\" void SDPhoneDrawControls(int width, int height);\n"
        "extern \"C\" void SDPhoneGetSurfaceSize(int* width, int* height);\n"
        "extern \"C\" int SDPhonePresent(void);\n"
        "#endif\n\n"
    )
    if 'extern "C" void SDPhoneUpdateInput(int racing);' not in text:
        marker = re.search(r"(?m)^void\s+GfuiDisplay\s*\(void\)", text)
        require(marker is not None, "Could not find a safe declaration insertion point in gui.cpp")
        text = text[:marker.start()] + proto + text[marker.start():]

    require("SDPhonePresent();" in text, "Phone EGL presentation hook is missing")
    require("SDL_GL_SwapWindow(GfuiWindow);" in text,
            "Desktop SDL swap path must remain available outside phone builds")
    return text


def patch_control(path: Path) -> str:
    """Expose the touch overlay as the game's primary virtual joystick."""
    text = read(path)

    def inject(pattern: str, body: str, label: str) -> None:
        nonlocal text
        if body.strip() in text:
            return
        compiled = re.compile(pattern)
        text, count = compiled.subn(lambda match: match.group(1) + body, text, count=1)
        require(count == 1, f"Could not locate {label} in control.cpp")

    inject(
        r"(int\s+GfctrlJoyIsAnyPresent\s*\(void\)\s*\{\s*)",
        "\n#ifdef SD_PHONE_UPSTREAM_MAIN\n    return GFCTRL_JOY_PRESENT;\n#endif\n",
        "GfctrlJoyIsAnyPresent()")
    inject(
        r"(int\s+GfctrlJoyNumber\s*\(void\)\s*\{\s*)",
        "\n#ifdef SD_PHONE_UPSTREAM_MAIN\n    return 1;\n#endif\n",
        "GfctrlJoyNumber()")
    inject(
        r"(std::string\s*&\s*GfctrlJoyName\s*\(\s*int\s+joy\s*\)\s*\{\s*)",
        '\n#ifdef SD_PHONE_UPSTREAM_MAIN\n'
        '    static std::string phoneControlsName = "Touch Controls";\n'
        '    if (joy == 0) return phoneControlsName;\n'
        '#endif\n',
        "GfctrlJoyName()")
    read_body = (
        "\n#ifdef SD_PHONE_UPSTREAM_MAIN\n"
        "    if (!joyInfo) return -1;\n"
        "    int phoneButtons = 0;\n"
        "    float phoneAxes[8] = {};\n"
        "    if (SDPhoneReadJoystick(&phoneButtons, phoneAxes) != 0) return -1;\n"
        "    const unsigned int previousButtons = static_cast<unsigned int>(joyInfo->oldb[0]);\n"
        "    for (int i = 0; i < GFCTRL_JOY_MAX_AXES * GFCTRL_JOY_NUMBER; ++i)\n"
        "        joyInfo->ax[i] = 0.0f;\n"
        "    for (int i = 0; i < GFCTRL_JOY_MAX_BUTTONS * GFCTRL_JOY_NUMBER; ++i) {\n"
        "        joyInfo->edgeup[i] = 0;\n"
        "        joyInfo->edgedn[i] = 0;\n"
        "        joyInfo->levelup[i] = 0;\n"
        "    }\n"
        "    for (int i = 1; i < GFCTRL_JOY_NUMBER; ++i) joyInfo->oldb[i] = 0;\n"
        "    for (int i = 0; i < GFCTRL_JOY_MAX_AXES && i < 8; ++i)\n"
        "        joyInfo->ax[i] = phoneAxes[i];\n"
        "    const unsigned int currentButtons = static_cast<unsigned int>(phoneButtons);\n"
        "    for (int i = 0; i < GFCTRL_JOY_MAX_BUTTONS; ++i) {\n"
        "        const unsigned int mask = 1u << i;\n"
        "        const bool isPressed = (currentButtons & mask) != 0;\n"
        "        const bool wasPressed = (previousButtons & mask) != 0;\n"
        "        joyInfo->edgeup[i] = isPressed && !wasPressed ? 1 : 0;\n"
        "        joyInfo->edgedn[i] = !isPressed && wasPressed ? 1 : 0;\n"
        "        joyInfo->levelup[i] = isPressed ? 1 : 0;\n"
        "    }\n"
        "    joyInfo->oldb[0] = phoneButtons;\n"
        "    return 0;\n"
        "#endif\n"
    )
    inject(
        r"(GfctrlJoyGetCurrentStates\s*\(\s*tCtrlJoyInfo\s*\*joyInfo\s*\)\s*\{\s*)",
        read_body,
        "GfctrlJoyGetCurrentStates()")

    proto = (
        "#ifdef SD_PHONE_UPSTREAM_MAIN\n"
        'extern "C" int SDPhoneReadJoystick(int* buttons, float* axes);\n'
        "#endif\n\n"
    )
    if 'extern "C" int SDPhoneReadJoystick(int* buttons, float* axes);' not in text:
        # Place the declaration near the joystick API to avoid a global
        # dependency on VR headers in the upstream tgfclient target.
        marker = re.search(r"(?m)^\s*(?:int\s+)?GfctrlJoyGetCurrentStates\s*\(", text)
        require(marker is not None, "Could not find a joystick API declaration point")
        text = text[:marker.start()] + proto + text[marker.start():]

    require("SDPhoneReadJoystick(&phoneButtons, phoneAxes)" in text,
            "Phone joystick state was not connected to the engine")
    return text



def patch_sd_cmake(path: Path) -> str:
    """Skip source entries removed by newer upstream engine revisions."""
    text = read(path)
    old = "        list(APPEND srcs ${dir}/${f})"
    new = (
        "        if(EXISTS \"${dir}/${f}\")\n"
        "            list(APPEND srcs ${dir}/${f})\n"
        "        else()\n"
        "            message(WARNING \"Skipping unavailable upstream source: ${dir}/${f}\")\n"
        "        endif()"
    )
    if "Skipping unavailable upstream source:" not in text and text.count(old) == 2:
        text = text.replace(old, new)
    require(text.count('Skipping unavailable upstream source:') >= 1,
            "Could not add compatibility checks for upstream source lists")
    return text


def apply(reference_root: Path, engine_root: Path) -> list[str]:
    require(reference_root.is_dir(), f"Reference checkout is missing: {reference_root}")
    require(engine_root.is_dir(), f"Engine checkout is missing: {engine_root}")
    expected = ("CMakeLists.txt", "android/app/build.gradle",
        "android/app/src/main/AndroidManifest.xml",
        "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java",
        "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java",
        "android/app/src/main/res/values/strings.xml")
    for item in expected:
        require((reference_root / item).is_file(), f"Reference source missing: {item}")
    for name in PHONE_FILES:
        require((SOURCE_ROOT / name).is_file(), f"Phone source missing: {name}")

    race_path = engine_root / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp"
    race_source = read(race_path)
    upstream_main = "VrRaceEyeBegin" not in race_source

    patches = [
        (reference_root / "CMakeLists.txt",
         lambda path: patch_cmake(path, upstream_main=upstream_main)),
        (reference_root / "cmake/sd.cmake", patch_sd_cmake),
        (reference_root / "android/app/build.gradle", patch_gradle),
        (reference_root / "android/app/src/main/AndroidManifest.xml", patch_manifest),
        (reference_root / "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java", patch_activity),
        (reference_root / "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java", patch_lib),
        (reference_root / "android/app/src/main/res/values/strings.xml", patch_app_name),
        (race_path, lambda path: patch_single_eye_loop(engine_root)),
    ]
    if upstream_main:
        screen_path = engine_root / "src/libs/tgfclient/guiscreen.cpp"
        gui_path = engine_root / "src/libs/tgfclient/gui.cpp"
        control_path = engine_root / "src/libs/tgfclient/control.cpp"
        require(screen_path.is_file(), "Public upstream source lacks src/libs/tgfclient/guiscreen.cpp")
        require(gui_path.is_file(), "Public upstream source lacks src/libs/tgfclient/gui.cpp")
        require(control_path.is_file(), "Public upstream source lacks src/libs/tgfclient/control.cpp")
        patches.extend([
            (screen_path, patch_guiscreen),
            (gui_path, patch_gui),
            (control_path, patch_control),
        ])
    replacements = [(path, patcher(path)) for path, patcher in patches]
    changed = []
    target_dir = reference_root / "src/vr/phone"
    for name in PHONE_FILES:
        source, target = SOURCE_ROOT / name, target_dir / name
        if not target.exists() or target.read_bytes() != source.read_bytes():
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
            changed.append(str(target.relative_to(reference_root)))
    for path, value in replacements:
        if write_if_changed(path, value):
            try:
                changed.append(str(path.relative_to(reference_root)))
            except ValueError:
                changed.append(str(path))
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-root", required=True, type=Path)
    parser.add_argument("--engine-root", type=Path,
                        help="defaults to sibling speed-dreams-code checkout")
    args = parser.parse_args()
    reference = args.reference_root.resolve()
    engine = args.engine_root.resolve() if args.engine_root else reference.parent / "speed-dreams-code"
    try:
        changed = apply(reference, engine)
    except (PortError, OSError, ET.ParseError) as exc:
        print(f"PHONE PORT PATCH FAILED: {exc}", file=sys.stderr)
        return 2
    print(f"Phone backend patch succeeded; changed {len(changed)} path(s).")
    for item in changed:
        print(f"  modified: {item}")
    print("Build and device tests are still required; no APK success is implied.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
