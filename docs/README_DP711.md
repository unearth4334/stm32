_Auto-generated from MkDocs render output in the main repository (including mkdocstrings). Edit source files, not the Wiki directly._

# RigolDP711

## `lab_drivers.drivers.serial.RigolDP711.RigolDP711(auto_connect=True, com_port=None, baud_rate=9600)`

Rigol DP711 programmable power-supply driver.

Serial-backed driver with voltage/current control, output state commands,
and measurement helpers for bench automation.

Initialize RigolDP711 driver.

Args:
auto\_connect: Automatically connect to device on initialization
com\_port: Optional explicit COM port (e.g., 'COM4', '/dev/ttyUSB0')
baud\_rate: Serial baud rate (default: 9600)

### `connect(com_port=None, baud_rate=9600)`

Establish connection to RigolDP711 power supply.

Args:
com\_port: Optional serial port (for example, `"COM4"` or `"/dev/ttyUSB0"`).
baud\_rate: Serial baud rate.

Returns:
None

Example:
>>> psu = RigolDP711(auto\_connect=False)
>>> psu.connect(com\_port="/dev/ttyUSB0")

### `disconnect()`

Close the serial connection to the device.

Returns:
None

Example:
>>> psu.disconnect()

### `get(item, channel=1)`

Retrieve a measurement value by item name.

Args:
item: Measurement selector. Supported values are `"CURR"`, `"CURRENT"`,
`"VOLT"`, and `"VOLTAGE"` (case-insensitive).
channel: Unused placeholder for API compatibility.

Returns:
Measurement value in base SI units (A or V).

Raises:
ValueError: If `item` is not supported.
ConnectionError: If the device is not connected.

Example:
>>> volts = psu.get("VOLT")
>>> amps = psu.get("CURRENT")

### `set_voltage(voltage)`

Set the output voltage setpoint.

Args:
voltage: Target voltage in volts. Valid range is 0 to 30.

Returns:
None

Raises:
ValueError: If `voltage` is outside the supported range.
ConnectionError: If the device is not connected.

Example:
>>> psu.set\_voltage(12.0)

### `set_current(current)`

Set the output current limit.

Args:
current: Current limit in amperes. Valid range is 0 to 5.

Returns:
None

Raises:
ValueError: If `current` is outside the supported range.
ConnectionError: If the device is not connected.

Example:
>>> psu.set\_current(1.5)

### `measure_voltage()`

Measure the actual output voltage.

Returns:
Measured output voltage in volts.

Raises:
ValueError: If the returned value cannot be parsed.
ConnectionError: If the device is not connected.

Example:
>>> voltage = psu.measure\_voltage()

### `measure_current()`

Measure the actual output current.

Returns:
Measured output current in amperes.

Raises:
ValueError: If the returned value cannot be parsed.
ConnectionError: If the device is not connected.

Example:
>>> current = psu.measure\_current()

### `set_output_state(state)`

Enable or disable the power supply output.

Sends the absolute SCPI command (`:OUTP ON` / `:OUTP OFF`) rather
than a toggle, so the requested state is applied regardless of the
current output state. The state is then read back and re-asserted once
if the device did not report the requested state.

Args:
state: `True` to enable output, `False` to disable output.

Returns:
None

Raises:
ConnectionError: If the device is not connected.

Example:
>>> psu.set\_output\_state(True)

### `get_output_state()`

Query the current output state.

Returns:
`True` if the output is enabled, `False` otherwise.

Raises:
ConnectionError: If the device is not connected.
ValueError: If the device response cannot be parsed.

Example:
>>> psu.get\_output\_state()
True

### `turn_on()`

Turn on the power supply output.

Returns:
None

Example:
>>> psu.turn\_on()

### `turn_off()`

Turn off the power supply output.

Returns:
None

Example:
>>> psu.turn\_off()
