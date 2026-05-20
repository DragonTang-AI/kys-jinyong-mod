#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PREBUILT="${ROOT}/thirdparty/prebuilt"

fail=0

need_file() {
  local p="$1"
  if [[ ! -e "$p" ]]; then
    echo "MISSING: $p"
    fail=1
  else
    echo "OK:      $p"
  fi
}

echo "== Checking headers =="
need_file "${PREBUILT}/include/SDL3/SDL.h"
need_file "${PREBUILT}/include/SDL3_image/SDL_image.h"
need_file "${PREBUILT}/include/SDL3_ttf/SDL_ttf.h"
need_file "${PREBUILT}/include/SDL3_mixer/SDL_mixer.h"
need_file "${PREBUILT}/include/lua.hpp"
need_file "${PREBUILT}/include/glaze/glaze.hpp"
need_file "${PREBUILT}/include/yaml-cpp/yaml.h"
need_file "${PREBUILT}/include/zip.h"

echo
echo "== Checking libraries (either layout is acceptable) =="
for plat in iphoneos iphonesimulator; do
  echo "-- ${plat} --"
  # allow either prebuilt/<plat>/lib or prebuilt/lib/<plat>
  libdir1="${PREBUILT}/${plat}/lib"
  libdir2="${PREBUILT}/lib/${plat}"
  if [[ -d "${libdir1}" ]]; then
    libdir="${libdir1}"
  else
    libdir="${libdir2}"
  fi

  need_file "${libdir}/libSDL3.a"
  need_file "${libdir}/libSDL3_image.a"
  need_file "${libdir}/libSDL3_ttf.a"
  need_file "${libdir}/libSDL3_mixer.a"
  need_file "${libdir}/liblua.a"
  need_file "${libdir}/libyaml-cpp.a"
  need_file "${libdir}/libzip.a"
done

echo
if [[ "${fail}" -ne 0 ]]; then
  echo "Prebuilt is NOT ready."
  echo
  echo "Put headers under:"
  echo "  ${PREBUILT}/include/"
  echo "Put libs under ONE of:"
  echo "  ${PREBUILT}/iphoneos/lib  +  ${PREBUILT}/iphonesimulator/lib"
  echo "  ${PREBUILT}/lib/iphoneos  +  ${PREBUILT}/lib/iphonesimulator"
  exit 2
fi

echo "Prebuilt looks ready."
