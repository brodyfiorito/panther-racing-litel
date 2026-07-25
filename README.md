# LiTel - Live Telemetry & Data Logging System

**Panther Racing - Autonomous Subteam**

A CAN-based telemetry and data-logging system for the FSAE EV. It captures full-rate
sensor data to on-board storage for post-session analysis and streams a prioritized
subset of channels live over an RF link to a pit-side receiver. Designed alongside the
MoTeC M150 VCU for PR-039 and PR-040 integration, with a migration path toward a future
in-house VCU.

> **Status:** design / requirements phase. First-year scope is intentionally limited

---

## Table of Contents

- [Goals & Scope](#goals--scope)
- [Data Flow](#data-flow)
- [Wire Protocol (UART/RF Frame)](#wire-protocol-uartrf-frame)
- [Storage Design](#storage-design)
- [RF Link](#rf-link)
- [Time Synchronization](#time-synchronization)
- [Dashboard](#dashboard)
- [Firmware Requirements](#firmware-requirements)
- [Roadmap](#roadmap)

---

## Goals & Scope

**In scope (first year):**
- Log all desired VCU channels at full rate to removable storage.
- Stream a prioritized, decimated subset of channels live over RF to the pit.
- Reliable, framed, CRC-protected transmission with drop detection.
- RTC / synchronized timestamping across logged channels.
- A modular dashboard for live viewing and post-session review - basics only.

**Designed-for but deferred:**
- Track-map transmission from the autonomous SLAM stack (waits on the DS trackmap stack).
- Logs consumable as replay / ground-truth input for the autonomous state estimator.
- Migration toward an in-house VCU with a unified tuning + log platform.

**Explicit non-goals (year one):**
- A polished, fully featured dashboard on par with MoTeC i2.
- Bidirectional command/control over the link.

---

## Data Flow

Three concurrent paths with very different timing characteristics must be decoupled so
one cannot starve another:

| Path        | Character              | Consequence if blocked        |
|-------------|------------------------|-------------------------------|
| CAN RX      | Bursty                 | FIFO overflow → dropped frames |
| SD write    | Blocks for ms at a time| Stalls everything behind it   |
| RF TX       | Drains slowly (kbps)   | Backpressure on live stream   |

**Requirement:** ring buffers decouple `CAN RX → SD writer` and `CAN RX → RF selector →
RF TX`, with DMA-driven transfers on each path so a blocking SD write cannot cause CAN
frame loss. (See [Firmware Requirements](#firmware-requirements).)

The RF path carries only the priority channels to minimize bus load

---

## Wire Protocol (UART/RF Frame)

Data is packaged for UART/RF transmission in a framed, CRC-protected format:

```
[START] [LENGTH] [SEQ #] [ID/TYPE] [PAYLOAD] [CRC16] [END]
```

- **START / END** - frame delimiters.
- **LENGTH** - payload length.
- **SEQ #** - per-frame sequence number for drop detection (fire-and-forget; no retransmit).
- **ID/TYPE** - message class (telemetry channel group, map delta, map snapshot, status, …).
- **PAYLOAD** - message body, decoded via the shared schema (below).
- **CRC16** - integrity check over the frame.

**Single source of truth for channel definitions.** A DBC-like schema shared across
VCU ↔ logger ↔ dashboard defines every channel's ID, type, endianness, scaling, and
units. Nothing decodes by hardcoded offsets.

---

## Storage Design

**Medium: microSD over the SDIO/SDMMC peripheral** (not SPI - full-rate IMU/OMS/GPS
plus buffering needs the throughput).

**Offload path** Physically pulling the card works but scales badly with frequency (socket wear, lost/cracked cards). Primary offload should be over USB

- **USB mass-storage device port** - logger enumerates as a drive when a laptop is plugged
  in. Sweet spot: needs a USB-capable MCU and a reachable connector.
- **Card removal** - will always work

**Log file spec:** filesystem (FAT/exFAT), binary log format, file naming + rotation,
flush cadence, card-full behavior, and schema version embedded per file.

**Integrity:** power-fail detection plus hold-up capacitance to flush and cleanly close
the open file on ignition-off - prevents corruption

---

## RF Link

**Chosen: direct RF via the RFD900ux modem over UART**, in a license-free ISM band, for a
pit-adjacent receiver. Rationale over an LTE/cellular link:

- **Latency & determinism** - point-to-point, low latency, predictable jitter. LTE
  routes through the carrier core and internet: tens–hundreds of ms, variable, with
  occasional multi-second stalls on handoff/congestion.
- **Dependency & control** - RF needs only the two radios; LTE depends on track coverage,
  a data plan, and carrier health, none of which the team controls.

**Bandwidth budget drives everything.** RFD900ux net throughput is a fraction of its headline
air rate after protocol overhead (realistically tens of kbps). Enumerate desired channels
× sample rate × payload bytes and compare against that budget; the result defines the
prioritized live subset (which channels, what live rate). This priority scheme is itself a
requirement and constrains the frame format, the VCU CAN config, and the dashboard.

**Config to define:** band, air rate, TX power, net ID/addressing, antenna type / placement
/ polarization (car and pit), and range target. `SEQ #` already provides drop detection;

---

## Time Synchronization

Tighter than a plain RTC - fusion quality (GPS + IMU + OMS) dies if timestamps skew.

- All logged channels land on **one timebase**.
- Target **sub-millisecond relative accuracy** across IMU (100 Hz+), OMS/slip (similar),
  and GPS (1–10 Hz).
- **Discipline source:** VCU sends time-sync with LVMS enable

---

## Dashboard

- **Modular** - multiple views open simultaneously
- Built with **integration in mind** for a future in-house VCU - LiTel and sensor logs on
  one platform, connect to the car and record live *or* review previous logs (the
  Tune + i2 model).
- **Decodes via the shared schema**, not hardcoded offsets.
- **Records the live session to laptop disk** - a pit-side copy independent of the car's SD.
- Handles reconnection and time-aligns live vs. reviewed logs.

Not required to be polished or fully developed for PR-039 - just the basics needed while
still running the MoTeC VCU.

---

## Firmware Requirements

- **Concurrency:** ring buffers decoupling CAN RX → SD writer and CAN RX → RF TX; DMA on all
  three paths (task-per-path under an RTOS, or disciplined DMA + ISR).
- **Watchdog.**
- **SD-fault behavior:** defined response when the card is absent or fails (keep
  transmitting? halt?).
- **CAN robustness:** expected bus load, filtering (accept-all vs. mask), bus-off
  recovery, missing-message/timeout detection.
- **Heartbeat/status output** (LEDs and/or a status frame): logging? SD ok? RF linked? CAN
  alive?
- **Configurability:** change logged/transmitted channels via a config file on the card -
  no reflash.
- **Firmware update path** beyond SWD.

---

## Roadmap

**Year one (with MoTeC M150):**
- Logger PCB (STM32 + CAN + SDIO + RFD900 + fail-safe power).
- Full-rate SD logging with power-fail-safe file closing.
- Prioritized live RF stream + framed/CRC protocol.
- Synchronized timestamping.
- Basic modular dashboard: live view + log review, recorded to laptop disk.

**Later:**
- Track-map transmission from the SLAM stack (deltas + periodic resync).
- Logs as ground-truth/replay for the autonomous state estimator.
- Migration toward an in-house VCU with a unified tune + log platform.
- Optional LTE backhaul for remote viewing.
