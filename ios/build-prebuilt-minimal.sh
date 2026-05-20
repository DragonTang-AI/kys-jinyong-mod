#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${ROOT}/thirdparty/src"
BUILD_DIR="${ROOT}/thirdparty/build"
PREBUILT="${ROOT}/thirdparty/prebuilt"

IOS_MIN="15.0"
ARCH="arm64"

mkdir -p "${SRC_DIR}" "${BUILD_DIR}" "${PREBUILT}/include" "${PREBUILT}/iphoneos/lib" "${PREBUILT}/iphonesimulator/lib"

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "missing command: $1" >&2; exit 1; }
}

require_cmd git
require_cmd cmake
require_cmd xcrun
require_cmd make

echo "== Toolchain =="
SDK_OS="$(xcrun --sdk iphoneos --show-sdk-path)"
SDK_SIM="$(xcrun --sdk iphonesimulator --show-sdk-path)"
echo "iphoneos sdk: ${SDK_OS}"
echo "iphonesimulator sdk: ${SDK_SIM}"

git_clone_or_update() {
  local url="$1"
  local dir="$2"
  local ref="${3:-}"
  if [[ -d "${dir}/.git" ]]; then
    echo "== Updating $(basename "$dir") =="
    git -C "$dir" fetch --tags --prune
  else
    echo "== Cloning $(basename "$dir") =="
    git clone --depth 1 "$url" "$dir"
    git -C "$dir" fetch --tags --prune
  fi
  if [[ -n "$ref" ]]; then
    git -C "$dir" checkout "$ref"
  fi
  # Some SDL_* repos use submodules for vendored deps (external/*)
  git -C "$dir" submodule update --init --recursive || true
}

cmake_build_install() {
  local name="$1"
  local src="$2"
  local plat="$3"            # iphoneos | iphonesimulator
  shift 3
  local extra=("$@")

  local sdk="iphoneos"
  local sysroot="${SDK_OS}"
  if [[ "$plat" == "iphonesimulator" ]]; then
    sdk="iphonesimulator"
    sysroot="${SDK_SIM}"
  fi

  local bdir="${BUILD_DIR}/${name}-${plat}"
  local prefix="${BUILD_DIR}/stage/${plat}"
  rm -rf "$bdir"
  mkdir -p "$bdir" "$prefix"

  cmake -S "$src" -B "$bdir" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT="$sysroot" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_MIN" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    "${extra[@]}"

  cmake --build "$bdir" --config Release -j"$(sysctl -n hw.ncpu)"
  cmake --install "$bdir" --config Release
}

copy_headers_once() {
  local from="$1"
  if [[ -d "$from" ]]; then
    rsync -a "$from/" "${PREBUILT}/include/"
  fi
}

copy_libs() {
  local plat="$1"
  local staged="${BUILD_DIR}/stage/${plat}"
  local out="${PREBUILT}/${plat}/lib"
  mkdir -p "$out"
  if [[ -d "${staged}/lib" ]]; then
    find "${staged}/lib" -maxdepth 1 -name "lib*.a" -print -exec cp -f {} "$out/" \;
  fi
}

echo "== Fetch sources =="
git_clone_or_update https://github.com/libsdl-org/SDL.git "${SRC_DIR}/SDL" main
git_clone_or_update https://github.com/libsdl-org/SDL_image.git "${SRC_DIR}/SDL_image" main
git_clone_or_update https://github.com/libsdl-org/SDL_ttf.git "${SRC_DIR}/SDL_ttf" main
git_clone_or_update https://github.com/libsdl-org/SDL_mixer.git "${SRC_DIR}/SDL_mixer" main
git_clone_or_update https://github.com/jbeder/yaml-cpp.git "${SRC_DIR}/yaml-cpp"
git_clone_or_update https://github.com/nih-at/libzip.git "${SRC_DIR}/libzip"
git_clone_or_update https://github.com/stephenberry/glaze.git "${SRC_DIR}/glaze"

