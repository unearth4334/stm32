#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <project_dir> <pio_env>"
  exit 2
fi

project_dir="$1"
pio_env="$2"
workspace_root="$(cd "${project_dir}/.." && pwd)"
firmware_path="/work/$(basename "${project_dir}")/.pio/build/${pio_env}/firmware.bin"

cd "${workspace_root}"

docker compose -f docker-compose.yml run --rm stm32-dfu \
  dfu-util -a 0 -s 0x08000000:leave -D "${firmware_path}"
