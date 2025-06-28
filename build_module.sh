#!/bin/bash
set -e

SOURCE="$1"
OUTPUT="$2"
APXS="$3"

echo "Building Apache module with apxs..."
echo "Source: $SOURCE"
echo "Output: $OUTPUT"
echo "APXS: $APXS"

# Remember the original working directory
ORIGINAL_DIR=$(pwd)

# Get the directory of the source file
SOURCE_DIR=$(dirname "$SOURCE")
SOURCE_FILE=$(basename "$SOURCE")

# Make OUTPUT path absolute if it's relative
if [[ "$OUTPUT" != /* ]]; then
    OUTPUT="$ORIGINAL_DIR/$OUTPUT"
fi

# Change to source directory to run apxs
cd "$SOURCE_DIR"

# Run apxs to build the module
"$APXS" -c "$SOURCE_FILE"

# Copy the resulting .so file to the expected location
if [ -f ".libs/mod_muse_ai.so" ]; then
    # Get the absolute path of the output file
    OUTPUT_DIR=$(dirname "$OUTPUT")
    OUTPUT_FILE=$(basename "$OUTPUT")
    
    # Make sure output directory exists
    mkdir -p "$OUTPUT_DIR"
    
    # Copy using absolute path
    cp .libs/mod_muse_ai.so "$OUTPUT"
    echo "Module built successfully: $OUTPUT"
    echo "File size: $(stat -c%s "$OUTPUT") bytes"
else
    echo "Error: .libs/mod_muse_ai.so not found"
    echo "Current directory: $(pwd)"
    ls -la .libs/ || echo "No .libs directory found"
    exit 1
fi