echo "== Build SDL3 (static) =="
for plat in iphoneos iphonesimulator; do
  cmake_build_install SDL3 "${SRC_DIR}/SDL" "$plat" \
    -DSDL_SHARED=OFF -DSDL_STATIC=ON \
    -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_TEST=OFF \
    -DSDL_INSTALL=ON
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build SDL3_image (vendored, minimal formats) =="
for plat in iphoneos iphonesimulator; do
  SDL3_DIR_PATH="${BUILD_DIR}/stage/${plat}/lib/cmake/SDL3"
  cmake_build_install SDL3_image "${SRC_DIR}/SDL_image" "$plat" \
    -DSDLIMAGE_VENDORED=OFF \
    -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF \
    -DSDLIMAGE_BACKEND_IMAGEIO=ON \
    -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_SVG=OFF \
    -DSDL3_DIR="${SDL3_DIR_PATH}" -DCMAKE_PREFIX_PATH="${BUILD_DIR}/stage/${plat}"
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build SDL3_ttf (vendored) =="
for plat in iphoneos iphonesimulator; do
  SDL3_DIR_PATH="${BUILD_DIR}/stage/${plat}/lib/cmake/SDL3"
  cmake_build_install SDL3_ttf "${SRC_DIR}/SDL_ttf" "$plat" \
    -DSDLTTF_VENDORED=ON \
    -DSDLTTF_SAMPLES=OFF -DSDLTTF_TESTS=OFF \
    -DSDL3_DIR="${SDL3_DIR_PATH}" -DCMAKE_PREFIX_PATH="${BUILD_DIR}/stage/${plat}"
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build SDL3_mixer (vendored: MIDI via Timidity, no FluidSynth) =="
for plat in iphoneos iphonesimulator; do
  SDL3_DIR_PATH="${BUILD_DIR}/stage/${plat}/lib/cmake/SDL3"
  cmake_build_install SDL3_mixer "${SRC_DIR}/SDL_mixer" "$plat" \
    -DSDLMIXER_VENDORED=ON \
    -DSDLMIXER_DEPS_SHARED=OFF \
    -DSDLMIXER_SAMPLES=OFF -DSDLMIXER_TESTS=OFF \
    -DSDLMIXER_MIDI=ON \
    -DSDLMIXER_MIDI_TIMIDITY=ON \
    -DSDLMIXER_MP3_MPG123=OFF \
    -DSDL3_DIR="${SDL3_DIR_PATH}" -DCMAKE_PREFIX_PATH="${BUILD_DIR}/stage/${plat}"
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build yaml-cpp (static) =="
for plat in iphoneos iphonesimulator; do
  cmake_build_install yaml-cpp "${SRC_DIR}/yaml-cpp" "$plat" \
    -DYAML_BUILD_SHARED_LIBS=OFF -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build libzip (static; use system zlib) =="
for plat in iphoneos iphonesimulator; do
  cmake_build_install libzip "${SRC_DIR}/libzip" "$plat" \
    -DBUILD_TOOLS=OFF -DBUILD_REGRESS=OFF -DBUILD_EXAMPLES=OFF \
    -DENABLE_BZIP2=OFF -DENABLE_LZMA=OFF -DENABLE_ZSTD=OFF -DENABLE_OPENSSL=OFF \
    -DLIBZIP_DO_INSTALL=ON
done
copy_headers_once "${BUILD_DIR}/stage/iphoneos/include"
for plat in iphoneos iphonesimulator; do copy_libs "$plat"; done

echo "== Build Lua 5.4 (static) =="
LUA_VER="5.4.7"
LUA_DIR="${SRC_DIR}/lua-${LUA_VER}"
if [[ ! -d "${LUA_DIR}" ]]; then
  curl -L -o "${SRC_DIR}/lua-${LUA_VER}.tar.gz" "https://www.lua.org/ftp/lua-${LUA_VER}.tar.gz"
  tar -xzf "${SRC_DIR}/lua-${LUA_VER}.tar.gz" -C "${SRC_DIR}"
fi

build_lua_one() {
  local plat="$1"
  local sdk="iphoneos"
  local sysroot="${SDK_OS}"
  if [[ "$plat" == "iphonesimulator" ]]; then
    sdk="iphonesimulator"
    sysroot="${SDK_SIM}"
  fi
  local outlib="${PREBUILT}/${plat}/lib/liblua.a"
  local outinc="${PREBUILT}/include"
  mkdir -p "$(dirname "$outlib")" "$outinc"

  local cc
  cc="$(xcrun --sdk "$sdk" --find clang)"
  local ar
  ar="$(xcrun --sdk "$sdk" --find ar)"

  local cflags="-O2 -fPIC -arch ${ARCH} -isysroot ${sysroot} -miphoneos-version-min=${IOS_MIN} -DLUA_USE_IOS"
  local objs=()
  pushd "${LUA_DIR}/src" >/dev/null
  rm -f ./*.o liblua.a
  for f in *.c; do
    case "$f" in
      lua.c|luac.c) continue ;;
    esac
    "$cc" $cflags -c "$f" -o "${f%.c}.o"
    objs+=("${f%.c}.o")
  done
  "$ar" rcs liblua.a "${objs[@]}"
  cp -f liblua.a "$outlib"
  cp -f lua.h luaconf.h lualib.h lauxlib.h lua.hpp "$outinc/"
  popd >/dev/null
}

build_lua_one iphoneos
build_lua_one iphonesimulator

echo "== Install glaze headers (header-only) =="
if [[ -d "${SRC_DIR}/glaze/include" ]]; then
  rsync -a "${SRC_DIR}/glaze/include/" "${PREBUILT}/include/"
else
  echo "WARN: glaze/include not found at ${SRC_DIR}/glaze/include" >&2
fi

echo
echo "== Done. Running verify-prebuilt.sh =="
"${ROOT}/verify-prebuilt.sh"

echo
echo "Prebuilt artifacts are in: ${PREBUILT}"
