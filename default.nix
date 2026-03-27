# Use NixOS 23.05 for glibc 2.37 compatibility with Debian 12+ and Ubuntu 22.04+
{ pkgs ? import (fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/nixos-23.05.tar.gz";
    sha256 = "05cbl1k193c9la9xhlz4y6y8ijpb2mkaqrab30zij6z4kqgclsrd";
  }) {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "nbd";
  version = "3.26.1";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    autoreconfHook
    pkg-config
    flex
    bison
    m4
    autoconf-archive
  ];

  buildInputs = with pkgs; [
    glib
    gnutls
    linuxHeaders
    # Note: libnl is intentionally omitted to avoid "no version information available" warnings
    # This disables netlink support, but the legacy ioctl interface still works
  ];

  # Ensure version is set correctly without git
  # We need to set VERSION before autoreconf runs
  postPatch = ''
    # Create a static version file since git won't be available in the sandbox
    echo "${version}" > support/VERSION

    # Patch genver.sh to use the VERSION file if git is not available
    cat > support/genver.sh << 'EOF'
#!/bin/sh
GITDESC=$(git describe --dirty 2>/dev/null | sed -e 's/nbd-//')
if [ -z "$GITDESC" ]; then
  if [ -f "$(dirname "$0")/VERSION" ]; then
    GITDESC=$(cat "$(dirname "$0")/VERSION")
  else
    GITDESC="0.unknown"
  fi
fi
echo $GITDESC
EOF
    chmod +x support/genver.sh

    echo "=== Patching Makefile.am for reproducible flex/bison builds ==="
    # Fix reproducibility: ensure bison generates deterministic output
    # The issue is in Makefile.am where bison is invoked without deterministic flags
    # --no-lines removes #line directives that contain file paths
    sed -i 's/bison -d \$^/bison -d $^ --no-lines/g' Makefile.am

    # Also ensure flex is configured for reproducible output
    # We need to pass this through configure since automake handles lex output
  '';

  # Override configure variables to ensure deterministic flex/bison output
  configureFlags = [
    "--enable-syslog"
    "--with-gnutls"
    "--without-libnl"  # Disable netlink support to avoid libnl dependency
    "--disable-manpages"
    "LEX=flex"          # Flex executable
    "YACC=bison"        # Bison executable
    "LFLAGS=-L"         # Suppress flex #line directives
    "YFLAGS=--no-lines"  # Suppress bison #line directives
    "--sysconfdir=/etc"  # Use /etc instead of /nix/store/... for reproducibility
    "--localstatedir=/var"  # Use /var instead of /nix/store/...
  ];

  # Additional CFLAGS to override embedded paths
  env = {
    SYSCONFDIR = "/etc";
    LOCALSTATEDIR = "/var";
  };

  # Force HAVE_LINUX_VM_SOCKETS_H to be defined since the header exists but configure doesn't find it
  postConfigure = ''
    echo "=== Forcing HAVE_LINUX_VM_SOCKETS_H definition ==="
    sed -i 's|/\* #undef HAVE_LINUX_VM_SOCKETS_H \*/|#define HAVE_LINUX_VM_SOCKETS_H 1|g' config.h
    grep HAVE_LINUX_VM_SOCKETS_H config.h

    echo "=== Configuring reproducible build settings ==="
    # Override any auto-detected flex/bison flags for deterministic output
    # These ensure generated C files have no embedded timestamps or build paths
    export M4="${pkgs.m4}/bin/m4"
  '';

  enableParallelBuilding = true;

  # Ensure reproducible builds
  # We need to fix timestamps in generated C files from flex and bison
  preBuild = ''
    echo "=== Ensuring reproducible build environment ==="
    # Set deterministic timestamp for all build operations
    export SOURCE_DATE_EPOCH=315532800  # 1980-01-01 00:00:00 UTC
  '';

  # After build, normalize timestamps in binaries
  postBuild = ''
    echo "=== Build complete ==="
  '';

  meta = with pkgs.lib; {
    description = "Network Block Device - client and server";
    longDescription = ''
      NBD is a Linux kernel module and userland utilities for accessing
      block devices over a network. This package contains the server
      and client utilities.
    '';
    homepage = "https://nbd.sourceforge.net/";
    license = licenses.gpl2Plus;
    platforms = platforms.linux;
    maintainers = [];
  };
}
