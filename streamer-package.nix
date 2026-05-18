{ lib, stdenv, cmake, pkg-config, opencv, gst_all_1, openvino, onnxruntime, cpp-httplib, src }:

stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  inherit src;

  sourceRoot = "${src.name or "source"}/better-video-pipeline";

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [
    opencv
    openvino
    onnxruntime
    cpp-httplib
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp main $out/bin/streamer
  '';
}