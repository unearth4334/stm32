"""DP711 HV sweep + STM32 ADS7822 capture application test.

This test drives a Rigol DP711 bench supply to command a high-voltage (HV)
supply through a bipolar-friendly sweep, reads the resulting voltage back from
the STM32 BlackPill (ADS7822 monitor), and plots the expected vs. measured HV
along with the per-step error.

Signal chain:
    DP711 setpoint (0..+8 V)
        --> HV supply (ratio 1:-125, so +8 V -> -1000 V)
            --> ADC monitor signal (default transfer: HV = -200 * ADC_V)
                --> ADS7822 --> STM32 console "sample=<raw>"

Sweep:
    HV is stepped from 0 V down to -1000 V and back to 0 V in configurable
    steps (default 50 V). Each HV target is converted to a DP711 setpoint via
    ``dp711_setpoint = hv_target / ratio`` (ratio defaults to -125).

Usage:
    .venv/bin/python scripts/dp711_hv_sweep.py \
        --dp711-port COM4 \
        --stm32-port COM18 \
        --step-size 50 \
        --delay 2

Dependencies (see scripts/requirements-dp711_hv_sweep.txt):
    pip install -r scripts/requirements-dp711_hv_sweep.txt
"""

from __future__ import annotations

import argparse
import csv
import math
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


@dataclass
class SweepPoint:
    step_index: int
    hv_target_v: float
    dp711_setpoint_v: float
    expected_hv_v: float
    measured_hv_v: float | None
    error_v: float | None
    stm32_status: str
    dp711_measured_voltage_v: float
    dp711_measured_current_a: float
    adc_sample: AdcSample | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Sweep HV via a Rigol DP711 and capture STM32 ADS7822 monitor readings, "
            "then plot expected vs. measured HV with a per-step error subplot."
        )
    )
    parser.add_argument("--dp711-port", help="Serial port for the DP711, for example COM4. If omitted, driver auto-detect is used.")
    parser.add_argument("--dp711-baud", type=int, default=9600, help="DP711 serial baud rate. Default: 9600.")
    parser.add_argument("--stm32-port", required=True, help="Serial port for the STM32 console, for example COM18.")
    parser.add_argument("--stm32-baud", type=int, default=115200, help="STM32 console baud rate. Default: 115200.")

    parser.add_argument("--hv-start", type=float, default=0.0, help="Sweep start HV in volts. Default: 0.")
    parser.add_argument("--hv-peak", type=float, default=-1000.0, help="Sweep peak (far end) HV in volts. Default: -1000.")
    parser.add_argument("--step-size", type=float, default=50.0, help="HV step magnitude in volts. Default: 50.")
    parser.add_argument(
        "--ratio",
        type=float,
        default=-125.0,
        help="HV-per-DP711-volt conversion ratio. Default: -125 (so +8 V -> -1000 V).",
    )
    parser.add_argument(
        "--hv-per-adc-volt",
        type=float,
        default=-200.0,
        help=(
            "Transfer from ADC voltage to HV in V/V. Default: -200, so "
            "measured_hv = adc_voltage * (-200)."
        ),
    )

    parser.add_argument("--delay", type=float, default=2.0, help="Delay after each hardware interaction, in seconds. Default: 2.")
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

    parser.add_argument("--output", type=Path, help="CSV output path. Default: artifacts/measurements/dp711_hv_sweep_<UTC>.csv")
    parser.add_argument("--plot-output", type=Path, help="Plot PNG path. Default: alongside the CSV with a .png suffix.")
    parser.add_argument("--no-show", action="store_true", help="Save the plot without opening an interactive window.")
    parser.add_argument("--no-plot", action="store_true", help="Skip plotting entirely (CSV only).")
    parser.add_argument("--leave-output-on", action="store_true", help="Do not disable the DP711 output at the end of the sweep.")
    parser.add_argument("--echo-console", action="store_true", help="Print STM32 console lines while waiting for samples.")
    return parser.parse_args()


def default_output_path() -> Path:
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    return Path("artifacts") / "measurements" / f"dp711_hv_sweep_{timestamp}.csv"


def validate_args(args: argparse.Namespace) -> None:
    if args.step_size <= 0.0:
        raise SystemExit("--step-size must be a positive magnitude.")
    if args.ratio == 0.0:
        raise SystemExit("--ratio must be non-zero.")
    if args.hv_per_adc_volt == 0.0:
        raise SystemExit("--hv-per-adc-volt must be non-zero.")
    if args.delay < 0.0:
        raise SystemExit("--delay must be non-negative.")
    if args.adc_sample_timeout <= 0.0:
        raise SystemExit("--adc-sample-timeout must be positive.")
    if args.vref <= 0.0:
        raise SystemExit("--vref must be positive.")
    if args.hv_filt_top_ohms < 0.0 or args.hv_filt_bottom_ohms <= 0.0:
        raise SystemExit("Divider resistor values must be valid positive values.")


