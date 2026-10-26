#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="${1:?usage: bootstrap_speed_dreams_android_build.sh WORK_DIR}"
WORK="$(mkdir -p "$WORK" && cd "$WORK" && pwd)"
VR="$WORK/SpeedDreams-VR"
ENGINE="$WORK/speed-dreams-code"
TP="$VR/third_party"
MANIFEST="$WORK/android-source-lock.txt"

clone_pinned() {
    local dest="$1" url="$2" ref="$3" marker="$4"
    if [[ -e "$dest" ]]; then
        if [[ ! -d "$dest/.git" ]]; then
            echo "Refusing to overwrite existing non-git dependency path: $dest" >&2
            return 2
        fi
        local actual
        actual="$(git -C "$dest" rev-parse HEAD)"
        echo "EXISTING $dest $actual"
        if ! git -C "$dest" describe --exact-match --tags HEAD 2>/dev/null | grep -Fxq "$ref"; then
            echo "Existing checkout at $dest is not exactly at expected tag $ref; refusing to modify it." >&2
            return 2
        fi
    else
        mkdir -p "$(dirname "$dest")"
        git -c advice.detachedHead=false clone --depth 1 --single-branch --branch "$ref" "$url" "$dest"
    fi
    if [[ ! -e "$dest/$marker" ]]; then
        echo "Missing expected file after checkout: $dest/$marker" >&2
        return 3
    fi
    printf '%s\t%s\t%s\n' "$(basename "$dest")" "$ref" "$(git -C "$dest" rev-parse HEAD)" >> "$MANIFEST"
}

: > "$MANIFEST"

if [[ ! -d "$VR/.git" ]]; then
    git -c advice.detachedHead=false clone --depth 1 --single-branch --branch v0.10.0-beta \
        https://github.com/SgtBilko76/SpeedDreams-VR.git "$VR"
fi
printf 'SpeedDreams-VR\tv0.10.0-beta\t%s\n' "$(git -C "$VR" rev-parse HEAD)" >> "$MANIFEST"

if [[ ! -d "$ENGINE/.git" ]]; then
    git -c advice.detachedHead=false clone --depth 1 --recurse-submodules --shallow-submodules \
        --single-branch --branch main \
        https://forge.a-lec.org/speed-dreams/speed-dreams-code.git "$ENGINE"
else
    git -C "$ENGINE" submodule update --init --depth 1
fi
printf 'speed-dreams-code\tmain\t%s\n' "$(git -C "$ENGINE" rev-parse HEAD)" >> "$MANIFEST"
git -C "$ENGINE" submodule status --recursive | tee "$WORK/engine-submodules.txt"

for item in \
    "zlib|https://github.com/madler/zlib.git|v1.3.1|zlib.h" \
    "libpng|https://github.com/pnggroup/libpng.git|v1.6.43|scripts/pnglibconf.h.prebuilt" \
    "libjpeg-turbo|https://github.com/libjpeg-turbo/libjpeg-turbo.git|3.0.4|jconfig.h.in" \
    "freetype|https://github.com/freetype/freetype.git|VER-2-13-2|CMakeLists.txt" \
    "SDL2|https://github.com/libsdl-org/SDL.git|release-2.30.11|include/SDL.h" \
    "SDL2_ttf|https://github.com/libsdl-org/SDL_ttf.git|release-2.0.14|SDL_ttf.c" \
    "SDL2_mixer|https://github.com/libsdl-org/SDL_mixer.git|release-2.8.0|src/codecs/music_minimp3.c" \
    "enet|https://github.com/lsalzman/enet.git|v1.3.18|CMakeLists.txt" \
    "mbedtls|https://github.com/Mbed-TLS/mbedtls.git|v3.5.1|CMakeLists.txt" \
    "curl|https://github.com/curl/curl.git|curl-8_6_0|CMakeLists.txt" \
    "cJSON|https://github.com/DaveGamble/cJSON.git|v1.7.18|cJSON.c" \
    "rhash|https://github.com/rhash/RHash.git|v1.4.5|librhash/rhash.h" \
    "gl4es|https://github.com/ptitSeb/gl4es.git|v1.1.5|include/gl4esinit.h" \
    "openal-soft|https://github.com/kcat/openal-soft.git|1.23.1|CMakeLists.txt"; do
    IFS='|' read -r name url ref marker <<< "$item"
    clone_pinned "$TP/$name" "$url" "$ref" "$marker"
done

for required in \
    "$ENGINE/src/libs/tgf/tgf.h" \
    "$ENGINE/freesolid/include" \
    "$ENGINE/speed-dreams-data/data"; do
    if [[ ! -e "$required" ]]; then
        echo "Required upstream checkout/submodule is missing: $required" >&2
        echo "Engine submodules present:" >&2
        git -C "$ENGINE" submodule status --recursive >&2 || true
        exit 4
    fi
done

python3 "$ROOT/phone-port/tools/apply_phone_port.py" \
    --reference-root "$VR" --engine-root "$ENGINE"

echo "Android source/dependency lock:"
cat "$MANIFEST"
echo "WORK=$WORK"
