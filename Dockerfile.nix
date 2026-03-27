# Dockerfile for reproducible NBD build using Nix
# Usage:
#   docker build -f Dockerfile.nix -t nbd-nix-build .
#   docker create --name nbd-build nbd-nix-build
#   docker cp nbd-build:/output/. ./nix-build-output/
#   docker rm nbd-build

# Pinned to specific digest for reproducibility
# Tag: nixos/nix:2.33.0
FROM nixos/nix@sha256:081b65e50a5c4e6ef4a9094a462da3b83ff76bfec70236eb010047fcee36e11c

# Copy source files
COPY . /src

# Build NBD using Nix
RUN cd /src && \
    nix-build default.nix && \
    mkdir -p /output && \
    cp -L result/bin/* /output/

# Install patchelf and strip for binary patching
RUN nix-env -iA nixpkgs.patchelf nixpkgs.binutils

# Patch binaries to use system interpreter for Debian 12+ / Ubuntu 22.04+ compatibility
RUN for binary in /output/*; do \
        patchelf --set-interpreter /lib64/ld-linux-x86-64.so.2 "$binary" || true; \
        patchelf --remove-rpath "$binary" || true; \
    done

# Strip versioned symbols to eliminate libnl warnings
# This removes symbol versioning that causes warnings on system libraries
RUN for binary in /output/*; do \
        strip --strip-all "$binary" 2>/dev/null || true; \
    done

# Show what was built
RUN ls -la /output/

# Default command shows build info
CMD ["sh", "-c", "echo 'NBD Nix Build Complete. Use docker cp to extract artifacts from /output/'"]