def load_serial_module():
    try:
        import serial  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency 'pyserial'. Install scripts/requirements-dp711_hv_sweep.txt first."
        ) from exc

    return serial


def load_dp711_driver():
    import importlib
    import inspect

    try:
        serial_pkg = importlib.import_module("lab_drivers.drivers.serial")
        candidate = getattr(serial_pkg, "RigolDP711", None)
        if inspect.isclass(candidate):
            return candidate

        dp711_module = importlib.import_module("lab_drivers.drivers.serial.RigolDP711")
        candidate = getattr(dp711_module, "RigolDP711", None)
        if inspect.isclass(candidate):
            return candidate
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency 'lab_drivers' with the RigolDP711 driver. Install lab-drivers "
            "v0.3.2 from scripts/requirements-dp711_hv_sweep.txt before running this script."
        ) from exc

    raise SystemExit(
        "Could not resolve the RigolDP711 class from lab_drivers. Check the installed lab_drivers version."
    )


def build_hv_sweep(start: float, peak: float, step_magnitude: float) -> list[float]:
    """Build a there-and-back HV sweep from ``start`` to ``peak`` and back."""
    if start == peak:
        return [start]

    direction = 1.0 if peak > start else -1.0
    step = direction * abs(step_magnitude)
    epsilon = abs(step) * 1e-9 + 1e-12

    up: list[float] = []
    current = start
    while (direction > 0.0 and current <= peak + epsilon) or (direction < 0.0 and current >= peak - epsilon):
        up.append(round(current, 9))
        current += step

    if not up or abs(up[-1] - peak) > epsilon:
        up.append(round(peak, 9))

    down = list(reversed(up[:-1]))
    return up + down


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


def safe_dp711_measure_voltage(dp711) -> float:
    try:
        return float(dp711.measure_voltage())
    except Exception:
        return math.nan


def safe_dp711_measure_current(dp711) -> float:
    try:
        return float(dp711.measure_current())
    except Exception:
        return math.nan


def write_rows(csv_path: Path, points: Iterable[SweepPoint], delay_s: float, ratio: float) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "host_timestamp_utc",
        "step_index",
        "delay_s",
        "ratio_hv_per_v",
        "hv_target_v",
        "dp711_setpoint_v",
        "dp711_measured_voltage_v",
        "dp711_measured_current_a",
        "expected_hv_v",
        "measured_hv_v",
        "error_v",
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
        for point in points:
            sample = point.adc_sample
            writer.writerow(
                {
                    "host_timestamp_utc": datetime.now(timezone.utc).isoformat(),
                    "step_index": point.step_index,
                    "delay_s": delay_s,
                    "ratio_hv_per_v": ratio,
                    "hv_target_v": point.hv_target_v,
                    "dp711_setpoint_v": point.dp711_setpoint_v,
                    "dp711_measured_voltage_v": point.dp711_measured_voltage_v,
                    "dp711_measured_current_a": point.dp711_measured_current_a,
                    "expected_hv_v": point.expected_hv_v,
                    "measured_hv_v": "" if point.measured_hv_v is None else point.measured_hv_v,
                    "error_v": "" if point.error_v is None else point.error_v,
                    "stm32_status": point.stm32_status,
                    "stm32_tick_ms": "" if sample is None else sample.tick_ms,
                    "stm32_raw_sample": "" if sample is None else sample.raw_sample,
                    "stm32_hv_vmon_m_v": "" if sample is None else sample.hv_vmon_m_v,
                    "stm32_hv_filt_v": "" if sample is None else sample.hv_filt_v,
                    "stm32_console_line": "" if sample is None else sample.console_line,
                }
            )


def plot_results(points: list[SweepPoint], plot_path: Path, show: bool) -> None:
    try:
        import matplotlib
    except ImportError as exc:
        raise SystemExit(
            "Missing dependency 'matplotlib'. Install scripts/requirements-dp711_hv_sweep.txt "
            "or rerun with --no-plot."
        ) from exc

    if not show:
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt

    steps = [p.step_index for p in points]
    expected = [p.expected_hv_v for p in points]

    measured_steps = [p.step_index for p in points if p.measured_hv_v is not None]
    measured = [p.measured_hv_v for p in points if p.measured_hv_v is not None]
    error_steps = [p.step_index for p in points if p.error_v is not None]
    error = [p.error_v for p in points if p.error_v is not None]

    fig, (ax_main, ax_err) = plt.subplots(
        2,
        1,
        sharex=True,
        figsize=(10, 7),
        gridspec_kw={"height_ratios": [3, 1]},
    )

    ax_main.plot(steps, expected, linestyle=":", color="tab:blue", marker="o", label="Expected HV")
    ax_main.plot(measured_steps, measured, linestyle="-", color="tab:red", marker=".", label="Measured HV")
    ax_main.set_ylabel("HV (V)")
    ax_main.set_title("DP711-commanded HV sweep vs. STM32 ADS7822 measurement")
    ax_main.grid(True, linestyle="--", alpha=0.4)
    ax_main.legend(loc="best")

    ax_err.axhline(0.0, color="gray", linewidth=0.8)
    ax_err.plot(error_steps, error, linestyle="-", color="tab:green", marker=".", label="Error")
    ax_err.set_ylabel("Error (V)")
    ax_err.set_xlabel("Sweep step")
    ax_err.grid(True, linestyle="--", alpha=0.4)

    fig.tight_layout()

    plot_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(plot_path, dpi=150)
    print(f"Wrote plot to {plot_path}")

    if show:
        plt.show()

    plt.close(fig)


