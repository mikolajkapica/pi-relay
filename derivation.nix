{ mkSbtDerivation, which, clang, hidapi, PATHS }:

let
  pname = "pi-relay";
  buildInputs = [ which clang hidapi ];

in mkSbtDerivation {
  inherit pname;
  version = "0.1.0";
  depsSha256 = "sha256-itdHNSinupu+5L6Z/qvwuLeXLRwfVmUBLFng5vGpxAI=";
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
