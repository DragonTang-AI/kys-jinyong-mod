#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "${DIR}"

if [[ ! -f "${DIR}/BundledResources/game.zip" && -f "${DIR}/../game.zip" ]]; then
    ln -sf "../game.zip" "${DIR}/BundledResources/game.zip"
fi

if [[ ! -f "${DIR}/BundledResources/game.zip" ]]; then
    echo "生成占位 game.zip（请稍后用 scripts/pack_game_zip.sh 结果覆盖）..."
    mkdir -p "${DIR}/BundledResources"
    python3 -c "import zipfile, pathlib; p=pathlib.Path('${DIR}/BundledResources/game.zip'); p.parent.mkdir(parents=True, exist_ok=True); z=zipfile.ZipFile(p,'w'); z.writestr('__REPLACE__.txt', b'Replace with scripts/pack_game_zip.sh'); z.close()"
fi

if ! command -v xcodegen >/dev/null 2>&1; then
    echo "请先安装 XcodeGen：brew install xcodegen" >&2
    exit 1
fi

xcodegen generate --spec project.yml
echo "生成完成: ${DIR}/KYSCPP.xcodeproj"
