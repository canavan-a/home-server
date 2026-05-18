{ lib, stdenv, cmake, pkg-config, opencv, gst_all_1, openvino, onnxruntime, fetchgit }:

stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  src = fetchgit {
    url = "https://github.com/canavan-a/home-server";
    rev = "e204459dbf1a74feaecce8a07631071012df5400"; # fill in commit hash
    sha256 = "sha256-PpnVQ+F1p/hPLr2NfJy5+57N0A2oThH0ZGhk/UqToVg="; # leave empty, nix will tell you
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