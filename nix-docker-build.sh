#!/usr/bin/env bash
# Build NBD using Nix in Docker and extract binaries
# Usage: ./nix-docker-build.sh [output-dir]
#
# This script builds NBD in a reproducible way using Nix inside Docker,
# then extracts the artifacts using docker cp.
#
# Build outputs:
#   - nbd-server:      NBD server with TLS support
#   - nbd-client:      NBD client with TLS support
#   - min-nbd-client:  NBD client without TLS (minimal)
#   - nbd-get-status:  Query NBD device status (requires netlink)
#   - nbd-trdump:      Trace dump utility
#   - nbd-trplay:      Trace replay utility

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${1:-$SCRIPT_DIR/nix-build-output}"
IMAGE_NAME="nbd-nix-build"
CONTAINER_NAME="nbd-build-$$"

cleanup() {
    docker rm "$CONTAINER_NAME" 2>/dev/null || true
}
trap cleanup EXIT

echo "=== NBD Nix Build ==="
echo "Output directory: $OUTPUT_DIR"
echo ""

# Clean output directory
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Build the Docker image
echo "=== Building Docker image ==="
docker build -f "$SCRIPT_DIR/Dockerfile.nix" -t "$IMAGE_NAME" "$SCRIPT_DIR"

# Create a container (don't run it)
echo ""
echo "=== Extracting artifacts ==="
docker create --name "$CONTAINER_NAME" "$IMAGE_NAME"

# Extract using tar to preserve structure but reset ownership
docker cp "$CONTAINER_NAME:/output" - | tar -xf - -C "$OUTPUT_DIR" --strip-components=1

echo "Binaries are patched and ready for Debian 12+ and Ubuntu 22.04+"

# Fix ownership to current user
# if [ "$(id -u)" != "0" ]; then
#     sudo chown -R "$(id -u):$(id -g)" "$OUTPUT_DIR" 2>/dev/null || true
# fi

echo ""
echo "=== Build complete ==="
echo "Artifacts exported to: $OUTPUT_DIR"
echo ""

# Show binaries
echo "Binaries:"
ls -la "$OUTPUT_DIR/"

echo ""
echo "SHA256 checksums:"
(cd "$OUTPUT_DIR" && sha256sum *)
