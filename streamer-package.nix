{ lib, stdenv, cmake, pkg-config, opencv, gst_all_1, openvino, onnxruntime, fetchgit }:

stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  src = fetchgit {
    url = "https://github.com/canavan-a/home-server";
    rev = "814df36870cb243beee146c23f1193ce552fbab7"; # fill in commit hash
    sha256 = "sha256-8hvAWAszkBR7z1w2rK+MOy/U5dqHou/33DSsLobsuF8="; # leave empty, nix will tell you
    fetchSubmodules = true;
  };

  sourceRoot = "better-video-pipeline";

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [
    opencv
    openvino
    onnxruntime
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp main $out/bin/streamer
  '';
}