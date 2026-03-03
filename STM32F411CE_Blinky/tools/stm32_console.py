#!/usr/bin/env python3
import argparse
import glob
import os
import queue
import sys
import threading
import time
from typing import Optional

import serial
from serial.tools import list_ports

DEFAULT_VID = 0x0483
DEFAULT_PID = 0x5740


class ConsoleError(Exception):
    pass


def _candidate_ports() -> list[str]:
    ports = [port.device for port in list_ports.comports()]

    if os.name == "nt":
        return ports

    extra = sorted(glob.glob("/dev/ttyACM*")) + sorted(glob.glob("/dev/ttyUSB*"))
    for device in extra:
        if device not in ports:
            ports.append(device)

    return ports


def auto_detect_port(preferred_vid: int, preferred_pid: int) -> Optional[str]:
    matched = []
    fallback = []

    for port in list_ports.comports():
        if (port.vid == preferred_vid) and (port.pid == preferred_pid):
            matched.append(port.device)
        elif port.device:
            fallback.append(port.device)

    if matched:
        return matched[0]

    for dev in _candidate_ports():
        if dev not in fallback:
            fallback.append(dev)

    return fallback[0] if fallback else None


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Cross-platform STM32 console for STS4x command interface"
    )
    parser.add_argument(
        "--port",
        default="auto",
        help="Serial port (e.g. /dev/ttyACM0 or COM4). Default: auto",
    )
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument(
        "--timeout",
        type=float,
        default=0.05,
        help="Serial read timeout in seconds (default: 0.05)",
    )
    parser.add_argument(
        "--newline",
        choices=["crlf", "lf", "cr"],
        default="crlf",
        help="Line ending when sending commands (default: crlf)",
    )
    parser.add_argument(
        "--command",
        action="append",
        default=[],
        help="Command to send (can be repeated). If used, runs batch mode and exits.",
    )
    parser.add_argument(
        "--wait-after",
        type=float,
        default=0.3,
        help="Seconds to wait after each batch command (default: 0.3)",
    )
    return parser


def newline_bytes(newline: str) -> bytes:
    if newline == "lf":
        return b"\n"
    if newline == "cr":
        return b"\r"
    return b"\r\n"


def send_command(ser: serial.Serial, command: str, eol: bytes) -> None:
    payload = command.encode("utf-8") + eol
    ser.write(payload)
    ser.flush()


def _reader_worker(ser: serial.Serial, out_queue: queue.Queue[str], stop_event: threading.Event) -> None:
    while not stop_event.is_set():
        try:
            data = ser.read(256)
        except serial.SerialException:
            out_queue.put("\n[error] serial read failed\n")
            break

        if data:
            out_queue.put(data.decode("utf-8", errors="replace"))


def _printer_worker(out_queue: queue.Queue[str], stop_event: threading.Event) -> None:
    while not stop_event.is_set() or not out_queue.empty():
        try:
            chunk = out_queue.get(timeout=0.1)
        except queue.Empty:
            continue
        sys.stdout.write(chunk)
        sys.stdout.flush()


def run_batch_mode(ser: serial.Serial, commands: list[str], eol: bytes, wait_after: float) -> None:
    for cmd in commands:
        print(f"$ {cmd}")
        send_command(ser, cmd, eol)
        time.sleep(wait_after)
        data = ser.read_all()
        if data:
            print(data.decode("utf-8", errors="replace"), end="")


def run_interactive_mode(ser: serial.Serial, eol: bytes) -> None:
    print("Interactive mode. Type 'quit' or 'exit' to leave.")

    out_queue: queue.Queue[str] = queue.Queue()
    stop_event = threading.Event()

    reader = threading.Thread(target=_reader_worker, args=(ser, out_queue, stop_event), daemon=True)
    printer = threading.Thread(target=_printer_worker, args=(out_queue, stop_event), daemon=True)
    reader.start()
    printer.start()

    try:
        while True:
            line = input("host> ").strip()
            if line.lower() in {"quit", "exit"}:
                break
            if not line:
                continue
            send_command(ser, line, eol)
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        stop_event.set()
        reader.join(timeout=0.5)
        printer.join(timeout=0.5)


def open_console(port: str, baud: int, timeout: float) -> serial.Serial:
    try:
        return serial.Serial(port=port, baudrate=baud, timeout=timeout)
    except serial.SerialException as exc:
        raise ConsoleError(f"Failed to open port {port}: {exc}") from exc


def main() -> int:
    args = build_parser().parse_args()
    eol = newline_bytes(args.newline)

    port = args.port
    if port == "auto":
        port = auto_detect_port(DEFAULT_VID, DEFAULT_PID)
        if port is None:
            print("No serial device found. Connect the board and try again.")
            return 2

    print(f"Using port: {port}")

    try:
        ser = open_console(port, args.baud, args.timeout)
    except ConsoleError as exc:
        print(str(exc))
        return 1

    with ser:
        time.sleep(0.2)
        boot_data = ser.read_all()
        if boot_data:
            print(boot_data.decode("utf-8", errors="replace"), end="")

        if args.command:
            run_batch_mode(ser, args.command, eol, args.wait_after)
        else:
            run_interactive_mode(ser, eol)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
