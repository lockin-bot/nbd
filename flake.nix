{
  description = "NBD - Network Block Device server and client";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        version = "3.26.1";
      in
      {
        packages = {
          default = self.packages.${system}.nbd;

          nbd = pkgs.stdenv.mkDerivation {
            pname = "nbd";
            inherit version;

            src = self;

            nativeBuildInputs = with pkgs; [
              autoreconfHook
              pkg-config
              flex
              bison
              autoconf-archive
            ];

            buildInputs = with pkgs; [
              glib
              gnutls
              libnl
            ];

            preConfigure = ''
              echo "${version}" > support/VERSION
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
            '';

            configureFlags = [
              "--enable-syslog"
              "--with-gnutls"
              "--with-libnl"
              "--disable-manpages"
            ];

            enableParallelBuilding = true;

            meta = with pkgs.lib; {
              description = "Network Block Device - client and server";
              homepage = "https://nbd.sourceforge.net/";
              license = licenses.gpl2Plus;
              platforms = platforms.linux;
            };
          };

          # Minimal build without TLS and netlink
          nbd-minimal = pkgs.stdenv.mkDerivation {
            pname = "nbd-minimal";
            inherit version;

            src = self;

            nativeBuildInputs = with pkgs; [
              autoreconfHook
              pkg-config
              flex
              bison
              autoconf-archive
            ];

            buildInputs = with pkgs; [
              glib
            ];

            preConfigure = ''
              echo "${version}" > support/VERSION
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
            '';

            configureFlags = [
              "--without-gnutls"
              "--without-libnl"
              "--disable-manpages"
            ];

            enableParallelBuilding = true;

            meta = with pkgs.lib; {
              description = "Network Block Device - minimal build";
              homepage = "https://nbd.sourceforge.net/";
              license = licenses.gpl2Plus;
              platforms = platforms.linux;
            };
          };

          # Static build for portable binaries
          nbd-static = pkgs.pkgsStatic.stdenv.mkDerivation {
            pname = "nbd-static";
            inherit version;

            src = self;

            nativeBuildInputs = with pkgs.pkgsStatic; [
              autoreconfHook
              pkg-config
              flex
              bison
              autoconf-archive
            ];

            buildInputs = with pkgs.pkgsStatic; [
              glib
            ];

            preConfigure = ''
              echo "${version}" > support/VERSION
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
            '';

            configureFlags = [
              "--without-gnutls"
              "--without-libnl"
              "--disable-manpages"
              "--disable-shared"
              "--enable-static"
            ];

            LDFLAGS = "-static";

            enableParallelBuilding = true;

            meta = with pkgs.lib; {
              description = "Network Block Device - static build";
              homepage = "https://nbd.sourceforge.net/";
              license = licenses.gpl2Plus;
              platforms = platforms.linux;
            };
          };
        };

        devShells.default = pkgs.mkShell {
          name = "nbd-dev";

          nativeBuildInputs = with pkgs; [
            autoconf
            automake
            autoreconfHook
            libtool
            pkg-config
            autoconf-archive
            flex
            bison
            docbook2x
            libxml2
            check
          ];

          buildInputs = with pkgs; [
            glib
            gnutls
            libnl
          ];

          shellHook = ''
            echo "NBD development environment"
            echo "Run: ./autogen.sh && ./configure && make"
          '';
        };
      }
    );
}
