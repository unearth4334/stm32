Copilot said:
SPI Conversion and Readout Sequence

The ADC uses a 3‑wire SPI-style interface consisting of:

CS/SHDN – Chip Select / Shutdown control (active low)
DCLOCK – Serial clock input
DOUT – Serial data output

A conversion is initiated when CS/SHDN is driven low. After a short setup time (tSUCS), the ADC begins sampling the input and subsequently performs the conversion.

Data Output Format

CS/SHDN goes low

DOUT remains in Hi-Z for a short period (tCSD).
The first clock causes a null bit to be output.

Conversion result transmission

Following the null bit, the ADC outputs the 12-bit conversion result.
The first valid data bit is B11 (MSB).
Data is shifted out MSB-first in the order:
Null, B11, B10, B9, B8, B7, B6, B5, B4, B3, B2, B1, B0


End of conversion

By the time B0 is transmitted, the conversion is complete (tCONV).
Additional Clock Cycles

After the 12-bit result has been shifted out:

Normal Mode (Top Timing Diagram)

If additional clocks are applied while CS/SHDN remains low:

The ADC outputs the conversion result again, but LSB-first:

B0, B1, B2, ... B11


After the LSB-first data is exhausted, the ADC continuously outputs zeros.

Power-Down Mode (Bottom Timing Diagram)

If CS/SHDN remains low after the conversion completes:

The ADC powers down its bias circuitry and comparator during tDATA.
The reference input becomes high impedance.
The serial clock may continue running.
The device outputs:
the conversion word in LSB-first order, followed by
zeros indefinitely.
DOUT returns to Hi-Z when CS/SHDN is deasserted (high).
Host Read Procedure

Typical firmware sequence:

Pull CS/SHDN low.
Generate one clock to read/discard the null bit.
Generate 12 additional clocks and capture:
B11 → B0

Assemble the 12 bits into the ADC result.
Drive CS/SHDN high to end the transaction and place the ADC into its standby/shutdown state.
Example

If the conversion result is:

0xB53 = 1011 0101 0011b


The DOUT sequence is:

Null, 1,0,1,1,0,1,0,1,0,0,1,1
      B11                     B0


The host ignores the null bit and reconstructs the 12-bit value from the following 12 clocked bits.