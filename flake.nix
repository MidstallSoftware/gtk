{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    systems.url = "github:nix-systems/default-linux";
    flake-utils.url = "github:numtide/flake-utils";
    nixos-apple-silicon = {
      url = "github:tpwrules/nixos-apple-silicon/release-2024-11-12";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      systems,
      flake-utils,
      nixos-apple-silicon,
      ...
    }:
    let
      inherit (nixpkgs) lib;
    in
    flake-utils.lib.eachSystem (import systems) (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system}.extend (
          pkgs: prev: with pkgs; {
            gtk4-drm = prev.gtk4.overrideAttrs (
              f: p: {
                buildInputs = p.buildInputs ++ [
                  seatd
                  udev
                  libdisplay-info
                  mesa
                ];

                postFixup =
                  lib.optionalString (!stdenv.hostPlatform.isDarwin) ''
                    demos=(gtk4-demo gtk4-demo-application gtk4-widget-factory)

                    for program in ''${demos[@]}; do
                      wrapProgram $dev/bin/$program \
                        --prefix XDG_DATA_DIRS : "$GSETTINGS_SCHEMAS_PATH:$out/share/gsettings-schemas/${f.pname}-${f.version}"
                    done
                  ''
                  + lib.optionalString (stdenv.hostPlatform.isLinux) ''
                    # Cannot be in postInstall, otherwise _multioutDocs hook in preFixup will move right back.
                    moveToOutput "share/doc" "$devdoc"
                  '';

                src = ./.;
              }
            );

            pkgsAsahi = (
              if stdenv.hostPlatform.isAarch64 then
                pkgs.appendOverlays [
                  nixos-apple-silicon.overlays.default
                  (pkgsAsahi: prev: {
                    mesa-asahi-edge = prev.mesa-asahi-edge.overrideAttrs (
                      super: p: {
                        meta = p.meta // {
                          inherit (prev.mesa.meta) platforms;
                        };
                      }
                    );

                    mesa = pkgsAsahi.mesa-asahi-edge;
                  })
                ]
              else
                null
            );
          }
        );
      in
      {
        packages.default = pkgs.gtk4-drm;

        legacyPackages = pkgs;

        nixosConfigurations = lib.nixosSystem {
          inherit system pkgs;

          modules = [
            "${nixpkgs}/nixos/modules/virtualisation/qemu-vm.nix"
            "${nixpkgs}/nixos/modules/profiles/graphical.nix"
            (
              { pkgs, ... }:
              {
                environment.systemPackages = with pkgs; [ pkgs.gtk4-drm pkgs.gtk4-drm.dev ];

                users.users.nixos = {
                  isNormalUser = true;
                  extraGroups = [ "wheel" "networkmanager" "video" ];
                  initialHashedPassword = "";
                };
              }
            )
          ];
        };

        devShells =
          let
            mkShell = pkgs: pkgs.gtk4-drm;
          in
          {
            default = mkShell pkgs;
          }
          // lib.optionalAttrs (pkgs.pkgsAsahi != null) { asahi = mkShell pkgs.pkgsAsahi; };
      }
    );
}
