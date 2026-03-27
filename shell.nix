{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "nbd-dev-shell";

  nativeBuildInputs = with pkgs; [
    # Build tools
    autoconf
    automake
    autoreconfHook
    libtool
    pkg-config
    autoconf-archive

    # Parser/lexer generators
    flex
    bison

    # Documentation
    docbook2x
    libxml2

    # For testing
    check
  ];

  buildInputs = with pkgs; [
    # Required libraries
    glib
    gnutls
    libnl
  ];

  shellHook = ''
    echo "NBD development environment loaded"
    echo "Run ./autogen.sh && ./configure && make to build"
  '';
}
