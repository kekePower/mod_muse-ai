#!/bin/bash

# Build script for modular Apache module
set -e

OUTDIR="$1"
APXS="$2"

echo "Building modular Apache module with apxs..."
echo "Output directory: $OUTDIR"
echo "APXS: $APXS"

cd "$OUTDIR"

# Copy all source files
echo "Copying source files..."
cp ../src/mod_muse_ai_main.c .
cp ../src/streaming.c .
cp ../src/sanitize.c .
cp ../src/http_client.c .
cp ../src/utils.c .
cp ../src/mod_muse_ai.h .

echo "Building with apxs..."
"$APXS" -c mod_muse_ai_main.c streaming.c sanitize.c http_client.c utils.c

# apxs creates mod_muse_ai_main.so, but we need mod_muse_ai.so
if [ -f .libs/mod_muse_ai_main.so ]; then
    cp .libs/mod_muse_ai_main.so mod_muse_ai.so
    ls -la mod_muse_ai.so
    echo "Module built successfully: $OUTDIR/mod_muse_ai.so"
    echo "File size: $(stat -c%s mod_muse_ai.so) bytes"
elif [ -f mod_muse_ai_main.so ]; then
    cp mod_muse_ai_main.so mod_muse_ai.so
    ls -la mod_muse_ai.so
    echo "Module built successfully: $OUTDIR/mod_muse_ai.so"
    echo "File size: $(stat -c%s mod_muse_ai.so) bytes"
else
    echo "ERROR: Module build failed! No .so file found."
    ls -la *.so .libs/*.so 2>/dev/null || echo "No .so files found"
    exit 1
fi
