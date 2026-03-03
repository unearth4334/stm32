# STS40 Temperature Sensor Decoder for Saleae Logic2

A High Level Analyzer extension for Saleae Logic2 that decodes I2C communication with STS40/STS41/STS4L temperature sensors and displays temperature readings in degrees Celsius.

## Features

- **Temperature Decoding**: Automatically decodes temperature measurements from I2C data and converts to °C
- **Multiple Precision Modes**: Supports high, medium, and low precision measurement commands
- **Command Recognition**: Identifies and labels STS40 commands (measure, read serial, soft reset)
- **Multiple I2C Addresses**: Configurable support for addresses 0x44, 0x45, and 0x46
- **Serial Number Reading**: Decodes sensor serial number when requested
- **CRC Display**: Shows CRC checksum values for validation

## Installation

### Option 1: Local Installation (Development)

1. Clone or copy this extension to your local machine
2. Open Saleae Logic2
3. Go to **Extensions** > **Load Existing Extension**
4. Navigate to the `Logic2_STS40` folder and select it

### Option 2: GitHub Installation (Production)

1. Create a public GitHub repository and push this code
2. Open Saleae Logic2
3. Go to **Extensions** > **Install Extension**
4. Enter your GitHub repository URL
5. Install the extension

## Usage

### Prerequisites

- Saleae Logic2 application installed
- Logic analyzer connected and capturing I2C data
- I2C analyzer already configured on your channels

### Setup Instructions

1. **Capture I2C Data**:
   - Connect your logic analyzer to the STS40's SCL and SDA lines
   - Add an **I2C** analyzer to your capture
   - Configure the I2C analyzer with the appropriate settings (typically 100 kHz or 400 kHz)

2. **Add STS40 Analyzer**:
   - Click the **+** button next to **Analyzers**
   - Select **STS40 Temperature Sensor Decoder** from the High Level Analyzers list
   - Configure the I2C address to match your STS40 sensor (default: 0x44)
   - Select the **I2C** analyzer as the input source

3. **View Results**:
   - Start capturing data
   - The STS40 analyzer will automatically decode temperature readings
   - Temperature values will appear as: `Temperature: XX.XX °C`

## STS40 Commands Supported

| Command | Hex Code | Description | Response |
|---------|----------|-------------|----------|
| Measure (High) | 0xFD | High precision temperature measurement | 3 bytes (16-bit temp + CRC) |
| Measure (Medium) | 0xF6 | Medium precision temperature measurement | 3 bytes (16-bit temp + CRC) |
| Measure (Low) | 0xE0 | Low precision temperature measurement | 3 bytes (16-bit temp + CRC) |
| Read Serial | 0x89 | Read sensor serial number | 6 bytes (32-bit serial + CRCs) |
| Soft Reset | 0x94 | Reset the sensor | ACK only |

## Temperature Conversion Formula

The extension uses the formula from the STS4x datasheet:

```
T (°C) = -45 + 175 × (ST / 65535)
```

Where `ST` is the 16-bit raw temperature value read from the sensor.

## Example Output

When viewing a capture in Logic2, you'll see decoded frames like:

```
CMD: Measure T (High Precision)
Temperature: 23.45 °C
CMD: Measure T (Medium Precision)
Temperature: 24.12 °C
CMD: Read Serial Number
Serial: 0x12345678
Soft Reset
```

## Configuration Options

- **STS40 I2C Address**: Select the I2C address of your STS40 sensor
  - Options: 0x44 (default), 0x45, 0x46
  - The address is laser-marked on the sensor package (A, B, or C)

## Troubleshooting

### No decoded output appears

- Verify the I2C analyzer is working and showing address/data frames
- Check that the correct I2C address is configured (look at the sensor marking)
- Ensure the STS40 analyzer is connected to the I2C analyzer as input

### Temperature values seem incorrect

- Verify the I2C bus is working correctly (check ACK/NACK)
- Check for correct byte order (MSB first, then LSB)
- Verify timing requirements are met (see datasheet)

### CRC errors

- Check signal integrity on SCL and SDA lines
- Verify pull-up resistors are correctly sized
- Check for electrical noise or signal reflections

## Technical References

- **STS4x Datasheet**: See `documentation/HT_DS_Datasheet_STS4x.txt` for full specifications
- **I2C Specification**: Supports standard, fast mode, and fast mode plus
- **Accuracy**: ±0.2°C typical for STS40, ±0.4°C for STS4L
- **Operating Range**: -40°C to +125°C

## Development

### Testing the Extension

1. Load the extension locally in Logic2
2. Open a capture file with STS40 I2C communication
3. Add the STS40 analyzer
4. Verify temperature decoding is correct

### Modifying the Code

- `HighLevelAnalyzer.py`: Main decoder logic
- `extension.json`: Extension metadata and configuration
- `README.md`: This documentation file

## License

This extension is provided as-is for use with Saleae Logic2.

## Author

shy coder

## Version

0.0.1 - Initial release

---

## Additional Resources

- [Saleae Logic2 Extensions Documentation](https://support.saleae.com/extensions/high-level-analyzer-extensions)
- [Sensirion STS4x Product Page](https://www.sensirion.com/en/environmental-sensors/humidity-sensors/temperature-sensors/)
- [GitHub Repository](https://github.com/yourusername/Logic2_STS40) *(update with your repo)*