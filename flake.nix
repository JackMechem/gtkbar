{
  description = "My bar that I wrote in c for some reason.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }: 
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        gtkbar = pkgs.stdenv.mkDerivation {
          pname = "gtkbar";
          version = "0.0.1";
          src = ./.;

          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.meson
            pkgs.ninja
            pkgs.makeWrapper
          ];

          buildInputs = [
            pkgs.gtk4
            pkgs.gtk4-layer-shell
            pkgs.cjson
            pkgs.networkmanager.dev
            pkgs.libpulseaudio
          ];
          postInstall = ''
              wrapProgram "$out/bin/gtkbar" \
                --prefix XDG_DATA_DIRS : "$out/share"
        '';
        };
      in {
        packages.default = gtkbar;

        devShell = pkgs.mkShell {
          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.meson
            pkgs.ninja
            pkgs.makeWrapper
          ];
          buildInputs = [
            pkgs.gtk4
            pkgs.gtk4-layer-shell
            pkgs.cjson
            pkgs.networkmanager.dev
            pkgs.libpulseaudio
          ];
        };
      });
}

