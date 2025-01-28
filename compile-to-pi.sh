#!/bin/bash
target_path=$(nix build --builders "ssh://eu.nixbuild.net x86_64-linux - 100 1 big-parallel,benchmark")
nix copy .#packages.aarch64-linux.default --to ssh://pi@192.168.1.65
echo "$target_path/bin/pi-relay"
