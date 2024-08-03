{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  packages = with pkgs; [ 
    python312Full
    openssl
    xmake
    cmake
    git
    busybox
    pkg-config
    p7zip
    neovim
    libGL
    xorg.libxcb
    xorg.libX11
    xorg.libXfixes
    xorg.libXrender
    xorg.libXcursor
    xorg.libXext
    xorg.libXrandr
  ];
  inputFrom = with pkgs; [
  ];
  shellHook = ''
    # export DEBUG=1
  '';
}

# with import <nixpkgs> {};
# pkgs.stdenv.mkShell {
#   name = "opengl-env";
#   # nativeBuildInputs is usually what you want -- tools you need to run
#   nativeBuildInputs = with pkgs; [
#     pkg-config
#     cmake
#     # clang_multi
#   ];
#   buildInputs = with pkgs;[
#     # Programs
#     p7zip git toybox xmake
#     libGL
#     libGLU
#     openal
#     flac
#     xorg.libxcb
#   ];
#   #library_path = pkgs.lib.makeLibraryPath [ stdenv ];
#   shellHook = ''
#     # export LD=$LD
#     # export CC=$CC
#   '';
# }
