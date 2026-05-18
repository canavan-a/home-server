{ self }: { config, lib, pkgs, ... }:
{
  options.services.home-server = {
    enable = lib.mkEnableOption "home-server";
  };

  config = lib.mkIf config.services.home-server.enable {
    systemd.services.server = {
      wantedBy = [ "multi-user.target" ];
      after = [ "network.target" ];
      serviceConfig = {
        ExecStart = "${self.packages.x86_64-linux.server}/bin/server";
        Restart = "always";
      };
    };

    systemd.services.streamer = {
      wantedBy = [ "multi-user.target" ];
      after = [ "network.target" ];
      serviceConfig = {
        ExecStart = "${self.packages.x86_64-linux.streamer}/bin/streamer";
        Restart = "always";
      };
    };

    systemd.services.client = {
      wantedBy = [ "multi-user.target" ];
      after = [ "network.target" ];
      serviceConfig = {
        ExecStart = "${self.packages.x86_64-linux.client}/bin/client";
        Restart = "always";
      };
    };
  };
}