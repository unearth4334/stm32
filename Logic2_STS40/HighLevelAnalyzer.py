# High Level Analyzer
# For more information and documentation, please go to https://support.saleae.com/extensions/high-level-analyzer-extensions
# STS40 Temperature Sensor I2C Decoder

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting


class Hla(HighLevelAnalyzer):
    """
    High Level Analyzer for STS40 Temperature Sensor I2C Communication
    Decodes temperature measurements and displays them in degrees Celsius
    """
    
    # STS40 I2C address setting (default addresses: 0x44, 0x45, 0x46)
    i2c_address = ChoicesSetting(
        label='STS40 I2C Address',
        choices=('0x44', '0x45', '0x46'),
    )
    
    # STS40 Commands
    CMD_MEASURE_HIGH_PRECISION = 0xFD
    CMD_MEASURE_MEDIUM_PRECISION = 0xF6
    CMD_MEASURE_LOW_PRECISION = 0xE0
    CMD_READ_SERIAL = 0x89
    CMD_SOFT_RESET = 0x94
    
    # Result types for different decoded frames
    result_types = {
        'error': {
            'format': 'Error: {{data.error_msg}}'
        },
        'command': {
            'format': 'CMD: {{data.cmd_name}}'
        },
        'temperature': {
            'format': 'Temperature: {{data.temp_c}} °C'
        },
        'serial': {
            'format': 'Serial: {{data.serial_hex}}'
        },
        'reset': {
            'format': 'Soft Reset'
        }
    }

    def __init__(self):
        """
        Initialize the STS40 decoder
        """
        # Get the configured I2C address
        self.sts40_address = int(self.i2c_address, 16)
        
        # State machine variables
        self.current_command = None
        self.temp_data_bytes = []
        self.serial_data_bytes = []
        self.current_address = None
        self.is_read_operation = False
        self.frame_start_time = None
        
        print(f"STS40 Decoder initialized for I2C address: {self.i2c_address}")

    def decode(self, frame: AnalyzerFrame):
        """
        Decode I2C frames from the STS40 temperature sensor
        
        Args:
            frame: Input AnalyzerFrame from the I2C analyzer
            
        Returns:
            AnalyzerFrame or None
        """
        
        # Handle different I2C frame types
        if frame.type == 'address':
            # Extract address and read/write flag
            address_byte = frame.data['address'][0]
            self.is_read_operation = frame.data.get('read', False)
            self.current_address = address_byte
            
            # Check if this is communication with the STS40
            if address_byte == self.sts40_address:
                self.frame_start_time = frame.start_time
                
                # If this is a read operation, we'll be receiving data
                if self.is_read_operation:
                    self.temp_data_bytes = []
                    self.serial_data_bytes = []
        
        elif frame.type == 'data':
            # Only process data for our STS40 address
            if self.current_address != self.sts40_address:
                return None
            
            data_byte = frame.data['data'][0]
            
            if not self.is_read_operation:
                # This is a command write
                return self._handle_command(data_byte, frame.start_time, frame.end_time)
            else:
                # This is a data read (temperature or serial number)
                return self._handle_read_data(data_byte, frame.start_time, frame.end_time)
        
        return None

    def _handle_command(self, cmd_byte, start_time, end_time):
        """
        Handle STS40 command bytes
        
        Args:
            cmd_byte: Command byte received
            start_time: Start time of the frame
            end_time: End time of the frame
            
        Returns:
            AnalyzerFrame with command information
        """
        self.current_command = cmd_byte
        
        if cmd_byte == self.CMD_MEASURE_HIGH_PRECISION:
            cmd_name = "Measure T (High Precision)"
        elif cmd_byte == self.CMD_MEASURE_MEDIUM_PRECISION:
            cmd_name = "Measure T (Medium Precision)"
        elif cmd_byte == self.CMD_MEASURE_LOW_PRECISION:
            cmd_name = "Measure T (Low Precision)"
        elif cmd_byte == self.CMD_READ_SERIAL:
            cmd_name = "Read Serial Number"
        elif cmd_byte == self.CMD_SOFT_RESET:
            self.current_command = None
            return AnalyzerFrame('reset', start_time, end_time, {})
        else:
            cmd_name = f"Unknown (0x{cmd_byte:02X})"
        
        return AnalyzerFrame('command', start_time, end_time, {
            'cmd_name': cmd_name,
            'cmd_byte': f"0x{cmd_byte:02X}"
        })

    def _handle_read_data(self, data_byte, start_time, end_time):
        """
        Handle data bytes read from STS40
        
        Args:
            data_byte: Data byte received
            start_time: Start time of the frame
            end_time: End time of the frame
            
        Returns:
            AnalyzerFrame with decoded data or None
        """
        # Check if this is temperature data
        if self.current_command in [self.CMD_MEASURE_HIGH_PRECISION, 
                                   self.CMD_MEASURE_MEDIUM_PRECISION, 
                                   self.CMD_MEASURE_LOW_PRECISION]:
            self.temp_data_bytes.append(data_byte)
            
            # Temperature data is 3 bytes: MSB, LSB, CRC
            if len(self.temp_data_bytes) == 3:
                result = self._decode_temperature(start_time, end_time)
                self.temp_data_bytes = []
                self.current_command = None
                return result
        
        # Check if this is serial number data
        elif self.current_command == self.CMD_READ_SERIAL:
            self.serial_data_bytes.append(data_byte)
            
            # Serial number is 6 bytes: MSB1, LSB1, CRC1, MSB2, LSB2, CRC2
            if len(self.serial_data_bytes) == 6:
                result = self._decode_serial(start_time, end_time)
                self.serial_data_bytes = []
                self.current_command = None
                return result
        
        return None

    def _decode_temperature(self, start_time, end_time):
        """
        Decode temperature from raw sensor data
        
        Formula from datasheet: T = -45 + 175 * (ST / 65535) °C
        
        Args:
            start_time: Start time of the frame
            end_time: End time of the frame
            
        Returns:
            AnalyzerFrame with temperature information
        """
        if len(self.temp_data_bytes) < 2:
            return AnalyzerFrame('error', start_time, end_time, {
                'error_msg': 'Insufficient temperature data'
            })
        
        # Combine MSB and LSB to get 16-bit raw value
        raw_temp = (self.temp_data_bytes[0] << 8) | self.temp_data_bytes[1]
        crc = self.temp_data_bytes[2] if len(self.temp_data_bytes) >= 3 else None
        
        # Convert to temperature in Celsius using datasheet formula
        temp_celsius = -45.0 + 175.0 * (raw_temp / 65535.0)
        
        # Format temperature to 2 decimal places
        temp_str = f"{temp_celsius:.2f}"
        
        return AnalyzerFrame('temperature', self.frame_start_time or start_time, end_time, {
            'temp_c': temp_str,
            'raw_value': f"0x{raw_temp:04X}",
            'crc': f"0x{crc:02X}" if crc is not None else "N/A"
        })

    def _decode_serial(self, start_time, end_time):
        """
        Decode serial number from raw sensor data
        
        Args:
            start_time: Start time of the frame
            end_time: End time of the frame
            
        Returns:
            AnalyzerFrame with serial number information
        """
        if len(self.serial_data_bytes) < 6:
            return AnalyzerFrame('error', start_time, end_time, {
                'error_msg': 'Insufficient serial data'
            })
        
        # Combine bytes to get serial number (ignoring CRC bytes at positions 2 and 5)
        serial_word1 = (self.serial_data_bytes[0] << 8) | self.serial_data_bytes[1]
        serial_word2 = (self.serial_data_bytes[3] << 8) | self.serial_data_bytes[4]
        serial_number = (serial_word1 << 16) | serial_word2
        
        serial_hex = f"0x{serial_number:08X}"
        
        return AnalyzerFrame('serial', self.frame_start_time or start_time, end_time, {
            'serial_hex': serial_hex,
            'serial_dec': str(serial_number)
        })
