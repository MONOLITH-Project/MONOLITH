{
  description = "Development environment for MONOLITH";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          gcc
          gnumake
          bison
          flex
          gmp.dev
          libmpc
          mpfr.dev
          texinfo
          isl
          qemu
          nasm
          grub2
          xorriso
          gnupg
        ];
      };
    };
}