def main() -> int:
    args = parse_args()
    validate_args(args)

    csv_path = args.output or default_output_path()
    plot_path = args.plot_output or csv_path.with_suffix(".png")
    hv_filt_gain = (args.hv_filt_top_ohms + args.hv_filt_bottom_ohms) / args.hv_filt_bottom_ohms

    hv_targets = build_hv_sweep(args.hv_start, args.hv_peak, args.step_size)

    serial_module = load_serial_module()
    dp711_cls = load_dp711_driver()

    serial_port = open_serial_port(serial_module, args.stm32_port, args.stm32_baud)
    try:
        dp711 = dp711_cls(auto_connect=False)
    except Exception as exc:
        serial_port.close()
        raise SystemExit(f"Unable to construct the RigolDP711 driver: {exc}") from exc

    points: list[SweepPoint] = []
    output_was_enabled = False

    try:
        try:
            dp711.connect(com_port=args.dp711_port, baud_rate=args.dp711_baud)
        except Exception as exc:
            raise SystemExit(f"Unable to connect to the DP711 on {args.dp711_port or 'auto'}: {exc}") from exc

        print(f"DP711 port: {args.dp711_port or 'auto'} @ {args.dp711_baud}")
        print(f"STM32 port: {args.stm32_port} @ {args.stm32_baud}")
        print(f"HV ratio: {args.ratio} V(HV)/V(DP711); sweep points: {len(hv_targets)}")
        print(f"ADC transfer: HV = ADC_V * {args.hv_per_adc_volt}")
        print(f"CSV output: {csv_path}")

        for index, hv_target in enumerate(hv_targets, start=1):
            setpoint = hv_target / args.ratio
            if setpoint < 0.0:
                raise SystemExit(
                    f"Computed DP711 setpoint {setpoint:.3f} V is negative for HV target {hv_target:.1f} V. "
                    "Check --ratio, --hv-start, and --hv-peak signs."
                )

            print(f"[{index}/{len(hv_targets)}] HV target {hv_target:.1f} V -> DP711 {setpoint:.3f} V")
            dp711.set_voltage(setpoint)
            if not output_was_enabled:
                dp711.set_output_state(True)
                output_was_enabled = True

            flush_serial_input(serial_port)

            if args.delay > 0.0:
                time.sleep(args.delay)

            dp711_v = safe_dp711_measure_voltage(dp711)
            dp711_i = safe_dp711_measure_current(dp711)

            adc_sample, stm32_status = read_next_adc_sample(
                serial_port=serial_port,
                sample_timeout_s=args.adc_sample_timeout,
                vref=args.vref,
                hv_filt_gain=hv_filt_gain,
                echo_console=args.echo_console,
            )

            measured_hv_v: float | None = None
            error_v: float | None = None
            if adc_sample is not None:
                measured_hv_v = adc_sample.hv_vmon_m_v * args.hv_per_adc_volt
                error_v = measured_hv_v - hv_target

            points.append(
                SweepPoint(
                    step_index=index,
                    hv_target_v=hv_target,
                    dp711_setpoint_v=setpoint,
                    expected_hv_v=hv_target,
                    measured_hv_v=measured_hv_v,
                    error_v=error_v,
                    stm32_status=stm32_status,
                    dp711_measured_voltage_v=dp711_v,
                    dp711_measured_current_a=dp711_i,
                    adc_sample=adc_sample,
                )
            )

            if measured_hv_v is None:
                print(f"  STM32 status={stm32_status} (no sample captured)")
            else:
                print(
                    "  measured HV={0:.1f} V (raw={1}), error={2:.1f} V".format(
                        measured_hv_v, adc_sample.raw_sample, error_v
                    )
                )

        write_rows(csv_path, points, args.delay, args.ratio)
        print(f"Wrote {len(points)} sweep samples to {csv_path}")

        if not args.no_plot:
            plot_results(points, plot_path, show=not args.no_show)

        return 0
    finally:
        try:
            if output_was_enabled and not args.leave_output_on:
                dp711.set_voltage(0.0)
                dp711.set_output_state(False)
        except Exception:
            pass

        try:
            dp711.disconnect()
        except Exception:
            pass

        try:
            serial_port.close()
        except Exception:
            pass


if __name__ == "__main__":
    sys.exit(main())
