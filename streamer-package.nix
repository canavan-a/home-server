{ lib, stdenv, cmake, pkg-config, makeWrapper, opencv, gst_all_1, openvino, onnxruntime, httplib, openssl, src }:

let
  gstPlugins = [
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
    gst_all_1.gst-plugins-good
    gst_all_1.gst-plugins-bad
  ];
  gstPluginPath = lib.makeSearchPath "lib/gstreamer-1.0" gstPlugins;
in
stdenv.mkDerivation {
  pname = "streamer";
  version = "0.1.0";

  inherit src;

  setSourceRoot = "sourceRoot=$(echo */better-video-pipeline)";

  nativeBuildInputs = [ cmake pkg-config makeWrapper ];
  buildInputs = [
    opencv
    openvino
    onnxruntime
    httplib
    openssl
  ] ++ gstPlugins;

  installPhase = ''
    mkdir -p $out/bin $out/libexec $out/share/streamer
    cp main $out/libexec/streamer-unwrapped
    cp -r ../models $out/share/streamer/

    cat > $out/bin/streamer <<EOF
    #!/usr/bin/env bash
    export GST_PLUGIN_SYSTEM_PATH_1_0="${gstPluginPath}"
    DATA_DIR="/var/lib/streamer"
    mkdir -p "\$DATA_DIR"
    ln -sfn "$out/share/streamer/models" "\$DATA_DIR/models"
    exec "$out/libexec/streamer-unwrapped" "\$@"
    EOF
    chmod +x $out/bin/streamer
  '';
}