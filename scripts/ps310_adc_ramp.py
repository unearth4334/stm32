"""PS310 ramp-and-capture utility.

Usage:
    .venv/bin/python scripts/ps310_adc_ramp.py \
        --stm32-port COM18 \
        --ps310-address GPIB0::14::INSTR \
        --start-voltage -100 \
        --step-size -50 \
        --end-voltage -300 \
        --settling-time 5

Dependencies:
    pip install pyserial
    pip install 'lab-drivers @ git+https://github.com/unearth4334/lab-drivers.git'
    Install a system VISA runtime such as NI-VISA or Keysight IO Libraries.

The script steps the Stanford PS310 through the requested ramp, waits for the
specified settling time at each setpoint, records PS310 telemetry, and captures
the next fresh STM32 ADS7822 console sample into a CSV row.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


ADC_SAMPLE_PATTERN = re.compile(r"^\[(?P<tick>\d+)\]\s+[A-Z]/ads7822:\s+sample=(?P<sample>\d+)$")
ADC_READ_FAIL_PATTERN = re.compile(r"^\[(?P<tick>\d+)\]\s+[A-Z]/ads7822:\s+read failed$")


@dataclass
class AdcSample:
    host_timestamp_utc: str
    tick_ms: int
    raw_sample: int
    hv_vmon_m_v: float
    hv_filt_v: float
    console_line: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Ramp a Stanford PS310 through a voltage pattern and record PS310 telemetry "
            "together with STM32 ADS7822-derived monitor readings."
        )
    )
    parser.add_argument("--ps310-address", help="VISA resource string for the PS310. If omitted, auto-detect is used.")
    parser.add_argument(
        "--visa-library",
        help="Optional explicit VISA library path, for example C:/Windows/System32/nivisa64.dll.",
    )
    parser.add_argument("--stm32-port", required=True, help="Serial port for the STM32 console, for example COM18.")
    parser.add_argument("--baud-rate", type=int, default=115200, help="STM32 console baud rate. Default: 115200.")
    parser.add_argument("--start-voltage", type=float, required=True, help="Starting PS310 voltage in volts.")
    parser.add_argument("--step-size", type=float, required=True, help="Step size in volts. Use a negative step for a more negative ramp.")
    parser.add_argument("--end-voltage", type=float, required=True, help="Final PS310 voltage in volts.")
    parser.add_argument("--settling-time", type=float, required=True, help="Delay after each step before measurements are captured, in seconds.")
    parser.add_argument(
        "--adc-sample-timeout",
        type=float,
        default=12.0,
        help="Maximum time to wait for a fresh STM32 ADC sample after settling, in seconds. Default: 12.",
    )
    parser.add_argument("--vref", type=float, default=3.0, help="ADS7822 reference voltage in volts. Default: 3.0.")
    parser.add_argument(
        "--hv-filt-top-ohms",
        type=float,
        default=30_000_000.0,
        help="Top resistor value for the HV_FILT divider in ohms. Default: 30 Mohm.",
    )
    parser.add_argument(
        "--hv-filt-bottom-ohms",
        type=float,
        default=150_000.0,
        help="Bottom resistor value for the HV_FILT divider in ohms. Default: 150 kohm.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help=(
            "CSV output path. Default: artifacts/measurements/ps310_adc_ramp_<UTC timestamp>.csv"
        ),
    )
    parser.add_argument(
        "--leave-output-on",
        action="store_true",
        help="Do not disable the PS310 output at the end of the ramp. Default behavior is to turn it off.",
    )
    parser.add_argument(
        "--echo-console",
        action="store_true",
        help="Print STM32 console lines as they are received while waiting for samples.",
    )
    return parser.parse_args()


def default_output_path() -> Path:
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    return Path("artifacts") / "measurements" / f"ps310_adc_ramp_{timestamp}.csv"


def load_serial_module():
    try:
        import serial  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency 'pyserial'. Install it in the workspace venv before running this script."
        ) from exc

    return serial


def load_ps310_driver():
    import importlib
    import inspect

    try:
        visa_pkg = importlib.import_module("lab_drivers.drivers.visa")
        candidate = getattr(visa_pkg, "StanfordPS310", None)
        if inspect.isclass(candidate):
            return candidate

        ps310_module = importlib.import_module("lab_drivers.drivers.visa.StanfordPS310")
        candidate = getattr(ps310_module, "StanfordPS310", None)
        if inspect.isclass(candidate):
            return candidate
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency 'lab_drivers' with VISA support. Install lab-drivers with the visa extra before running this script."
        ) from exc

    raise SystemExit(
        "Could not resolve the StanfordPS310 class from lab_drivers. Check the installed lab_drivers version."
    )


def resolve_visa_library(user_value: str | None) -> str | None:
    if user_value:
        return user_value

    if os.name != "nt":
        return None

    try:
        import winreg

        with winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r"SOFTWARE\VXIPNP_Alliance\VISA_Installs\CurrentVersion",
        ) as root_key:
            subkey_count, _, _ = winreg.QueryInfoKey(root_key)
            for index in range(subkey_count):
                subkey_name = winreg.EnumKey(root_key, index)
                with winreg.OpenKey(root_key, subkey_name) as install_key:
                    location, _ = winreg.QueryValueEx(install_key, "Location")
                    if location and Path(location).exists():
                        return str(Path(location))
    except OSError:
        pass

    fallback_paths = [
        Path(r"C:\Windows\System32\nivisa64.dll"),
        Path(r"C:\Windows\System32\visa64.dll"),
    ]
    for candidate in fallback_paths:
        if candidate.exists():
            return str(candidate)

    return None


def configure_pyvisa_runtime(visa_library: str | None) -> None:
    if not visa_library:
        return

    import pyvisa

    original_resource_manager = pyvisa.ResourceManager

    def resource_manager_wrapper(*args, **kwargs):
        if not args and "visa_library" not in kwargs:
            return original_resource_manager(visa_library)
        return original_resource_manager(*args, **kwargs)

    pyvisa.ResourceManager = resource_manager_wrapper


def validate_args(args: argparse.Namespace) -> None:
    if args.step_size == 0.0:
        raise SystemExit("--step-size must be non-zero.")
    if args.settling_time < 0.0:
        raise SystemExit("--settling-time must be non-negative.")
    if args.adc_sample_timeout <= 0.0:
        raise SystemExit("--adc-sample-timeout must be positive.")
    if args.vref <= 0.0:
        raise SystemExit("--vref must be positive.")
    if args.hv_filt_top_ohms < 0.0 or args.hv_filt_bottom_ohms <= 0.0:
        raise SystemExit("Divider resistor values must be valid positive values.")

    increasing = args.end_voltage > args.start_voltage
    if increasing and args.step_size < 0.0:
        raise SystemExit("Step size sign does not move from start voltage toward end voltage.")
    if (not increasing) and args.end_voltage != args.start_voltage and args.step_size > 0.0:
        raise SystemExit("Step size sign does not move from start voltage toward end voltage.")


def build_ramp(start: float, end: float, step: float) -> list[float]:
    if start == end:
        return [start]

    values: list[float] = []
    epsilon = abs(step) * 1e-9 + 1e-12
    current = start

    if step > 0.0:
        while current <= end + epsilon:
            values.append(current)
            current += step
    else:
        while current >= end - epsilon:
            values.append(current)
            current += step

    if not values:
        raise SystemExit("Ramp generation produced no points. Check start/end/step values.")

    return values


def open_serial_port(serial_module, port: str, baud_rate: int):
    try:
        return serial_module.Serial(port=port, baudrate=baud_rate, timeout=0.25)
    except Exception as exc:
        raise SystemExit(f"Unable to open STM32 serial port {port}: {exc}") from exc


def flush_serial_input(serial_port) -> None:
    try:
        serial_port.reset_input_buffer()
    except Exception:
        pass


def read_next_adc_sample(
    serial_port,
    sample_timeout_s: float,
    vref: float,
    hv_filt_gain: float,
    echo_console: bool,
) -> tuple[AdcSample | None, str]:
    deadline = time.monotonic() + sample_timeout_s
    last_status = "timeout"

    while time.monotonic() < deadline:
        raw_line = serial_port.readline()
        if not raw_line:
            continue

        try:
            line = raw_line.decode("ascii", errors="replace").strip()
        except Exception:
            continue

        if not line:
            continue

        if echo_console:
            print(f"STM32 {line}")

        match = ADC_SAMPLE_PATTERN.match(line)
        if match:
            tick_ms = int(match.group("tick"))
            raw_sample = int(match.group("sample"))
            hv_vmon_m_v = (raw_sample / 4095.0) * vref
            hv_filt_v = hv_vmon_m_v * hv_filt_gain
            sample = AdcSample(
                host_timestamp_utc=datetime.now(timezone.utc).isoformat(),
                tick_ms=tick_ms,
                raw_sample=raw_sample,
                hv_vmon_m_v=hv_vmon_m_v,
                hv_filt_v=hv_filt_v,
                console_line=line,
            )
            return sample, "ok"

        if ADC_READ_FAIL_PATTERN.match(line):
            last_status = "adc_read_failed"

    return None, last_status


def capture_ps310_telemetry(hvps) -> dict[str, object]:
    measured_voltage_filtered_v = float(hvps.measure_voltage(apply_filter=True))
    measured_voltage_raw_v = float(hvps.measure_voltage(apply_filter=False))
    measured_current_a = float(hvps.measure_current())
    set_voltage_v = float(hvps.get_voltage())
    current_limit_a = float(hvps.get_current_limit())
    voltage_limit_v = float(hvps.get_voltage_limit())
    output_enabled = bool(hvps.get_output_state())

    return {
        "ps310_set_voltage_v": set_voltage_v,
        "ps310_measured_voltage_filtered_v": measured_voltage_filtered_v,
        "ps310_measured_voltage_raw_v": measured_voltage_raw_v,
        "ps310_measured_current_a": measured_current_a,
        "ps310_current_limit_a": current_limit_a,
        "ps310_voltage_limit_v": voltage_limit_v,
        "ps310_output_enabled": output_enabled,
    }


def write_rows(csv_path: Path, rows: Iterable[dict[str, object]]) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "host_timestamp_utc",
        "step_index",
        "target_voltage_v",
        "settling_time_s",
        "ps310_address",
        "ps310_identification",
        "ps310_set_voltage_v",
        "ps310_measured_voltage_filtered_v",
        "ps310_measured_voltage_raw_v",
        "ps310_measured_current_a",
        "ps310_current_limit_a",
        "ps310_voltage_limit_v",
        "ps310_output_enabled",
        "stm32_status",
        "stm32_tick_ms",
        "stm32_raw_sample",
        "stm32_hv_vmon_m_v",
        "stm32_hv_filt_v",
        "stm32_console_line",
    ]

    with csv_path.open("w", newline="", encoding="ascii") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def format_ps310_runtime_error(exc: Exception) -> str:
    message = str(exc)

    if "VI_ERROR_LIBRARY_NFOUND" in message:
        return (
            "PyVISA could not find a system VISA runtime. Install a 64-bit VISA backend "
            "such as NI-VISA or Keysight IO Libraries, then restart the shell and retry."
        )

    return f"Unable to initialize the PS310 VISA driver: {message}"


def validate_ps310_resource(visa_library: str | None, resource_name: str | None) -> None:
    if not resource_name or not resource_name.upper().startswith("GPIB"):
        return

    import pyvisa

    manager = pyvisa.ResourceManager(visa_library) if visa_library else pyvisa.ResourceManager()

    try:
        gpib_resources = manager.list_resources("GPIB?*::INSTR")
    except Exception as exc:
        raise SystemExit(
            "VISA opened, but GPIB resource enumeration failed. This usually means the NI GPIB provider "
            "is not fully available to VISA. Check that the NI GPIB-USB-HS is visible in NI tools and that "
            "the NI GPIB Hardware Enumeration Service is running. Original error: "
            f"{exc}"
        ) from exc

    if resource_name in gpib_resources:
        return

    raise SystemExit(
        "The requested PS310 resource was not found in VISA. Visible GPIB resources: "
        f"{gpib_resources if gpib_resources else 'none'}. Windows can see the NI GPIB adapter, but VISA is not "
        "currently publishing GPIB instruments. Check the NI GPIB-USB-HS in NI MAX/GPIB Configuration Utility, "
        "verify the NI GPIB Hardware Enumeration Service is running, and confirm the PS310 is powered and set to "
        f"address {resource_name}."
    )


def main() -> int:
    args = parse_args()
    validate_args(args)

    csv_path = args.output or default_output_path()
    hv_filt_gain = (args.hv_filt_top_ohms + args.hv_filt_bottom_ohms) / args.hv_filt_bottom_ohms
    ramp_points = build_ramp(args.start_voltage, args.end_voltage, args.step_size)
    visa_library = resolve_visa_library(args.visa_library)

    if visa_library:
        os.environ["PYVISA_LIBRARY"] = visa_library

    configure_pyvisa_runtime(visa_library)
    validate_ps310_resource(visa_library, args.ps310_address)

    serial_module = load_serial_module()
    ps310_cls = load_ps310_driver()

    serial_port = open_serial_port(serial_module, args.stm32_port, args.baud_rate)
    try:
        hvps = ps310_cls(auto_connect=False)
    except Exception as exc:
        serial_port.close()
        raise SystemExit(format_ps310_runtime_error(exc)) from exc
    recorded_rows: list[dict[str, object]] = []
    ps310_idn = ""
    ps310_address = args.ps310_address or "auto"
    output_was_enabled = False

    try:
        try:
            hvps.connect(args.ps310_address)
        except Exception as exc:
            raise SystemExit(f"Unable to connect to the PS310 at {ps310_address}: {exc}") from exc
        ps310_idn = str(hvps.get_identification())
        if hasattr(hvps, "clear_status"):
            hvps.clear_status()

        print(f"Connected to PS310: {ps310_idn}")
        print(f"STM32 port: {args.stm32_port} @ {args.baud_rate}")
        if visa_library:
            print(f"VISA library: {visa_library}")
        print(f"Ramp points: {len(ramp_points)}")
        print(f"CSV output: {csv_path}")

        for index, target_voltage in enumerate(ramp_points, start=1):
            print(f"[{index}/{len(ramp_points)}] Setting PS310 to {target_voltage:.3f} V")
            hvps.set_voltage(target_voltage)
            if not output_was_enabled:
                hvps.set_output_state(True)
                output_was_enabled = True

            flush_serial_input(serial_port)

            if args.settling_time > 0.0:
                time.sleep(args.settling_time)

            ps310_values = capture_ps310_telemetry(hvps)
            adc_sample, stm32_status = read_next_adc_sample(
                serial_port=serial_port,
                sample_timeout_s=args.adc_sample_timeout,
                vref=args.vref,
                hv_filt_gain=hv_filt_gain,
                echo_console=args.echo_console,
            )

            row = {
                "host_timestamp_utc": datetime.now(timezone.utc).isoformat(),
                "step_index": index,
                "target_voltage_v": target_voltage,
                "settling_time_s": args.settling_time,
                "ps310_address": ps310_address,
                "ps310_identification": ps310_idn,
                "ps310_set_voltage_v": ps310_values["ps310_set_voltage_v"],
                "ps310_measured_voltage_filtered_v": ps310_values["ps310_measured_voltage_filtered_v"],
                "ps310_measured_voltage_raw_v": ps310_values["ps310_measured_voltage_raw_v"],
                "ps310_measured_current_a": ps310_values["ps310_measured_current_a"],
                "ps310_current_limit_a": ps310_values["ps310_current_limit_a"],
                "ps310_voltage_limit_v": ps310_values["ps310_voltage_limit_v"],
                "ps310_output_enabled": ps310_values["ps310_output_enabled"],
                "stm32_status": stm32_status,
                "stm32_tick_ms": "",
                "stm32_raw_sample": "",
                "stm32_hv_vmon_m_v": "",
                "stm32_hv_filt_v": "",
                "stm32_console_line": "",
            }

            if adc_sample is not None:
                row.update(
                    {
                        "stm32_tick_ms": adc_sample.tick_ms,
                        "stm32_raw_sample": adc_sample.raw_sample,
                        "stm32_hv_vmon_m_v": adc_sample.hv_vmon_m_v,
                        "stm32_hv_filt_v": adc_sample.hv_filt_v,
                        "stm32_console_line": adc_sample.console_line,
                    }
                )

            recorded_rows.append(row)

            print(
                "  PS310 measured={0:.3f} V filtered, {1:.6f} A; STM32 status={2}{3}".format(
                    row["ps310_measured_voltage_filtered_v"],
                    row["ps310_measured_current_a"],
                    stm32_status,
                    "" if adc_sample is None else f", raw={adc_sample.raw_sample}",
                )
            )

        write_rows(csv_path, recorded_rows)
        print(f"Wrote {len(recorded_rows)} ramp samples to {csv_path}")
        return 0
    finally:
        try:
            if output_was_enabled and not args.leave_output_on:
                hvps.set_output_state(False)
        except Exception:
            pass

        try:
            hvps.disconnect()
        except Exception:
            pass

        try:
            serial_port.close()
        except Exception:
            pass


if __name__ == "__main__":
    sys.exit(main())