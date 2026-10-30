#!/usr/bin/env bash
# Builds the PS5 Dig Dug title (PPSA07800) with ps5-native-app-boilerplate.
# Run inside WSL (Debian):  bash build.sh            -> build only
#                           bash build.sh deploy IP  -> build and FTP to /data/homebrew
#
# The boilerplate work copy lives in ~/ps5-projects/digdug-ps5; this folder's
# src/ and sce_sys/ are synced into it before every build.
set -euo pipefail

here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
work=${DIGDUG_WORK:-$HOME/ps5-projects/digdug-ps5}
boilerplate=${BOILERPLATE:-$HOME/ps5-projects/ps5-native-app-boilerplate}

if [[ ! -d $work ]]; then
    echo "==> creating work copy $work from $boilerplate"
    mkdir -p "$work"
    (cd "$boilerplate" && tar --exclude=./.git --exclude=./build --exclude=./dist \
        --exclude=./src --exclude=./assets -cf - .) | tar -xf - -C "$work"
fi

rm -rf "$work/src" "$work/sce_sys"
mkdir -p "$work/src" "$work/sce_sys" "$work/assets"
cp "$here"/src/*.cpp "$here"/src/*.h "$work/src/"
cp "$here"/sce_sys/* "$work/sce_sys/"

# The title packages no files under /app0/assets.
cat > "$work/.env" <<'EOF'
APP_ASSETS =
EOF

cd "$work"
if [[ ${1:-} == deploy ]]; then
    make deploy PS5_HOST="${2:?usage: build.sh deploy <ps5-ip>}"
else
    make app
fi

mkdir -p "$here/dist"
rm -rf "$here/dist/PPSA07800"
cp -a "$work/dist/PPSA07800" "$work/dist/PPSA07800.zip" "$here/dist/"
echo "==> output: $here/dist/PPSA07800 (folder) and PPSA07800.zip"
