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
dfu_vidpid="0483:df11"
wait_seconds="${DFU_WAIT_SECONDS:-20}"
retry_interval="${DFU_RETRY_INTERVAL:-1}"
verbose="${DFU_VERBOSE:-0}"

cd "${workspace_root}"

attempt=0
max_attempts=$(( (wait_seconds + retry_interval - 1) / retry_interval ))

echo "Waiting for DFU device ${dfu_vidpid} (timeout=${wait_seconds}s, interval=${retry_interval}s)..."

while true; do
  dfu_list_output="$(docker compose -f docker-compose.yml run --rm stm32-dfu dfu-util -l 2>&1 || true)"

  if printf "%s\n" "${dfu_list_output}" | grep -qi "${dfu_vidpid}"; then
    echo "DFU device detected, starting flash..."
    break
  fi

  if [[ "${verbose}" == "1" ]]; then
    echo "Attempt $((attempt + 1))/${max_attempts}: DFU device ${dfu_vidpid} not found yet."
    printf "%s\n" "${dfu_list_output}"
  fi

  attempt=$((attempt + 1))
  if [[ ${attempt} -ge ${max_attempts} ]]; then
    echo "Error: DFU device ${dfu_vidpid} not detected within ${wait_seconds}s."
    echo "Ensure BOOT0=1, reset the board, and connect via USB data cable."
    exit 74
  fi

  sleep "${retry_interval}"
done

docker compose -f docker-compose.yml run --rm stm32-dfu \
  dfu-util -a 0 -s 0x08000000:leave -D "${firmware_path}"
