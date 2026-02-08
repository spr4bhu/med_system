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

### Node A (Gateway)
- MPU6050 (I2C: GPIO 21/22)
- DHT11 (1-wire: GPIO 5)
- HW-827 (ADC: GPIO 36)
- Emergency Button (GPIO 4)

### Node B (Sentry)
- RC522 RFID (SPI: GPIO 23/25/19/22)
- IR Motion Sensor (GPIO 13)
- Buzzer (GPIO 5, NPN driver)

### Power
- 3.3V regulated via AMS1117
- USB-C or Li-ion battery input
- Bulk + decoupling capacitors

## Usage

1. Open `Hackthon_PCB.kicad_pro` in KiCAD 8.x+
2. Schematic: `Hackthon_PCB.kicad_sch`
3. Layout: `Hackthon_PCB.kicad_pcb`
4. Export gerbers from PCB layout for manufacturing
