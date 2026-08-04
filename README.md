# LiTel - Live Telemetry & Data Logging System

**Panther Racing - Autonomous Subteam**

Designed to be a low-latency live data logging system. Collects data from VCU over CAN, saves to SD card and sends over RF using RFD900ux. Has USB-C port for easy data retrieval.


## Table of Contents
- [Hardware](#hardware)
- [STM32 Pinout](#stm32-pinout)
- [Data Flow](#data-flow)
- [Firmware](#firmware)


## Hardware

Schematic and PCB in Altium (2027 (PR-039)/Autonomous/LiTel)

[Design Sheet](https://docs.google.com/document/d/1RxNdBkyVu-avMjCsnAB_X7mWcfvH4GB2---DR_EiWPw/edit?usp=sharing)






## STM32 Pinout
| Pin | Pin Name | Function |
| --- | --- | --- |
|





## Data Flow






## Firmware

### File Structure
<pre>
LiTel/
├── Core/
│   ├── Src/
│   │   └── main.c              # HAL init → state_machine_init() → state_machine_poll().
│   └── Inc/
├── Applications/
│   ├── Inc/
│   │   ├── app_config.h        # General config: channel map, rates, build params
│   │   ├── state_machine.h     # System states, transition table
│   │   ├── can_ingest.h        # FDCAN init, RX filters, frame → record
│   │   ├── ring_buffer.h       # Ring buffer, block assembly, write scheduling
│   │   ├── sd_logger.h         # SDMMC + FatFs, mount/unmount, file rotation
│   │   └── rf_telemetry.h      # RFD900uX UART, packet framing, downsampling
│   └── Src/
│       ├── state_machine.c     # Handles running the rest of the stack depending on the current state
│       ├── can_ingest.c        # Grabs the incoming CAN messages and pushes them into the ring buffer
│       ├── ring_buffer.c       # Handles ring buffer
│       ├── sd_logger.c         # Mounts SD and manages writes
│       └── rf_telemetry.c      # Packages data to be sent over RF, sends to RFD900ux over UART
├── Drivers/                    # ST HAL (generated)
</pre>




