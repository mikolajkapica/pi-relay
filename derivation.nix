{ mkSbtDerivation, which, clang, hidapi, openssl, PATHS }:

let
  pname = "pi-relay";
  buildInputs = [ which clang hidapi openssl ];

in mkSbtDerivation {
  inherit pname;
  version = "0.1.0";
  depsSha256 = null;
  inherit buildInputs;

  depsWarmupCommand = ''
    sbt app/compile
  '';
  overrideDepsAttrs = final: prev: {
    inherit buildInputs;
    inherit (PATHS) BINDGEN_PATH HIDAPI_PATH;
  };
  inherit (PATHS) BINDGEN_PATH HIDAPI_PATH;

  src = ./.;

  buildPhase = ''
    sbt app/nativeLink
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp app/target/scala-3.6.3/app-out $out/bin/$pname
  '';
}
