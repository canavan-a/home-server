{ lib, buildNpmPackage, nodejs }:
buildNpmPackage {
    pname = "client";
    version = "0.1.0";
    src = ./client;
    npmDepsHash = null;

    buildPhase = ''
        npm run build;
    '';

    installPhase = ''
        mkdir -p $out/bin
        cp -r dist $out/dist
        cp package.json $out/
        cat > $out/bin/client <<EOF
    #!/bin/sh
    cd $out
    exec ${nodejs}/bin/npm run preview -- --host 0.0.0.0
    EOF
        chmod +x $out/bin/client
    '';
}