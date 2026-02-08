# Medical Monitoring System - PCB Design

KiCAD project for dual-node ESP32 medical monitoring and security system.

## Files

### Design Files
- **Hackthon_PCB.kicad_pro** - Project configuration
- **Hackthon_PCB.kicad_sch** - Schematic (both Node A & B)
- **Hackthon_PCB.kicad_pcb** - PCB layout

### Libraries
- **kicad_parts/** - Custom footprints and symbols (Charleslabs_Parts)
- **MY_lib.kicad_sym** - Additional custom symbols
- **sym-lib-table** - Symbol library configuration

### Manufacturing
- **gerbers/** - Gerber files for PCB fabrication
  - Copper layers (F_Cu, B_Cu)
  - Silkscreen layers
  - Solder mask layers
  - Drill files (PTH, NPTH)
  - Job file for fab house

## Design Notes

### Node A: Vital Detector (Medical Monitoring)

**Sensors:**
- MPU6050 (I2C: GPIO 21/22) - Fall detection + posture
- DHT11 (1-wire: GPIO 5) - Temperature & humidity
- HW-827 (ADC: GPIO 36) - Heart rate
- Emergency Button (GPIO 4) - Manual SOS trigger

**Schematic:**
![Vital Detector Schematic](vital%20detector%20schematic.jpeg)

**PCB Layout:**
![Vital Detector PCB](vital%20detector%20pcb.jpeg)

---

### Node B: Authenticator (Security Monitoring)

**Sensors:**
- RC522 RFID (SPI: GPIO 23/25/19/22) - Access control
- IR Motion Sensor (GPIO 13) - Intrusion detection
- Buzzer (GPIO 5, NPN driver) - Alarm output

**Schematic:**
![Authenticator Schematic](authenticator%20schematic.jpeg)

**PCB Layout:**
![Authenticator PCB](authenticator%20pcb.jpeg)

---

### Power Supply

- **Regulator:** AMS1117-3.3 (500mA)
- **Input:** USB-C 5V or Li-ion battery (3.7-4.2V)
- **Output:** 3.3V for ESP32 and peripherals
- **Decoupling:** 10µF bulk + 100nF per IC
- **Protection:** Reverse polarity diode, USB overvoltage

## Usage

1. Open `Hackthon_PCB.kicad_pro` in KiCAD 8.x+
2. Schematic: `Hackthon_PCB.kicad_sch`
3. Layout: `Hackthon_PCB.kicad_pcb`
4. Export gerbers from PCB layout for manufacturing
