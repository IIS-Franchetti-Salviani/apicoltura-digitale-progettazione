@ -0,0 +1,234 @@
# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## Project Overview

This is an **IoT beekeeping monitoring system** (Apicoltura Digitale) developed as a high school project work (a.s. 2025/2026). The system collects environmental and operational data from beehives using ESP32-CAM microcontrollers with multiple sensors, transmits data via REST APIs to a cloud database, and provides a mobile web interface for beekeepers.

**Key Components:**
- ESP32-CAM device with sensors (temperature, humidity, weight, audio, water level, camera)
- 4G LTE router (Strong 300 Mini) for connectivity
- REST API server (PHP)
- MySQL database (hosted on restdb.io)
- Mobile web application

## Architecture

### Hardware Layer (ESP32-CAM)
- **Sensors:**
  - SHT21/HTU21D: Temperature and humidity (I2C on GPIO14/15)
  - DS18B20: Internal temperature (1-Wire on GPIO2)
  - HX711 + Load Cell: Hive weight measurement (GPIO12/13)
  - INMP441/KY-038: Audio monitoring (GPIO12)
  - HW-038: Water level detection
  - ESP32-CAM: Visual monitoring of hive entrance

- **Pin Assignments:** See `DOCUMENTAZIONE.md` section "Tabelle dei PIN" for complete wiring

### Firmware Architecture
The firmware follows a **modular validation pattern** with centralized error handling:

```
CodiceSensori/
├── SensorValidation.h        # Common validation library (error codes, validation logic)
├── MainIDE/
│   ├── MainIDE.ino           # Main controller (setup + loop coordination)
│   ├── Temperatura.ino       # Temperature sensor logic
│   ├── Umidita.ino          # Humidity sensor logic
│   └── sensore_peso_alert.ino # Weight sensor logic
└── DOCUMENTAZIONE.md         # Complete system documentation
```

**Key Design Patterns:**
1. **Centralized Validation:** `SensorValidation.h` provides reusable validation functions and standardized error codes (1xxx=common, 2xxx=alerts, 3xxx=weight, 4xxx=humidity, 5xxx=temperature)
2. **Modular Sensors:** Each sensor has its own .ino file with `setup<Sensor>()` and `funzione<Sensor>()` functions
3. **Main Coordinator:** `MainIDE.ino` orchestrates timing and calls sensor functions at appropriate intervals
4. **Fail-Safe Design:** Invalid sensor data returns safe defaults; system never crashes on bad data

### Network Layer
- **Local:** ESP32-CAM connects via Wi-Fi (IEEE 802.11 b/g/n, 2.4GHz) to router
- **WAN:** 4G LTE router provides internet connectivity (see `documentazine Standard di rete.md`)
- **Protocol:** REST over HTTP/HTTPS with JSON payloads
- **IP Assignment:** DHCP from router to ESP32 devices

### Data Model
See `EsempiJSON/` for payload formats. Key entities:
- **Apiario** (apiary): Collection of hives with geographic location
- **Arnia** (hive): Individual hive with installation date
- **Sensore** (sensor): Sensor metadata (type, state, thresholds)
- **Rilevazione** (measurement): Sensor data with timestamp
- **Notifica** (notification): Alerts when thresholds exceeded

## Common Development Commands

### Building/Flashing Firmware
```bash
# Open Arduino IDE and load CodiceSensori/MainIDE/MainIDE.ino
# Board: ESP32 Dev Module (or AI-Thinker ESP32-CAM)
# Upload Speed: 115200
# Flash Frequency: 80MHz
```

**Required Arduino Libraries:**
- OneWire
- DallasTemperature
- Wire (built-in)
- Adafruit_HTU21DF (for SHT21)

### Testing Sensors
```bash
# Monitor serial output at 115200 baud
# System prints validation results with error codes
# Each sensor runs at configured intervals (see MainIDE.ino timing constants)
```

### Git Workflow
This project uses **feature branches** per team:
```bash
# Always pull first
git pull

# Create feature branch
git checkout -b feature/nome-funzionalita

# Commit with descriptive message (participio passato)
git add .
git commit -m "Aggiunto endpoint /misure con validazione JSON"

# Push branch
git push -u origin feature/nome-funzionalita
```

**Commit Message Standards:**
- Use past participle form (Italian): "Aggiunto", "Corretto", "Implementato"
- Be specific: describe what changed, not "fix" or "update"
- Include problem/solution/tests if complex

## Error Code System

The system uses a **hierarchical error code scheme** (see `SensorValidation.h`):

- **1xxx - Common Errors:** Data validation (1001=null, 1002=NaN, 1101=out of range), sensor state (1201=not ready), network (1501=offline)
- **2xxx - Alerts:** Threshold violations (2001=low, 2002=high), quality issues
- **3xxx - Weight-Specific:** ADC saturation (3003), calibration errors (3101-3104), conversion failures (3201-3203)
- **4xxx - Humidity-Specific:** I2C communication (4002), sensor faults (4001)
- **5xxx - Temperature-Specific:** OneWire failures (5003), CRC errors (5002)
- **9xxx - Status:** 9000=OK, 9001=OK with warning

