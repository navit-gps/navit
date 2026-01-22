# APRS Frequency Reference

Complete worldwide list of APRS frequencies for 2-meter (VHF) operations.

## Primary Frequencies by Region

### 144.390 MHz
- **North America**: United States, Canada
- **South America**: Colombia, Chile
- **Asia**: Indonesia, Malaysia, Singapore, Thailand

### 144.575 MHz
- **Oceania**: New Zealand

### 144.640 MHz
- **Asia**: China, Taiwan, Japan (main island)

### 144.660 MHz
- **Asia**: Japan (Kyushu area)

### 144.800 MHz
- **Europe**: Unified frequency for IARU Region 1
  - Austria
  - Belgium
  - Denmark
  - Finland
  - France
  - Greece
  - Hungary
  - Italy
  - The Netherlands
  - Norway
  - Portugal
  - Slovenia
  - Spain
  - Sweden
  - Switzerland
  - Turkey
  - United Kingdom
- **Africa**: South Africa
- **Asia**: Russia

### 144.930 MHz
- **South America**: Argentina, Uruguay
- **Central America**: Panama

### 144.990 MHz
- **North America**: Vancouver, Canada (secondary frequency for handhelds)

### 145.175 MHz
- **Oceania**: Australia (nationwide)

### 145.570 MHz
- **South America**: Brazil

## Usage Notes

1. **Primary vs Secondary**: Most regions have a primary frequency. Secondary frequencies may exist for specific use cases (e.g., handheld operations, local coordination).

2. **IARU Regions**:
   - **Region 1**: Europe, Africa, Middle East
   - **Region 2**: Americas
   - **Region 3**: Asia-Pacific

3. **Channel Spacing**: Most regions use 12.5 kHz or 25 kHz channel spacing.

4. **Modulation**: All frequencies use Bell 202 / 2FSK modulation at 1200 bps.

5. **Bandwidth**: APRS signals typically occupy approximately 6 kHz of bandwidth.

## Configuration Examples

### North America (USA/Canada)
```
Frequency: 144.390 MHz
Modulation: Bell 202
Data Rate: 1200 bps
```

### Europe
```
Frequency: 144.800 MHz
Modulation: Bell 202
Data Rate: 1200 bps
```

### Australia
```
Frequency: 145.175 MHz
Modulation: Bell 202
Data Rate: 1200 bps
```

### Japan
```
Frequency: 144.640 MHz (main island)
Frequency: 144.660 MHz (Kyushu)
Modulation: Bell 202
Data Rate: 1200 bps
```

## Integration with Navit APRS Plugin

The Navit APRS plugin does not directly configure frequencies. You must configure your:

1. **SDR Software**: Set center frequency in your SDR application
2. **TNC Hardware**: Configure frequency via TNC settings
3. **Demodulator**: Ensure multimon-ng, direwolf, or similar is tuned correctly

Example for multimon-ng (North America):
```bash
multimon-ng -a AFSK1200 -t raw /dev/ttyUSB0 | navit_aprs_processor
```

Example for direwolf (Europe):
```bash
direwolf -c direwolf.conf -r 48000 | navit_aprs_processor
```

Where `direwolf.conf` contains:
```
ADEVICE plughw:1,0
ACHANNELS 1
CHANNEL 0
MYCALL YOURCALL
MODEM 1200
```

## References

- IARU Region 1 VHF Manager's Handbook
- APRS Protocol Specification
- Local amateur radio regulations (check before transmitting)

## Legal Notice

Always ensure you have proper licensing and authorization before transmitting on amateur radio frequencies. This plugin is for receive-only operation unless you have appropriate licenses and permissions.

