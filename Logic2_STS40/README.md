# STS40 Temperature Sensor Decoder for Saleae Logic2

A High Level Analyzer extension for Saleae Logic2 that decodes I2C communication with STS40/STS41/STS4L temperature sensors and displays temperature readings in degrees Celsius.

## Features

- **Temperature Decoding**: Automatically decodes temperature measurements from I2C data and converts to °C and °F
- **Numeric Data Export**: Temperature values are available as numeric data for plotting and analysis
- **Data Table Integration**: View and export decoded values using Logic2's Data Table feature
- **Multiple Precision Modes**: Supports high, medium, and low precision measurement commands
- **Command Recognition**: Identifies and labels STS40 commands (measure, read serial, soft reset)
- **Multiple I2C Addresses**: Configurable support for addresses 0x44, 0x45, and 0x46
- **Serial Number Reading**: Decodes sensor serial number when requested
- **CRC Display**: Shows CRC checksum values for validation
- **Visual Temperature Display**: Shows readings with temperature emoji (🌡️) in both °C and °F

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
   - Temperature values will appear as: `🌡️ XX.XX°C (XX.XX°F)`

## Visualizing Temperature Data

### Using Logic2's Data Table

The extension exports numeric temperature values that you can visualize:

1. **Open the Data Table**:
   - Click on the **Data Table** icon in Logic2
   - Select the **STS40** analyzer from the list
   - The table will show all decoded frames with their data

2. **View Temperature Values**:
   - The `celsius` column contains numeric temperature values in °C
   - The `fahrenheit` column contains values in °F
   - The `raw_decimal` column shows the raw 16-bit sensor value
   - These numeric columns can be exported for plotting

3. **Export for Plotting**:
   - Click **Export Table** in the Data Table view
   - Save as CSV file
   - Open in Excel, Python (matplotlib/pandas), or any plotting tool
   - Plot `Time` vs `celsius` for a temperature trend graph

### Example: Plotting with Python

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load exported CSV
data = pd.read_csv('sts40_data.csv')

# Filter only temperature frames
temp_data = data[data['type'] == 'temperature']

# Plot temperature over time
plt.figure(figsize=(12, 6))
plt.plot(temp_data['time'], temp_data['celsius'], marker='o', linestyle='-', linewidth=2)
plt.xlabel('Time (s)')
plt.ylabel('Temperature (°C)')
plt.title('STS40 Temperature Readings Over Time')
plt.grid(True, alpha=0.3)
plt.tight_layout()
plt.show()
```

**Ready-to-Use Script Included!** 

A complete plotting script (`plot_temperature.py`) is included with this extension. 

First, install the required dependencies:
```bash
pip install -r requirements.txt
```

Then run the plotter:
```bash
python plot_temperature.py sts40_data.csv
```

This will generate:
- Dual plots (Celsius and Fahrenheit)
- Statistical summary (mean, min, max, std deviation)
- High-resolution PNG output
- Autoscaled with grid and annotations

### Tips for Best Results

- **Continuous Monitoring**: The sensor should be polled regularly (e.g., 1 Hz) for smooth plots
- **Autoscaling**: Your plotting tool will automatically scale the Y-axis based on temperature range
- **Long Captures**: For thermal profiling, capture over extended periods (minutes to hours)
- **Multiple Sensors**: If monitoring multiple STS40 sensors, use different I2C addresses and create separate analyzer instances

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
🌡️ 23.45°C (74.21°F)
CMD: Measure T (Medium Precision)
🌡️ 24.12°C (75.42°F)
CMD: Read Serial Number
Serial: 0x12345678
Soft Reset
```

### Data Table Columns

When viewing the Data Table, temperature frames will include:

| Column | Type | Description | Use Case |
|--------|------|-------------|----------|
| `temp_c` | string | Formatted °C value | Display |
| `temp_f` | string | Formatted °F value | Display |
| `celsius` | **numeric** | Raw °C value | **Plotting/Export** |
| `fahrenheit` | **numeric** | Raw °F value | **Plotting/Export** |
| `raw_decimal` | numeric | 16-bit sensor value | Analysis |
| `raw_value` | string | Hex sensor value | Debugging |
| `crc` | string | CRC checksum | Validation |

The **numeric** `celsius` and `fahrenheit` columns are perfect for creating temperature plots!

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