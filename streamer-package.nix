{ lib, stdenv, cmake, pkg-config, opencv, gst_all_1, openvino, onnxruntime, fetchgit }:

stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  src = builtins.fetchGit {
    url = "https://github.com/canavan-a/home-server";
    ref = "main";
    rev = "43db89d5d59f7dc68eeac04427bbbad0f143f8d8";
    submodules = true;
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