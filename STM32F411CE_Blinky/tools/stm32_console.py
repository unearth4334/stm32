#!/usr/bin/env python3
import argparse
import glob
import os
import queue
import sys
import threading
import time
from typing import Optional

try:
    import readline  # type: ignore
except Exception:  # pragma: no cover
    readline = None

import serial
from serial.tools import list_ports

DEFAULT_VID = 0x0483
DEFAULT_PID = 0x5740


class ConsoleError(Exception):
    pass


LOCAL_COMMANDS = ["/help", "/ports", "/clear", "/quit"]


def _list_ports() -> list:
    return list(list_ports.comports())


def print_ports() -> None:
    ports = _list_ports()
    if not ports:
        print("No serial ports found.")
        return

    print("Available serial ports:")
    for port in ports:
        vid_pid = ""
        if (port.vid is not None) and (port.pid is not None):
            vid_pid = f" vid:pid={port.vid:04X}:{port.pid:04X}"
        desc = f" ({port.description})" if port.description else ""
        print(f"  - {port.device}{vid_pid}{desc}")


def print_interactive_help() -> None:
    print("Local commands:")
    print("  /help   Show this help")
    print("  /ports  List available serial ports")
    print("  /clear  Clear terminal")
    print("  /quit   Exit interactive mode")
    print("Device commands:")
    print("  help, status, read c, read f, read raw, serial, config ..., poll ...")


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
    acm_preferred = []
    usb_preferred = []
    other = []

    for port in list_ports.comports():
        if (port.vid == preferred_vid) and (port.pid == preferred_pid):
            matched.append(port.device)
        elif port.device:
            if "ttyACM" in port.device:
                acm_preferred.append(port.device)
            elif "ttyUSB" in port.device:
                usb_preferred.append(port.device)
            else:
                other.append(port.device)

    if matched:
        return matched[0]

    fallback = acm_preferred + usb_preferred + other

    for dev in _candidate_ports():
        if dev not in fallback:
            if "ttyACM" in dev:
                fallback.insert(0, dev)
            elif "ttyUSB" in dev:
                insert_index = len([d for d in fallback if "ttyACM" in d])
                fallback.insert(insert_index, dev)
            else:
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
    parser.add_argument(
        "--bar",
        type=int,
        default=0,
        metavar="WIDTH",
        help="Show temperature bar graph with WIDTH characters (e.g., --bar 10). Temp range: 0-50°C",
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


def _printer_worker(out_queue: queue.Queue[str], stop_event: threading.Event, bar_width: int = 0) -> None:
    """Print output from serial, optionally adding temperature bar graphs."""
    import re
    
    while not stop_event.is_set() or not out_queue.empty():
        try:
            chunk = out_queue.get(timeout=0.1)
        except queue.Empty:
            continue
        
        # If bar is enabled, check for polling temperature lines and add bar
        if bar_width > 0:
            lines = chunk.split('\n')
            processed_lines = []
            
            for line in lines:
                # Match "poll temp_c=XX.XX" pattern
                match = re.search(r'poll temp_c=([-\d.]+)', line)
                if match:
                    try:
                        temp_c = float(match.group(1))
                        bar = _make_temp_bar(temp_c, bar_width)
                        # Add bar after the temperature value
                        line = line.rstrip() + '\t' + bar
                    except ValueError:
                        pass  # Invalid temperature, skip bar
                
                processed_lines.append(line)
            
            chunk = '\n'.join(processed_lines)
        
        sys.stdout.write(chunk)
        sys.stdout.flush()


def _make_temp_bar(temp_c: float, width: int, temp_min: float = 0.0, temp_max: float = 50.0) -> str:
    """Create temperature bar using Braille characters.
    
    Args:
        temp_c: Temperature in Celsius
        width: Width of bar in characters
        temp_min: Minimum temperature for scale (default 0°C)
        temp_max: Maximum temperature for scale (default 50°C)
    
    Returns:
        String with Braille bar visualization
    """
    # Clamp temperature to range
    temp_clamped = max(temp_min, min(temp_max, temp_c))
    
    # Calculate fill ratio (0.0 to 1.0)
    ratio = (temp_clamped - temp_min) / (temp_max - temp_min)
    
    # Calculate number of filled characters
    filled_chars = int(ratio * width)
    
    # Build bar: ⠿ for filled, ⠇ for empty
    bar = '⠿' * filled_chars + '⠇' * (width - filled_chars)
    
    return bar


def run_batch_mode(ser: serial.Serial, commands: list[str], eol: bytes, wait_after: float, bar_width: int = 0) -> None:
    """Run commands in batch mode, optionally with temperature bar visualization."""
    import re
    
    for cmd in commands:
        print(f"$ {cmd}")
        send_command(ser, cmd, eol)
        time.sleep(wait_after)
        data = ser.read_all()
        if data:
            output = data.decode("utf-8", errors="replace")
            
            # If bar is enabled, add bars to polling lines
            if bar_width > 0:
                lines = output.split('\n')
                processed_lines = []
                
                for line in lines:
                    match = re.search(r'poll temp_c=([-\d.]+)', line)
                    if match:
                        try:
                            temp_c = float(match.group(1))
                            bar = _make_temp_bar(temp_c, bar_width)
                            line = line.rstrip() + '\t' + bar
                        except ValueError:
                            pass
                    
                    processed_lines.append(line)
                
                output = '\n'.join(processed_lines)
            
            print(output, end="")


def run_interactive_mode(ser: serial.Serial, eol: bytes, bar_width: int = 0) -> None:
    print("Interactive mode. Type /help for local commands. Type /quit to leave.")
    if bar_width > 0:
        print(f"Temperature bar enabled: {bar_width} chars (0-50°C range)")

    if readline is not None:
        readline.parse_and_bind("tab: complete")

        def completer(text: str, state: int) -> Optional[str]:
            choices = [cmd for cmd in LOCAL_COMMANDS if cmd.startswith(text)]
            if state < len(choices):
                return choices[state]
            return None

        readline.set_completer(completer)

    out_queue: queue.Queue[str] = queue.Queue()
    stop_event = threading.Event()

    reader = threading.Thread(target=_reader_worker, args=(ser, out_queue, stop_event), daemon=True)
    printer = threading.Thread(target=_printer_worker, args=(out_queue, stop_event, bar_width), daemon=True)
    reader.start()
    printer.start()

    try:
        while True:
            line = input("host> ").strip()
            if line.lower() in {"quit", "exit", "/quit"}:
                break
            if not line:
                continue

            if line == "/help":
                print_interactive_help()
                continue

            if line == "/ports":
                print_ports()
                continue

            if line == "/clear":
                if os.name == "nt":
                    os.system("cls")
                else:
                    os.system("clear")
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
            run_batch_mode(ser, args.command, eol, args.wait_after, args.bar)
        else:
            run_interactive_mode(ser, eol, args.bar)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