**Usage Example:**
```cpp
RisultatoValidazione result = validaDatoSensore(valore, timestamp, ready, config);
if (!result.valido) {
  // result.codiceErrore contains specific error code
  // result.valorePulito contains safe default
  // result.messaggioErrore contains description
}
```

## Measurement Intervals and Thresholds

**Default Intervals** (configured in `MainIDE.ino`):
- Temperature: 6 minutes (360000 ms)
- Humidity: 6 minutes (360000 ms)
- Weight: 3 hours with deep sleep
- Audio: 10 seconds burst, aggregated every minute
- Water level: 40 minutes

**Alert Thresholds** (loaded from database):
- Temperature: 30-37°C operational range
- Humidity: 40-70% RH
- Weight: Alerts for abnormal changes (too high/low)
- Audio: Frequency analysis 200-600Hz for swarming detection

## Requirements and Specifications

See `REQUISITI.md` for complete functional and non-functional requirements.

**Key Functional Requirements:**
- RF-SW-01 to RF-SW-43: Firmware and sensor behavior
- RF-HW-01 to RF-HW-05: Hardware sensor specifications
- RF-DB-01 to RF-DB-06: Database schema and operations
- RF-NET-01 to RF-NET-06: Network architecture and connectivity

**Non-Functional Constraints:**
- RNF-SW-01: Local persistence when network unavailable
- RNF-HW-01: IP65/IP67 protection for outdoor sensors
- RNF-HW-02: Anti-propolis protection for internal sensors
- RNF-NET-01: Solar + battery power autonomy

## Project Team Structure

4 working groups (each has own feature branch):
- **Group A (Database):** Schema design, queries, data model on restdb.io
- **Group B (Hardware):** Sensor selection, wiring, physical installation
- **Group C (Firmware):** ESP32 code, REST communication, data acquisition (this codebase)
- **Group D (Network):** Router config, IP addressing, power management

## Important Context: Beekeeping Domain

**Why these sensors matter:**
- **Weight:** Indicates honey production and colony health (nectar accumulation vs consumption)
- **Temperature:** Brood nest must stay ~35°C; deviations indicate ventilation problems
- **Humidity:** High RH (>65%) prevents honey ripening; low causes stress
- **Audio:** Specific frequencies indicate swarming (400-500Hz "piping"), orphaning, or distress
- **Water:** Bees need water for thermoregulation; depletion risks thermal collapse
- **Visual:** Entrance activity shows colony health without opening hive

**Environmental Challenges:**
- **Propolis:** Bees coat foreign objects with resin, can seal sensors within 48 hours (requires mesh protection)
- **Acids:** Varroa treatments (formic/oxalic acid) corrode standard electronics
- **Weather:** IP65+ rating required; UV exposure, rain, frost conditions
- **Power:** Remote locations require solar + battery; deep sleep modes critical

## Code Style and Conventions

**When modifying firmware:**
- Always include sensor in validation config before reading
- Use `gestisciRisultatoValidazione()` for consistent logging
- Sensor functions should return `RisultatoValidazione` struct
- Timing managed in `MainIDE.ino` loop, not individual sensors
- Comment complex logic in Italian (project language)
- Pin assignments documented in `DOCUMENTAZIONE.md` "Tabelle dei PIN"

**When adding new sensors:**
1. Add sensor-specific error codes to `SensorValidation.h` enum
2. Create `Sensore<Nome>.ino` with `setup<Nome>()` and `funzione<Nome>()`
3. Define `ConfigValidazioneSensore` with appropriate ranges
4. Add function declarations in `MainIDE.ino` with `extern`
5. Call setup in `setup()` and function in `loop()` with timing
6. Update `DOCUMENTAZIONE.md` with pin assignments and requirements

## Testing and Validation

**Simulating Errors** (for testing without hardware):
```cpp
// In acquisition function, temporarily force error conditions:
float valore = NAN;              // Test NaN handling
float valore = -999;             // Test out-of-range
bool sensoreReady = false;       // Test sensor offline
unsigned long timestamp = 0;     // Test missing timestamp
```

**Serial Monitor Output Format:**
```
✓ [TEMP][9000] Valore valido: 32.50
❌ ERRORE_COMUNE [HUM][1002] Dato NaN o Infinito (NaN)
[ALERT][2002] TEMP: 38.00 > 37.00
```

## Known Issues and Future Extensions

**Implemented:**
- Robust error handling with standardized codes
- Modular sensor architecture
- Safe defaults on failures
- Coordinated timing in main loop

**Planned/TODO:**
- Store-and-forward for offline data buffering
- Retry logic with exponential backoff
- Watchdog timer for crash recovery
- Persistent error counters in EEPROM
- OTA firmware updates
- HTTPS/TLS for secure communication