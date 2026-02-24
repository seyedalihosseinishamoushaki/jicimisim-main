# jicimisim
This is the repository containing the code of the JIC IMI project.

# Grant-Free Scheduling for OpenAirInterface 5G NR

[![License](https://img.shields.io/badge/License-OAI%20Public%20License%20V1.1-blue.svg)](https://www.openairinterface.org/?page_id=698)
[![5G NR](https://img.shields.io/badge/5G-NR-green.svg)](https://www.3gpp.org/)
[![OAI](https://img.shields.io/badge/OpenAirInterface-5G-orange.svg)](https://openairinterface.org/)

Implementation of **Grant-Free (Configured Grant) Scheduling** for OpenAirInterface 5G NR, enabling ultra-low latency uplink transmissions for URLLC applications.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [File Structure](#file-structure)
- [API Reference](#api-reference)
- [Known Issues](#known-issues)

---

## Overview

### What is Grant-Free Scheduling?

In traditional 5G NR scheduling, User Equipment (UE) must request uplink resources through a multi-step process (SR → BSR → Grant → Data), adding 8-12ms of latency. **Grant-Free Scheduling** eliminates this overhead by allowing UEs to transmit on pre-configured resources without waiting for explicit grants.

```
Traditional Scheduling (8-12ms latency):
┌────┐                              ┌─────┐
│ UE │ ── SR ──────────────────────>│ gNB │
│    │ <─────────── UL Grant ───────│     │
│    │ ── BSR ─────────────────────>│     │
│    │ <─────────── UL Grant ───────│     │
│    │ ── PUSCH Data ──────────────>│     │
└────┘                              └─────┘

Grant-Free Scheduling (<1ms latency):
┌────┐                              ┌─────┐
│ UE │ ── PUSCH Data ──────────────>│ gNB │
│    │     (pre-configured)         │     │
│    │ <─────────── HARQ ACK ───────│     │
└────┘                              └─────┘
```

### Key Benefits

| Metric | Traditional | Grant-Free |
|--------|-------------|------------|
| UL Latency | 8-12 ms | **< 1 ms** |
| Signaling Overhead | High | **Minimal** |
| Use Cases | eMBB | **URLLC, IIoT, V2X** |

---

## Features

- ✅ **Autonomous UE Transmission** - No DCI grants required
- ✅ **Configurable Periodicity** - GF occasions every 1-65535 slots
- ✅ **Multiple UE Support** - Up to 32 UEs on gNB side
- ✅ **Multiple Configs per UE** - Up to 4 GF configurations per UE
- ✅ **Full HARQ Support** - ACK/NACK with retransmission handling
- ✅ **Coexistence** - Runs alongside normal PF scheduling
- ✅ **Standards Compliant** - Based on 3GPP TS 38.321, 38.214, 38.211

---

## Architecture

### End-to-End System

```
┌─────────────────────────────────────────────────────────────┐
│                         UE Side                              │
│  ┌─────────────┐    ┌──────────────────────────────────┐    │
│  │  RLC Layer  │───>│  GF Scheduler                    │    │
│  └─────────────┘    │  • Check GF occasion             │    │
│                     │  • Build MAC PDU (BSR + SDUs)    │    │
│                     │  • Configure PUSCH               │    │
│                     └──────────────┬───────────────────┘    │
│                                    ▼                         │
│                     ┌──────────────────────────────────┐    │
│                     │  ul_config_request (FAPI)        │    │
│                     └──────────────┬───────────────────┘    │
│                                    ▼                         │
│                     ┌──────────────────────────────────┐    │
│                     │  PHY Layer (PUSCH TX)            │    │
│                     └──────────────┬───────────────────┘    │
└────────────────────────────────────┼────────────────────────┘
                                     │ RF
┌────────────────────────────────────┼────────────────────────┐
│                     ┌──────────────▼───────────────────┐    │
│                     │  PHY Layer (PUSCH RX)            │    │
│                     └──────────────┬───────────────────┘    │
│                                    ▼                         │
│                     ┌──────────────────────────────────┐    │
│                     │  UL_tti_req_ahead (NFAPI)        │    │
│                     └──────────────┬───────────────────┘    │
│                                    ▼                         │
│  ┌─────────────┐    ┌──────────────────────────────────┐    │
│  │  RLC Layer  │<───│  GF Manager                      │    │
│  └─────────────┘    │  • Expect GF occasions           │    │
│                     │  • Schedule PUSCH reception      │    │
│                     │  • Process MAC PDUs              │    │
│                     │  • Send HARQ feedback            │    │
│                     └──────────────────────────────────┘    │
│                         gNB Side                             │
└─────────────────────────────────────────────────────────────┘
```

### GF Occasion Pattern

GF occasions occur based on the formula:
```
IS_GF_OCCASION = (absolute_slot >= offset) && ((absolute_slot - offset) % periodicity == 0)
```

Example with `periodicity=4, offset=1`:
```
Slot:  0   1   2   3   4   5   6   7   8   9   10  11  12
GF:        ✓           ✓           ✓           ✓
```

---

## Installation

### 1. Clone OpenAirInterface
```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
git checkout develop
source oaienv
```

### 2. Apply Grant-Free Patches

Copy the modified files to their respective locations:

```bash
# gNB files
cp gNB_scheduler_gf.c openair2/LAYER2/NR_MAC_gNB/
# (Also update nr_mac_gNB.h, mac_proto.h, gNB_scheduler.c, main.c)

# UE files  
# (Update nr_ue_scheduler.c, config_ue.c, mac_defs.h, mac_proto.h, main_ue_nr.c)
```

### 3. Update CMakeLists.txt

Add `gNB_scheduler_gf.c` to `openair2/LAYER2/NR_MAC_gNB/CMakeLists.txt`:
```cmake
add_library(nr_mac_gNB
  # ... existing sources ...
  gNB_scheduler_gf.c
)
```

### 4. Build
```bash
cd cmake_targets

# Build both gNB and UE
./build_oai -w USRP --gNB --ninja --nrUE -c

# Or build separately
./build_oai -w USRP --gNB -c --ninja
./build_oai -w USRP --nrUE -c --ninja
```

---

## Configuration

### ⚠️ Critical: UE and gNB Must Match

All GF parameters must be **identical** on both UE and gNB. Mismatches cause CRC errors and failed reception.

### Default Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `periodicity` | 4 | GF occasion every 4 slots |
| `offset` | 0 | Start at slot 0 |
| `start_symbol` | 2 | Start at OFDM symbol 2 |
| `nr_of_symbols` | 12 | Use 12 symbols |
| `rb_start` | 0 | Start at PRB 0 |
| `rb_size` | 10 | Use 10 PRBs |
| `mcs` | 9 | MCS index 9 (QPSK) |
| `mcs_table` | 0 | MCS Table 1 |
| `harq_process_id` | 0 | HARQ process 0 |
| `dmrs_config_type` | 0 | DMRS Type 1 |
| `dmrs_ports` | 1 | Port 0 |

### Custom Configuration (UE)

```c
nr_gf_config_t gf_params = {0};

// Time domain
gf_params.periodicity = 4;
gf_params.offset = 0;
gf_params.start_symbol = 2;
gf_params.nr_of_symbols = 12;
gf_params.mapping_type = 0;  // Type A

// Frequency domain
gf_params.rb_start = 0;
gf_params.rb_size = 10;
gf_params.frequency_hopping = 0;

// MCS
gf_params.mcs = 9;
gf_params.mcs_table = 0;

// HARQ
gf_params.harq_process_id = 0;
gf_params.rv_sequence[0] = 0;
gf_params.rv_sequence[1] = 2;
gf_params.rv_sequence[2] = 3;
gf_params.rv_sequence[3] = 1;

// DMRS
gf_params.dmrs_config_type = 0;
gf_params.dmrs_ports = 1;
gf_params.num_dmrs_cdm_grps_no_data = 2;

// Apply configuration
nr_ue_configure_grant_free(mac, &gf_params);
```

### Custom Configuration (gNB)

```c
nr_gf_gnb_ue_config_t gf_config = {0};

// Must match UE configuration exactly!
gf_config.rnti = ue_rnti;
gf_config.periodicity = 4;
gf_config.offset = 0;
gf_config.start_symbol = 2;
gf_config.nr_of_symbols = 12;
gf_config.rb_start = 0;
gf_config.rb_size = 10;
gf_config.mcs = 9;
gf_config.mcs_table = 0;
gf_config.harq_process_id = 0;
gf_config.dmrs_config_type = 0;
gf_config.dmrs_ports = 1;

nr_gf_gnb_configure_ue(gNB, ue_id, ue_rnti, &gf_config);
```

---

## Usage

### 1. Start gNB
Start the gNB using the webpage of the OAIBOX MAX.

### 2. Start UE
```bash
cd openairinterface5g/cmake_targets/ran_build/build

clear && sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 77 -C 4019160000 --ssb 144 -E --continuous-tx --ue-fo-compensation --usrp-args "serial=326F0F9" --ue-txgain 0 --ue-rxgain 76
```

### 3. Wait for Connection
Monitor logs until UE reaches `UE_CONNECTED` state.

### 4. Enable Grant-Free
GF is automatically initialized after UE connection. 

---

## File Structure

```
openair2/LAYER2/
├── NR_MAC_gNB/
│   ├── gNB_scheduler_gf.c      # NEW: GF scheduler implementation
│   ├── nr_mac_gNB.h            # MODIFIED: GF data structures
│   ├── mac_proto.h             # MODIFIED: GF function declarations
│   ├── gNB_scheduler.c         # MODIFIED: Integration hook
│   └── main.c                  # MODIFIED: GF initialization
│
└── NR_MAC_UE/
    ├── nr_ue_scheduler.c       # MODIFIED: GF scheduling functions
    ├── config_ue.c             # MODIFIED: GF configuration
    ├── mac_defs.h              # MODIFIED: GF data structures
    ├── mac_proto.h             # MODIFIED: GF function declarations
    └── main_ue_nr.c            # MODIFIED: GF initialization
```

---

## API Reference

### UE Functions

| Function | Description |
|----------|-------------|
| `nr_ue_configure_grant_free()` | Configure GF parameters |
| `nr_ue_init_default_grant_free()` | Initialize with default config |
| `nr_ue_release_grant_free()` | Release GF configuration |
| `nr_ue_check_grant_free_occasion()` | Check if slot is GF occasion |
| `nr_ue_schedule_grant_free()` | Main GF scheduling function |
| `nr_config_pusch_pdu_grant_free()` | Configure PUSCH for GF |
| `nr_ue_gf_build_mac_pdu()` | Build MAC PDU with BSR/SDUs |
| `nr_ue_gf_process_harq_feedback()` | Handle HARQ ACK/NACK |

### gNB Functions

| Function | Description |
|----------|-------------|
| `nr_gf_gnb_init()` | Initialize GF manager |
| `nr_gf_gnb_configure_ue()` | Configure GF for a UE |
| `nr_gf_gnb_configure_default()` | Configure with defaults |
| `nr_gf_gnb_release_ue()` | Release UE GF config |
| `nr_gf_gnb_slot_process()` | Main slot processing |
| `nr_gf_gnb_check_occasions()` | Check for GF occasions |
| `nr_gf_gnb_schedule_reception()` | Schedule PUSCH RX |
| `nr_gf_gnb_print_stats()` | Print statistics |

---

### Statistics

```c
// gNB statistics
nr_gf_gnb_print_stats(gNB);

// Output:
// ========== Grant-Free Statistics (gNB) ==========
// Global: enabled=1, UEs=1
// Occasions: 1000, Receptions: 950, Successes: 920 (96.8%)
// UE 0 (RNTI 0x21B4):
//   RX: 920 success, 30 fail, 184000 bytes
//   HARQ: 920 ACK, 30 NACK
// ==================================================
```

---

## Known Issues

### 1. USRP Operating on USB 2.0 Instead of USB 3.0

**Symptom:** Reduced throughput, sample drops, underruns

**Diagnostic:**
```bash
lsusb -t | grep -i usb
uhd_find_devices
```

**Possible causes:**
- USB cable not rated for USB 3.0 SuperSpeed
- Connected to USB 2.0 port
- Missing USB 3.0 drivers
- BIOS USB 3.0 disabled

### 2. Invalid MAC PDU Parsing Error

**Symptom:**
```
[NR_MAC] Invalid PDU in 980.3 for RNTI 21B4! 
        mac_subheader_len: 1, mac_len: 6, remaining pdu_len: 3
```

**Analysis:** Subheader indicates 6 bytes payload but only 3 bytes remain

**Possible causes:**
- Incorrect subheader type selection
- Length field byte order issue (use `htons()` for 16-bit)
- TBS mismatch between UE and gNB
- MAC PDU buffer corruption

