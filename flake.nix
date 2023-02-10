{
  description = "A very basic flake";
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { self
    , nixpkgs
    , flake-utils
    ,
    }:
    let
      overlay =
        final: prev: {
          dwm = prev.dwm.overrideAttrs (oldAttrs: rec {
            postPatch = (oldAttrs.postPatch or "") + ''
              cp -r DEF/* .
            '';
            version = "master";
            src = ./.;
          });
        };
    in
    flake-utils.lib.eachDefaultSystem
      (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [
              self.overlays.default
            ];
          };
        in
        rec {
          # apps = {
          #   dwm = {
          #     type = "app";
          #     program = "${packages.default}/bin/st";
          #   };
          # };
          packages.dwm = pkgs.dwm;
          # apps.default = apps.dwm;
          packages.default = pkgs.dwm;
          devShells.default = pkgs.mkShell {
            buildInputs = with pkgs; [ xorg.libX11 xorg.libXft xorg.libXinerama gcc ];
          };
        }
      )
    // { overlays.default = overlay; };
}
