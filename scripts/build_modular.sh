#!/bin/bash

# Build script for modular Apache module
set -e

OUTDIR="$1"
APXS="$2"
shift 2 # The rest of the arguments are source files
SOURCES=($@)

echo "Building modular Apache module with apxs..."
echo "Output directory: $OUTDIR"
echo "APXS: $APXS"
echo "Source files: ${SOURCES[@]}"

cd "$OUTDIR"

# Copy all source files passed from meson
echo "Copying source files..."
cp "${SOURCES[@]}" .

# Get a list of just the .c files for compilation
C_FILES=()
for file in "${SOURCES[@]}"; do
    base_file=$(basename "$file")
    if [[ "$base_file" == *.c ]]; then
        C_FILES+=("$base_file")
    fi
done

echo "Building with apxs..."
"$APXS" -c -o mod_muse_ai.so "${C_FILES[@]}"

# apxs names the .so file after the *first* .c file in the list.
# In our case, it's mod_muse_ai_main.c, so the output is mod_muse_ai_main.so
MAIN_SO_NAME="mod_muse_ai_main.so"
TARGET_SO_NAME="mod_muse_ai.so"

if [ -f .libs/$MAIN_SO_NAME ]; then
    cp .libs/$MAIN_SO_NAME $TARGET_SO_NAME
    ls -la $TARGET_SO_NAME
    echo "Module built successfully: $OUTDIR/$TARGET_SO_NAME"
    echo "File size: $(stat -c%s $TARGET_SO_NAME) bytes"
elif [ -f $MAIN_SO_NAME ]; then
    cp $MAIN_SO_NAME $TARGET_SO_NAME
    ls -la $TARGET_SO_NAME
    echo "Module built successfully: $OUTDIR/$TARGET_SO_NAME"
    echo "File size: $(stat -c%s $TARGET_SO_NAME) bytes"
else
    echo "ERROR: Module build failed! No .so file found."
    ls -la *.so .libs/*.so 2>/dev/null || echo "No .so files found"
    exit 1
fi
