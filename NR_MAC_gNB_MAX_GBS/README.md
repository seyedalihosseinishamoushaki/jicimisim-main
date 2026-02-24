# Grant-Based Scheduling in OpenAirInterface - gNB Side

## Overview

Grant-Based scheduling is the standard uplink scheduling mechanism in 5G NR. The gNodeB maintains full control over uplink resource allocation, with UEs requesting and receiving explicit grants before transmitting data.

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
| `gNB_scheduler.c` | Main scheduler entry point and slot processing |
| `gNB_scheduler_ulsch.c` | Uplink shared channel (PUSCH) scheduling |
| `gNB_scheduler_dlsch.c` | Downlink shared channel (PDSCH) scheduling |
| `gNB_scheduler_primitives.c` | Helper functions and resource management |
| `gNB_scheduler_bch.c` | Broadcast channel scheduling |
| `nr_mac_gNB.h` | Data structures and definitions |

## Scheduler Architecture

### Main Entry Point

The scheduler runs every slot via `gNB_dlsch_ulsch_scheduler()` in `gNB_scheduler.c`:

```c
void gNB_dlsch_ulsch_scheduler(module_id_t module_id,
                                frame_t frame,
                                sub_frame_t slot);
```

### Scheduling Algorithm

OAI implements a **Proportional Fair (PF)** scheduler that balances:
- Instantaneous channel quality (CQI)
- Historical throughput (fairness)

## Key Data Structures

### UE Information
```c
typedef struct NR_UE_info_t {
    rnti_t      rnti;
    NR_UE_sched_ctrl_t  UE_sched_ctrl;
    NR_CellGroupConfig_t *CellGroup;
    // ... additional fields
} NR_UE_info_t;
```

### Scheduling Control
```c
typedef struct NR_UE_sched_ctrl_t {
    NR_UE_ul_sched_ctrl_t ul_sched_ctrl;
    NR_UE_dl_sched_ctrl_t dl_sched_ctrl;
    int harq_processes[16];
    // ... additional fields
} NR_UE_sched_ctrl_t;
```

## Uplink Scheduling Process

### 1. Buffer Status Report (BSR) Processing

UEs report buffer status via MAC CE:

```c
// BSR types (3GPP TS 38.321)
- Short BSR:      1 LCG, truncated buffer size
- Long BSR:       Multiple LCGs, full reporting
- Short Truncated BSR
- Long Truncated BSR
```

### 2. Resource Allocation

The scheduler determines:
- **PRB allocation**: Which Physical Resource Blocks to assign
- **MCS selection**: Based on CQI and BLER targets
- **TBS calculation**: Transport Block Size from PRBs and MCS
- **HARQ process**: Which HARQ process ID to use

### 3. DCI Generation

Downlink Control Information (DCI) Format 0_1 fields:
- Frequency domain resource assignment
- Time domain resource assignment
- MCS index
- HARQ process number
- New data indicator (NDI)
- Redundancy version (RV)

## Configuration Parameters

Key RRC/MAC parameters affecting scheduling:

| Parameter | Description |
|-----------|-------------|
| `initialUplinkBWP` | Initial uplink bandwidth part configuration |
| `pusch-Config` | PUSCH transmission parameters |
| `schedulingRequestConfig` | SR periodicity and resources |
| `bsr-Config` | BSR timers and triggers |

## HARQ Management

OAI supports up to 16 HARQ processes for uplink:

```c
#define NR_MAX_HARQ_PROCESSES 16

// HARQ states
- IDLE:     Available for new transmission
- WAITING:  Awaiting transmission/feedback
- NACK:     Retransmission required
```

