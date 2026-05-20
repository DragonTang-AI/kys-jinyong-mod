#!/usr/bin/env bash
# 在仓库根目录生成供 iOS/Android 包内使用的 game.zip（根目录为 config、resource 等）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GAME="${ROOT}/game"
OUT="${ROOT}/game.zip"
if [[ ! -d "${GAME}/config" ]]; then
    echo "error: ${GAME}/config not found — put game assets under game/ first." >&2
    exit 1
fi
rm -f "${OUT}"
( cd "${GAME}" && zip -r -q "${OUT}" . )
echo "wrote ${OUT}"
