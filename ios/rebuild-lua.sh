#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${ROOT}/thirdparty/src"
PREBUILT="${ROOT}/thirdparty/prebuilt"

IOS_MIN="16.3"
ARCH="arm64"

SDK_OS="$(xcrun --sdk iphoneos --show-sdk-path)"
SDK_SIM="$(xcrun --sdk iphonesimulator --show-sdk-path)"

LUA_VER="5.4.7"
LUA_DIR="${SRC_DIR}/lua-${LUA_VER}"
if [[ ! -d "${LUA_DIR}" ]]; then
  mkdir -p "${SRC_DIR}"
  curl -L -o "${SRC_DIR}/lua-${LUA_VER}.tar.gz" "https://www.lua.org/ftp/lua-${LUA_VER}.tar.gz"
  tar -xzf "${SRC_DIR}/lua-${LUA_VER}.tar.gz" -C "${SRC_DIR}"
fi

build_one() {
  local plat="$1" # iphoneos | iphonesimulator
  local sdk="$plat"
  local sysroot="$SDK_OS"
  local target="arm64-apple-ios${IOS_MIN}"
  local minflag="-miphoneos-version-min=${IOS_MIN}"
  if [[ "$plat" == "iphonesimulator" ]]; then
    sdk="iphonesimulator"
    sysroot="$SDK_SIM"
    target="arm64-apple-ios${IOS_MIN}-simulator"
    minflag="-mios-simulator-version-min=${IOS_MIN}"
  fi

  local cc ar
  cc="$(xcrun --sdk "$sdk" --find clang)"
  ar="$(xcrun --sdk "$sdk" --find ar)"

  local outlib="${PREBUILT}/${plat}/lib/liblua.a"
  local outinc="${PREBUILT}/include"
  mkdir -p "$(dirname "$outlib")" "$outinc"

  local cflags="-O2 -fPIC -arch ${ARCH} -isysroot ${sysroot} -target ${target} ${minflag} -DLUA_USE_IOS"
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

build_one iphoneos
build_one iphonesimulator

echo "rebuilt lua to ${PREBUILT}"
