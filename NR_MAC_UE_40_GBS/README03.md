# Grant-Based Scheduling in OpenAirInterface - UE Side

## Overview

Grant-Based scheduling is the standard uplink scheduling mechanism in 5G NR. The UE requests uplink resources from the gNodeB and waits for an explicit grant before transmitting data.

### Scheduling Flow

```
UE                                          gNB
 |                                           |
 |  1. Scheduling Request (SR) via PUCCH     |
 |  ---------------------------------------->|
 |                                           |
 |  2. Uplink Grant via PDCCH (DCI 0_1)      |
 |  <----------------------------------------|
 |                                           |
 |  3. Data Transmission via PUSCH           |
 |  ---------------------------------------->|
 |                                           |
 |  4. HARQ Feedback (ACK/NACK)              |
 |  <----------------------------------------|
```

## Key Files

| File | Purpose |
|------|---------|
| `nr_ue_scheduler.c` | Main UE MAC scheduler |
| `nr_ue_procedures.c` | UE MAC procedures and PDU handling |
| `nr_ue_dci_configuration.c` | DCI processing and configuration |
| `mac_defs.h` | MAC layer definitions |
| `mac_proto.h` | Function prototypes |

## UE Scheduler Architecture

### Main Entry Point

The UE scheduler runs every slot via `nr_ue_ul_scheduler()` in `nr_ue_scheduler.c`:

```c
void nr_ue_ul_scheduler(NR_UE_MAC_INST_t *mac,
                        nr_uplink_indication_t *ul_info);
```

### Processing Chain

1. **DCI Reception** — Parse uplink grants from PDCCH
2. **Grant Processing** — Extract resource allocation parameters
3. **MAC PDU Construction** — Build PDU with headers and data
4. **PUSCH Transmission** — Send via physical layer

## Key Data Structures

### UE MAC Instance
```c
typedef struct NR_UE_MAC_INST_s {
    NR_UE_L2_STATE_t state;
    NR_UE_Scheduling_Info_t scheduling_info;
    NR_UE_HARQ_t harq[NR_MAX_HARQ_PROCESSES];
    // ... additional fields
} NR_UE_MAC_INST_t;
```

### Scheduling Information
```c
typedef struct NR_UE_Scheduling_Info_s {
    uint32_t BSR[NR_MAX_NUM_LCGID];
    uint8_t  BSR_reporting_active;
    // ... additional fields
} NR_UE_Scheduling_Info_t;
```

## Uplink Transmission Process

### 1. Scheduling Request (SR)

When data arrives and no grant is available:

```c
// SR trigger conditions (3GPP TS 38.321)
- Regular BSR triggered
- No UL grant available
- SR prohibit timer expired
```

SR is transmitted on configured PUCCH resources.

### 2. Grant Reception

DCI Format 0_1 provides:
- Frequency domain resource assignment
- Time domain resource assignment
- MCS index
- HARQ process number
- New data indicator (NDI)
- Redundancy version (RV)

### 3. MAC PDU Construction

The UE builds MAC PDUs using standard structures:

```c
// MAC subheader types
NR_MAC_SUBHEADER_FIXED   // Fixed-size MAC CE
NR_MAC_SUBHEADER_SHORT   // Variable size, L < 256 bytes
NR_MAC_SUBHEADER_LONG    // Variable size, L >= 256 bytes
```

### MAC PDU Format
```
+----------------+----------------+----------------+
| Subheader 1    | Subheader 2    | Subheader N    |
+----------------+----------------+----------------+
| MAC CE / SDU 1 | MAC CE / SDU 2 | Padding        |
+----------------+----------------+----------------+
```

### 4. Buffer Status Report (BSR)

BSR informs gNB about pending data:

| BSR Type | LCID | Description |
|----------|------|-------------|
| Short BSR | 61 | Single LCG reporting |
| Long BSR | 62 | Multiple LCG reporting |
| Short Truncated BSR | 63 | Truncated single LCG |
| Long Truncated BSR | 64 | Truncated multiple LCG |

```c
// BSR index lookup (3GPP TS 38.321 Table 6.1.3.1-1)
uint8_t nr_locate_BsrIndexByBufferSize(uint32_t bufferSize);
```

## HARQ Management

UE maintains up to 16 HARQ processes:

```c
#define NR_MAX_HARQ_PROCESSES 16

typedef struct NR_UE_HARQ_s {
    uint8_t  harq_id;
    uint8_t  ndi;
    uint8_t  round;
    // ... additional fields
} NR_UE_HARQ_t;
```

### HARQ Operation
- **New transmission**: NDI toggles, RV = 0
- **Retransmission**: NDI unchanged, RV cycles through {0, 2, 3, 1}

