# Grant-Free Scheduling - UE Implementation
## OpenAirInterface 5G NR User Equipment

[![License](https://img.shields.io/badge/License-OAI%20Public%20License%20V1.1-blue.svg)](https://www.openairinterface.org/?page_id=698)
[![5G NR](https://img.shields.io/badge/5G-NR-green.svg)](https://www.3gpp.org/)
[![OAI](https://img.shields.io/badge/OpenAirInterface-nrUE-orange.svg)](https://openairinterface.org/)

UE-side implementation of **Grant-Free (Configured Grant) Scheduling** for OpenAirInterface 5G NR, enabling ultra-low latency uplink transmissions without waiting for dynamic grants.

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [MAC PDU Construction](#mac-pdu-construction)
- [HARQ Management](#harq-management)
- [Monitoring & Debugging](#monitoring--debugging)
- [Known Issues](#known-issues)
- [File Structure](#file-structure)
- [License](#license)

---

## Overview

### What is Grant-Free Scheduling?

Grant-Free scheduling allows the UE to transmit uplink data on **pre-configured resources** without requesting grants from the gNB. This eliminates the SR → BSR → Grant signaling overhead, reducing latency from 8-12ms to less than 1ms.

```
Traditional UL Transmission:              Grant-Free UL Transmission:
                                         
UE        gNB                            UE        gNB
 │         │                              │         │
 │── SR ──>│  ┐                           │         │
 │<─ Grant─│  │                           │══ DATA ═>│  (pre-configured)
 │── BSR ─>│  ├─ 8-12ms                   │<─ ACK ──│
 │<─ Grant─│  │                           │         │
 │══ DATA ═>│  ┘                           │         │
 │         │                              │         │
                                         
      Latency: 8-12ms                         Latency: <1ms
```

### GF Occasion Formula

The UE transmits on Grant-Free occasions determined by:

```
IS_GF_OCCASION = (absolute_slot >= offset) && ((absolute_slot - offset) % periodicity == 0)

where: absolute_slot = frame × slots_per_frame + slot
```

**Example** (`periodicity=4, offset=1`):
```
Slot:   0   1   2   3   4   5   6   7   8   9   10  11  12
GF:         ✓           ✓           ✓           ✓
```

---

## Features

- ✅ **Autonomous Transmission** - Transmit without DCI grants
- ✅ **Configurable Periodicity** - 1 to 65535 slots
- ✅ **Multiple Configurations** - Up to 4 GF configs per UE
- ✅ **MAC PDU Construction** - Proper BSR, SDU multiplexing, padding
- ✅ **HARQ Support** - NDI toggle, RV sequence, retransmissions
- ✅ **No-Data Behavior** - Skip, send BSR, or send padding
- ✅ **Statistics Tracking** - TX count, success rate, bytes sent
- ✅ **Coexistence** - Works alongside normal DCI-based scheduling

---

## Architecture

### UE Grant-Free Scheduler

```
┌─────────────────────────────────────────────────────────────────┐
│                      NR_UE_MAC_INST                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───────────────────┐    ┌─────────────────────────────────┐  │
│  │  Normal Scheduler │    │      Grant-Free Scheduler       │  │
│  │  (DCI-based)      │    │  ┌───────────────────────────┐  │  │
│  │                   │    │  │ GF Configurations         │  │  │
│  │  • Process DCI    │    │  │ (nr_gf_config_t[4])       │  │  │
│  │  • DCI 0_0/0_1    │    │  └───────────────────────────┘  │  │
│  └─────────┬─────────┘    │  ┌───────────────────────────┐  │  │
│            │              │  │ Occasion Checker          │  │  │
│            │              │  │ • Check periodicity/offset│  │  │
│            │              │  └───────────────────────────┘  │  │
│            │              │  ┌───────────────────────────┐  │  │
│            │              │  │ MAC PDU Builder           │  │  │
│            │              │  │ • BSR insertion           │  │  │
│            │              │  │ • SDU multiplexing        │  │  │
│            │              │  │ • Padding                 │  │  │
│            │              │  └───────────────────────────┘  │  │
│            │              │  ┌───────────────────────────┐  │  │
│            │              │  │ HARQ Handler              │  │  │
│            │              │  │ • ACK/NACK processing     │  │  │
│            │              │  │ • NDI toggle              │  │  │
│            │              │  │ • RV sequence             │  │  │
│            │              │  └───────────────────────────┘  │  │
│            │              └────────────────┬────────────────┘  │
│            │                               │                    │
│            ▼                               ▼                    │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                 ul_config_request (FAPI)                  │  │
│  │                   PUSCH PDUs for PHY                      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   PHY Layer (PUSCH)   │
                    └───────────────────────┘
```

### Slot Processing Flow

```
nr_ue_ul_scheduler()
        │
        ▼
┌───────────────────────────────────────┐
│  nr_ue_schedule_grant_free()          │
│  1. Check GF enabled & UE connected   │
│  2. nr_ue_check_grant_free_occasion() │
│  3. If GF occasion:                   │
│     a. Check pending data             │
│     b. Allocate ul_config PDU         │
│     c. nr_config_pusch_pdu_grant_free │
│     d. nr_ue_gf_build_mac_pdu         │
│     e. Update HARQ buffer             │
│     f. Update statistics              │
└───────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────┐
│  Normal UL Scheduling (DCI-based)     │
└───────────────────────────────────────┘
```

---

## Requirements

### Software
- Ubuntu 20.04 / 22.04 LTS
- OpenAirInterface 5G NR (develop branch)
- UHD 4.0+ (for USRP)
- GCC 9+ or Clang 10+

### Hardware
- USRP B200/B210 or X300/X310
- USB 3.0 connection (required for B2xx)

---

## Installation

### 1. Clone OAI and Apply Modifications

```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
git checkout develop
source oaienv
```

### 2. Modified Files

Copy the following modified files to `openair2/LAYER2/NR_MAC_UE/`:

| File | Changes |
|------|---------|
| `mac_defs.h` | Added `nr_gf_config_t`, `nr_gf_statistics_t`, GF fields in `NR_UE_MAC_INST_t` |
| `mac_proto.h` | Added GF function declarations |
| `nr_ue_scheduler.c` | Added GF scheduling, MAC PDU building, HARQ handling |
| `config_ue.c` | Added `nr_ue_configure_grant_free()`, `nr_ue_init_default_grant_free()` |
| `main_ue_nr.c` | Added GF initialization in `nr_ue_init_mac()` |

### 3. Build

```bash
cd cmake_targets
./build_oai -w USRP --nrUE -c
```

---

## Configuration

### Data Structures

#### GF Configuration (`nr_gf_config_t`)

```c
typedef struct nr_gf_config {
  // State
  bool enabled;              // Configuration valid
  bool active;               // Currently active
  
  // Time Domain
  uint16_t periodicity;      // Period in slots (1-65535)
  uint16_t offset;           // Slot offset (0 to periodicity-1)
  uint8_t start_symbol;      // Starting symbol (0-13)
  uint8_t nr_of_symbols;     // Number of symbols (1-14)
  uint8_t mapping_type;      // 0=Type A, 1=Type B
  
  // Frequency Domain
  uint16_t rb_start;         // Starting PRB
  uint16_t rb_size;          // Number of PRBs
  uint8_t frequency_hopping; // 0=disabled
  
  // MCS
  uint8_t mcs;               // MCS index (0-28)
  uint8_t mcs_table;         // 0=Table1, 1=Table2, 2=Table3
  
  // HARQ
  uint8_t harq_process_id;   // HARQ process (0-15)
  uint8_t rv_sequence[4];    // RV sequence {0,2,3,1}
  uint8_t ndi_toggle;        // Current NDI state
  uint8_t max_retransmissions;
  
  // DMRS
  uint8_t dmrs_config_type;  // 0=Type1, 1=Type2
  uint8_t dmrs_ports;        // Port bitmap
  uint16_t dmrs_scrambling_id;
  uint8_t num_dmrs_cdm_grps_no_data;
  
  // Behavior
  uint8_t no_data_behavior;  // GF_NO_DATA_SKIP/SEND_BSR/SEND_PADDING
  
  // RNTI (0 = use mac->crnti)
  uint16_t rnti;
  
  // Statistics
  uint32_t tx_count;
  uint32_t tx_with_data;
  uint32_t successful_tx_count;
  uint32_t failed_tx_count;
  // ...
} nr_gf_config_t;
```

#### No-Data Behavior

```c
typedef enum {
  GF_NO_DATA_SKIP = 0,        // Don't transmit if no data (default)
  GF_NO_DATA_SEND_BSR = 1,    // Send BSR only
  GF_NO_DATA_SEND_PADDING = 2 // Send padding
} gf_no_data_behavior_t;
```

### Default Configuration

```c
// Applied by nr_ue_init_default_grant_free()
nr_gf_config_t defaults = {
  .periodicity = 4,
  .offset = 0,
  .start_symbol = 2,
  .nr_of_symbols = 12,
  .mapping_type = 0,          // Type A
  .rb_start = 0,
  .rb_size = 10,
  .frequency_hopping = 0,
  .mcs = 9,
  .mcs_table = 0,
  .harq_process_id = 0,
  .rv_sequence = {0, 2, 3, 1},
  .dmrs_config_type = 0,      // Type 1
  .dmrs_ports = 1,            // Port 0
  .num_dmrs_cdm_grps_no_data = 2,
  .no_data_behavior = GF_NO_DATA_SKIP,
};
```

### Custom Configuration Example

```c
void configure_custom_gf(NR_UE_MAC_INST_t *mac) {
  nr_gf_config_t gf = {0};
  
  // Time: every 8 slots, starting at slot 2
  gf.periodicity = 8;
  gf.offset = 2;
  gf.start_symbol = 2;
  gf.nr_of_symbols = 12;
  gf.mapping_type = 0;
  
  // Frequency: 20 PRBs starting at PRB 10
  gf.rb_start = 10;
  gf.rb_size = 20;
  
  // MCS 12 for higher throughput
  gf.mcs = 12;
  gf.mcs_table = 0;
  
  // HARQ process 1
  gf.harq_process_id = 1;
  gf.rv_sequence[0] = 0;
  gf.rv_sequence[1] = 2;
  gf.rv_sequence[2] = 3;
  gf.rv_sequence[3] = 1;
  
  // DMRS
  gf.dmrs_config_type = 0;
  gf.dmrs_ports = 1;
  gf.num_dmrs_cdm_grps_no_data = 2;
  
  // Use C-RNTI
  gf.rnti = 0;  // Will use mac->crnti
  
  // Apply
  int ret = nr_ue_configure_grant_free(mac, &gf);
  if (ret == 0) {
    LOG_I(NR_MAC, "GF configuration applied successfully\n");
  }
}
```

---

## API Reference

### Configuration Functions

| Function | Description |
|----------|-------------|
| `int nr_ue_configure_grant_free(mac, gf_params)` | Apply GF configuration |
| `void nr_ue_init_default_grant_free(mac)` | Initialize with defaults |
| `int nr_ue_release_grant_free(mac, config_index)` | Release configuration |

### Scheduling Functions

| Function | Description |
|----------|-------------|
| `nr_gf_config_t* nr_ue_check_grant_free_occasion(mac, frame, slot)` | Check if slot is GF occasion |
| `void nr_ue_schedule_grant_free(mac, frame, slot)` | Main GF scheduler |
| `int nr_config_pusch_pdu_grant_free(mac, gf, pusch_pdu, frame, slot)` | Configure PUSCH PDU |

### MAC PDU Functions

| Function | Description |
|----------|-------------|
| `uint32_t nr_ue_gf_check_pending_data(mac)` | Check pending data in RLC |
| `int nr_ue_gf_build_mac_pdu(mac, gf, pdu, pdu_size)` | Build MAC PDU |

### HARQ Functions

| Function | Description |
|----------|-------------|
| `void nr_ue_gf_process_harq_feedback(mac, harq_pid, ack)` | Process ACK/NACK |
| `void nr_ue_gf_handle_dtx(mac, harq_pid)` | Handle DTX (no feedback) |

### Statistics Functions

| Function | Description |
|----------|-------------|
| `int nr_ue_get_grant_free_stats(mac, config_index, stats)` | Get per-config stats |
| `void nr_ue_gf_get_statistics(mac, stats)` | Get global stats |
| `void nr_ue_gf_reset_statistics(mac)` | Reset statistics |

---

## MAC PDU Construction

### PDU Structure

```
┌──────────────────────────────────────────────────────────────┐
│                         MAC PDU                               │
├──────────────┬──────────────┬─────────────┬─────────────────┤
│ BSR subPDU   │ SDU subPDU   │ SDU subPDU  │ Padding subPDU  │
│ (optional)   │ (LCID 1-32)  │    ...      │ (LCID 0x3F)     │
└──────────────┴──────────────┴─────────────┴─────────────────┘
```

### Subheader Formats

**Fixed (1 byte)** - Used for BSR, C-RNTI, Padding:
```
┌───┬───┬─────────────────┐
│ R │ R │      LCID       │
└───┴───┴─────────────────┘
  0   1   2             7
```

**Short (2 bytes)** - SDU ≤ 255 bytes:
```
┌───┬───┬─────────────────┬─────────────────────────┐
│ R │ F │      LCID       │           L             │
└───┴───┴─────────────────┴─────────────────────────┘
  0   1   2             7   8                     15
```

**Long (3 bytes)** - SDU > 255 bytes:
```
┌───┬───┬─────────────────┬───────────────────────────────────────────────┐
│ R │ F │      LCID       │                    L (16 bits)                │
└───┴───┴─────────────────┴───────────────────────────────────────────────┘
  0   1   2             7   8                                           23
```

### Important LCID Values

| LCID | Description |
|------|-------------|
| 1-32 | Data logical channels |
| 0x3D (61) | Short BSR |
| 0x3E (62) | Long BSR |
| 0x3F (63) | Padding |

### MAC PDU Building Rules

1. **Use OAI structures**: `NR_MAC_SUBHEADER_FIXED`, `NR_MAC_SUBHEADER_SHORT`, `NR_MAC_SUBHEADER_LONG`, `NR_BSR_SHORT`
2. **Network byte order**: Use `htons()` for 16-bit length fields
3. **Single padding subheader**: Only one 0x3F LCID, rest is filler bytes
4. **BSR payload**: Short BSR is 1 byte (LcgID + Buffer_size)

---

## HARQ Management

### NDI Toggle

```c
// New transmission: toggle NDI
gf->ndi_toggle = !gf->ndi_toggle;
harq->last_ndi = gf->ndi_toggle;
harq->round = 0;

// Retransmission: keep same NDI
// NDI unchanged, increment round
harq->round++;
```

### RV Sequence

Standard 3GPP sequence: `{0, 2, 3, 1}`

```c
uint8_t rv = gf->rv_sequence[harq->round % 4];

// Round 0: RV=0 (initial transmission)
// Round 1: RV=2 (1st retransmission)
// Round 2: RV=3 (2nd retransmission)
// Round 3: RV=1 (3rd retransmission)
```

### HARQ Feedback Processing

```c
void nr_ue_gf_process_harq_feedback(mac, harq_pid, ack) {
  if (ack) {
    // Success
    gf->ndi_toggle = !gf->ndi_toggle;
    harq->round = 0;
    free(harq->tx_buffer);
    gf->successful_tx_count++;
  } else {
    // NACK - schedule retransmission
    harq->round++;
    if (harq->round >= gf->max_retransmissions) {
      // Give up
      gf->ndi_toggle = !gf->ndi_toggle;
      harq->round = 0;
      free(harq->tx_buffer);
      gf->failed_tx_count++;
    }
  }
}
```


## File Structure

```
openair2/LAYER2/NR_MAC_UE/
├── mac_defs.h           # MODIFIED
│   ├── nr_gf_config_t           (GF configuration structure)
│   ├── nr_gf_statistics_t       (Statistics structure)
│   ├── gf_no_data_behavior_t    (Enum for no-data behavior)
│   └── NR_UE_MAC_INST_t         (Added gf_config[], gf_enabled, etc.)
│
├── mac_proto.h          # MODIFIED
│   ├── nr_ue_configure_grant_free()
│   ├── nr_ue_init_default_grant_free()
│   ├── nr_ue_release_grant_free()
│   ├── nr_ue_check_grant_free_occasion()
│   ├── nr_ue_schedule_grant_free()
│   ├── nr_config_pusch_pdu_grant_free()
│   ├── nr_ue_gf_build_mac_pdu()
│   ├── nr_ue_gf_process_harq_feedback()
│   └── nr_ue_gf_handle_dtx()
│
├── nr_ue_scheduler.c    # MODIFIED
│   ├── nr_ue_gf_check_pending_data()
│   ├── nr_ue_gf_build_mac_pdu()
│   ├── nr_ue_check_grant_free_occasion()
│   ├── nr_config_pusch_pdu_grant_free()
│   ├── nr_ue_schedule_grant_free()
│   ├── nr_ue_gf_process_harq_feedback()
│   └── nr_ue_gf_handle_dtx()
│
├── config_ue.c          # MODIFIED
│   ├── nr_ue_configure_grant_free()
│   ├── nr_ue_init_default_grant_free()
│   └── nr_ue_release_grant_free()
│
└── main_ue_nr.c         # MODIFIED
    ├── nr_ue_init_mac()         (GF initialization)
    ├── reset_mac_inst()         (GF state reset)
    └── release_mac_configuration() (GF release)
```

---

## Configuration Matching

⚠️ **Critical**: UE configuration must **exactly match** gNB configuration!

| Parameter | Must Match |
|-----------|------------|
| periodicity | ✓ |
| offset | ✓ |
| start_symbol | ✓ |
| nr_of_symbols | ✓ |
| rb_start | ✓ |
| rb_size | ✓ |
| mcs | ✓ |
| mcs_table | ✓ |
| harq_process_id | ✓ |
| dmrs_config_type | ✓ |
| dmrs_ports | ✓ |
| rnti | ✓ |

Mismatched configurations will cause CRC errors and failed reception at gNB.

