#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
IMAGE_NAME="schwung-midiverb-builder"

if [ -z "$CROSS_PREFIX" ] && [ ! -f "/.dockerenv" ]; then
    echo "=== Building Midiverb Module (via Docker) ==="
    if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
        docker build -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile" "$REPO_ROOT"
    fi
    docker run --rm \
        -v "$REPO_ROOT:/build" \
        -u "$(id -u):$(id -g)" \
        -w /build \
        "$IMAGE_NAME" \
        ./scripts/build.sh
    exit 0
fi

CROSS_PREFIX="${CROSS_PREFIX:-aarch64-linux-gnu-}"

cd "$REPO_ROOT"
mkdir -p build dist/midiverb

${CROSS_PREFIX}gcc -O3 -shared -fPIC \
    -march=armv8-a -mtune=cortex-a72 \
    -ffast-math -fomit-frame-pointer -fno-stack-protector \
    -DNDEBUG \
    -Isrc/dsp \
    src/dsp/plugin.c \
    src/dsp/midiverb_core.c \
    src/dsp/resampler.c \
    -o build/midiverb.so \
    -lm

cp src/module.json dist/midiverb/module.json
[ -f src/help.json ] && cp src/help.json dist/midiverb/help.json
cp build/midiverb.so dist/midiverb/midiverb.so
cp src/dsp/THIRD_PARTY_LICENSES.md dist/midiverb/THIRD_PARTY_LICENSES.md
chmod +x dist/midiverb/midiverb.so

cd dist
tar -czvf midiverb-module.tar.gz midiverb/
echo "OK: dist/midiverb-module.tar.gz"
