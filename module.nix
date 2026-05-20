{ self }: { config, lib, pkgs, ... }:
{
  options.services.home-server = {
    server.enable = lib.mkEnableOption "home-server server process";
    streamer.enable = lib.mkEnableOption "home-server streamer process";
    client.enable = lib.mkEnableOption "home-server client process";
    nginx.enable = lib.mkEnableOption "home-server nginx reverse proxy";
  };

  config = lib.mkMerge [
    (lib.mkIf config.services.home-server.server.enable {
      systemd.services.server = {
        wantedBy = [ "multi-user.target" ];
        after = [ "network.target" ];
        environment.WEBM_CLIPHOST = "/var/lib/streamer/clips";
        serviceConfig = {
          ExecStart = "${self.packages.x86_64-linux.server}/bin/server";
          # Secrets live in a root-only file on the host, not in the nix store.
          # Leading "-" makes it optional so a missing file won't fail the unit.
          EnvironmentFile = "-/var/lib/server/.env";
          Restart = "always";
        };
      };
    })

    (lib.mkIf config.services.home-server.streamer.enable {
      systemd.services.streamer = {
        wantedBy = [ "multi-user.target" ];
        after = [ "network.target" ];
        serviceConfig = {
          ExecStart = "${self.packages.x86_64-linux.streamer}/bin/streamer";
          Restart = "always";
        };
      };
    })

    (lib.mkIf config.services.home-server.client.enable {
      systemd.services.client = {
        wantedBy = [ "multi-user.target" ];
        after = [ "network.target" ];
        serviceConfig = {
          ExecStart = "${self.packages.x86_64-linux.client}/bin/client";
          Restart = "always";
        };
      };
    })

    (lib.mkIf config.services.home-server.nginx.enable {
      services.nginx = {
          enable = true;
          configFile = pkgs.writeText "nginx.conf" (builtins.readFile ./nginx.conf);
      };
    })
  ];
}