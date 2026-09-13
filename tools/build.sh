#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_root="${1:-$HOME/caffeine-for-vita-build}"
plugin_build="$build_root/plugin"
installer_build="$build_root/installer"
output_dir="$project_dir/out"

cmake -S "$project_dir" -B "$plugin_build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$plugin_build" --parallel

cmake -S "$project_dir/installer" -B "$installer_build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCAFFEINE_SUPRX="$plugin_build/caffeine.suprx"
cmake --build "$installer_build" --parallel

mkdir -p "$output_dir"
cp "$installer_build/CaffeineForVita.vpk" "$output_dir/CaffeineForVita.vpk"
sha256sum "$output_dir/CaffeineForVita.vpk"
