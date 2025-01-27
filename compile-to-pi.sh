#!/bin/bash
target_path=$(nix build .#packages.aarch64-linux.default --no-link --print-out-paths -L --option sandbox false)
nix copy .#packages.aarch64-linux.default --to ssh://pi@192.168.1.65
echo "$target_path/bin/pi-relay"
