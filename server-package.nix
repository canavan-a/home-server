{ lib, buildGoModule }:
buildGoModule {
    pname = "server";
    version = "0.1.0";
    src = ./server;
    vendorHash = null;
}