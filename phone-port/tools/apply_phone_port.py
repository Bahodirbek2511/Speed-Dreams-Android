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


def patch_cmake(path: Path) -> str:
    text = read(path)
    dollar = chr(36)
    vdir = dollar + "{VR_DIR}"
    sroot = dollar + "{SD_SRC}"
    if "option(SD_PHONE" not in text:
        text = replace_once(text, "project(sdvr C CXX)",
            'project(sdvr C CXX)\n\noption(SD_PHONE "Build the flat-screen Android phone port" OFF)',
            "project declaration")

    original_find = "find_package(OpenXR REQUIRED CONFIG)"
    if original_find in text and "if(NOT SD_PHONE)\\n    find_package(OpenXR REQUIRED CONFIG)\\nendif()" not in text:
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
            '    target_compile_definitions(sdvr PRIVATE SD_PHONE=1)\n'
            '    target_compile_definitions(sd_headers INTERFACE SD_PHONE=1)\n'
            'endif()',
            "Speed Dreams module CMake include")

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
        elif tag == "uses-feature" and name in {
            "android.hardware.vr.headtracking", "oculus.software.handtracking"
        }:
            root.remove(child)
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
    return value + "\n"


def patch_activity(path: Path) -> str:
    text = read(path)
    if "import android.view.MotionEvent;" not in text:
        text = replace_once(text, "import android.view.SurfaceView;",
                            "import android.view.SurfaceView;\nimport android.view.MotionEvent;",
                            "SurfaceView import")
    for unused in ("import static android.system.Os.setenv;\n",
                   "import android.os.Build;\n", "import java.util.Locale;\n"):
        text = text.replace(unused, "")
    static_pattern = re.compile(
        r"\s*static\s*\{\s*String manufacturer.*?System\.loadLibrary\(\"sdvr\"\);\s*\}",
        re.DOTALL)
    if 'System.loadLibrary("openxr_loader")' in text or "OPENXR_HMD" in text:
        text, count = static_pattern.subn('\n    static {\n        System.loadLibrary("sdvr");\n    }', text, count=1)
        require(count == 1, "Could not replace VR-only static native library loader")
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
    require(calls, "Engine source lacks VrRaceEyeBegin hooks; wrong source branch?")
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

    patches = [
        (reference_root / "CMakeLists.txt", patch_cmake),
        (reference_root / "android/app/build.gradle", patch_gradle),
        (reference_root / "android/app/src/main/AndroidManifest.xml", patch_manifest),
        (reference_root / "android/app/src/main/java/com/speeddreamsvr/SDVRActivity.java", patch_activity),
        (reference_root / "android/app/src/main/java/com/speeddreamsvr/SDVRLib.java", patch_lib),
        (reference_root / "android/app/src/main/res/values/strings.xml", patch_app_name),
        (engine_root / "src/modules/userinterface/legacymenu/racescreens/racerunningmenus.cpp",
         lambda path: patch_single_eye_loop(engine_root)),
    ]
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
