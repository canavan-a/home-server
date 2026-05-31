{
    inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-26.05";

    outputs = { self, nixpkgs }: {
        packages.x86_64-linux = {
            server = nixpkgs.legacyPackages.x86_64-linux.callPackage ./server-package.nix {};
            client = nixpkgs.legacyPackages.x86_64-linux.callPackage ./client-package.nix {};
            streamer = nixpkgs.legacyPackages.x86_64-linux.callPackage ./streamer-package.nix {
                src = ./.;
            };
        };
    nixosModules.default = import ./module.nix self;
  };
}
