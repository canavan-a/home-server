{ lib, stdenv, cmake, pkg-config, opencv, gst_all_1, openvino, onnxruntime, fetchgit }:

stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  src = fetchgit {
    url = "https://github.com/canavan-a/home-server";
    rev = "00ed1821fb086ef9911ea8eff13a76e06d1f5a91"; # fill in commit hash
    sha256 = ""; # leave empty, nix will tell you
    fetchSubmodules = true;
  };

  sourceRoot = "source/better-video-pipeline";

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