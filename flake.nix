{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable-small";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, utils, ... }: utils.lib.eachDefaultSystem (system:
    let
      pkgs = (import nixpkgs { inherit system; });
    in
    {
      devShells.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          bison
          clang-tools # for clang-format
          cmake
          flex
          gcc
        ];
      };
    });
}

