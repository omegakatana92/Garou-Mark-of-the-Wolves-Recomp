#!/usr/bin/env bash
# Build the Linux x86_64 player and package it as a Steam Deck AppImage.
#
# Prerequisites: gcc, g++, cmake 3.21+, make, libsdl2-dev, wget.
# The user's ROMs are never bundled: the AppImage contains the recompiled
# player and the build-time decrypt tool only.
#
# Usage: ./packaging/appimage/build-appimage.sh [ROM_DIR]
# Default ROM_DIR=$ROOT/roms-src, output dist/steamdeck/...
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ROMS="${1:-$ROOT/roms-src}"
BUILD="$ROOT/build-linux"
APPDIR="$BUILD/AppDir"
OUT="$ROOT/dist/steamdeck/Garou_Mark_of_The_Wolves-SteamDeck-x86_64.AppImage"

echo "== configure/build (Release) =="
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release "-DGAROU_ROM_DIR=$ROMS"
cmake --build "$BUILD" --parallel "$(nproc)"

echo "== assemble AppDir =="
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" \
         "$APPDIR/usr/share/doc/garou"
cp "$BUILD/Garou" "$APPDIR/usr/bin/Garou"
cp "$BUILD/garou_decrypt" "$APPDIR/usr/bin/garou_decrypt"
strip --strip-unneeded "$APPDIR/usr/bin/Garou" 2>/dev/null || true
strip --strip-unneeded "$APPDIR/usr/bin/garou_decrypt" 2>/dev/null || true

# Bundle the SDL2 the build linked against; SteamOS's system copy may differ.
SDL=$(ldd "$BUILD/Garou" | awk '/libSDL2-2\.0\.so\.0/{print $3; exit}')
if [ -n "$SDL" ]; then cp "$SDL" "$APPDIR/usr/lib/"; fi

cp "$ROOT/packaging/appimage/garou.png" "$APPDIR/garou.png"
cp "$ROOT/packaging/appimage/garou.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/garou.png"
cp "$ROOT/packaging/appimage/garou.desktop" "$APPDIR/garou.desktop"
cp "$ROOT/packaging/appimage/garou.desktop" "$APPDIR/usr/share/applications/garou.desktop"
cp "$ROOT/packaging/appimage/README.txt" "$APPDIR/usr/share/doc/garou/README.txt"
cp "$ROOT/LICENSE" "$APPDIR/usr/share/doc/garou/LICENSE.txt"
cp "$ROOT/LICENSE-STATUS.md" "$APPDIR/usr/share/doc/garou/LICENSE-STATUS.txt" 2>/dev/null || true
cp "$ROOT/THIRD_PARTY_NOTICES.md" "$APPDIR/usr/share/doc/garou/THIRD_PARTY_NOTICES.txt"
mkdir -p "$APPDIR/usr/share/doc/garou/LICENSES"
cp "$ROOT/neogeorecomp/LICENSE" "$APPDIR/usr/share/doc/garou/LICENSES/neogeorecomp.txt"
cp "$ROOT/neogeorecomp/NOTICE" "$APPDIR/usr/share/doc/garou/LICENSES/neogeorecomp-NOTICE.txt"
cp "$ROOT/neogeorecomp/LICENSES/"*.txt "$APPDIR/usr/share/doc/garou/LICENSES/"
cp "$ROOT/packaging/windows/SDL2.txt" "$APPDIR/usr/share/doc/garou/LICENSES/SDL2.txt"

cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Explicit --rom-path always wins.
has_rom=0
for a in "$@"; do [ "$a" = "--rom-path" ] && has_rom=1; done

if [ "$has_rom" = 0 ]; then
    cand=""
    if [ -n "${GAROU_ROM_DIR:-}" ]; then
        cand="$GAROU_ROM_DIR"
    elif [ -n "${APPIMAGE:-}" ]; then
        for d in "$(dirname "$APPIMAGE")/roms" \
                 "$(dirname "$APPIMAGE")/roms/garou" \
                 "$HOME/Emulation/roms/garou" \
                 "$HOME/roms/garou" "$HOME/roms"; do
            if [ -d "$d" ]; then cand="$d"; break; fi
        done
    fi
    if [ -n "$cand" ]; then
        if [ -f "$cand/253-pd.bin" ]; then
            set -- --rom-path "$cand" "$@"
        elif [ -f "$cand/253-ep1.p1" ]; then
            # The user pointed us at the original encrypted set: decrypt into
            # the per-user data directory once, then launch from there.
            CACHE="$HOME/.local/share/garou/roms"
            if [ ! -f "$CACHE/253-pd.bin" ]; then
                mkdir -p "$CACHE"
                "$HERE/usr/bin/garou_decrypt" --rom-path "$cand" --out "$CACHE" || exit 1
            fi
            set -- --rom-path "$CACHE" "$@"
        else
            set -- --rom-path "$cand" "$@"
        fi
    fi
fi

# Steam Deck / Gaming Mode wants a fullscreen game.  Keep --headless and an
# explicit --scale or --fullscreen from being overridden.
add_fs=1
for a in "$@"; do
    case "$a" in --headless|--fullscreen|--scale) add_fs=0;; esac
done
[ "$add_fs" = 1 ] && set -- --fullscreen "$@"

exec "$HERE/usr/bin/Garou" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo "== appimagetool =="
mkdir -p "$ROOT/dist/steamdeck"
TOOL="${APPIMAGETOOL:-$HOME/appimagetool-x86_64.AppImage}"
if [ ! -x "$TOOL" ]; then
    TOOL="$BUILD/appimagetool.AppImage"
    wget -q -O "$TOOL" \
      https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$TOOL"
fi
ARCH=x86_64 "$TOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "$OUT"
chmod +x "$OUT"
echo "wrote $OUT"
