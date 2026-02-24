/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/* \file mac_defs.h
 * \brief MAC data structures and constants
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_MAC_DEFS_H__
#define __LAYER2_NR_MAC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/platform_types.h"

/* IF */
#include "NR_IF_Module.h"
#include "fapi_nr_ue_interface.h"

/* MAC */
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "mac_defs_sl.h"

/* RRC */
#include "NR_DRX-Config.h"
#include "NR_SchedulingRequestConfig.h"
#include "NR_BSR-Config.h"
#include "NR_TAG-Config.h"
#include "NR_PHR-Config.h"
#include "NR_RNTI-Value.h"
#include "NR_MIB.h"
#include "NR_MAC-CellGroupConfig.h"
#include "NR_PhysicalCellGroupConfig.h"
#include "NR_CellGroupConfig.h"
#include "NR_ServingCellConfig.h"
#include "NR_MeasConfig.h"
#include "NR_ServingCellConfigCommonSIB.h"

// ==========
// NR UE defs
// ==========

#define NR_BSR_TRIGGER_NONE      (0) /* No BSR Trigger */
#define NR_BSR_TRIGGER_REGULAR   (1) /* For Regular and ReTxBSR Expiry Triggers */
#define NR_BSR_TRIGGER_PERIODIC (2) /* For BSR Periodic Timer Expiry Trigger */

#define NR_INVALID_LCGID (NR_MAX_NUM_LCGID)

#define MAX_NUM_BWP_UE 5
#define NR_MAX_SR_ID 8  // SchedulingRequestId ::= INTEGER (0..7)

/*!\brief value for indicating BSR Timer is not running */
#define NR_MAC_UE_BSR_TIMER_NOT_RUNNING   (0xFFFF)

#define MAX_NB_SSB (64) // Maximum number of possible SSB indexes

// ===============
// DCI fields defs
// ===============

#define NBR_NR_FORMATS                   8     // The number of formats is 8 (0_0, 0_1, 1_0, 1_1, 2_0, 2_1, 2_2, 2_3)
#define NBR_NR_DCI_FIELDS               56    // The number of different dci fields defined in TS 38.212 subclause 7.3.1

#define IDENTIFIER_DCI_FORMATS           0
#define CARRIER_IND                      1
#define SUL_IND_0_1                      2
#define SLOT_FORMAT_IND                  3
#define PRE_EMPTION_IND                  4
#define BLOCK_NUMBER                     5
#define CLOSE_LOOP_IND                   6
#define BANDWIDTH_PART_IND               7
#define SHORT_MESSAGE_IND                8
#define SHORT_MESSAGES                   9
#define FREQ_DOM_RESOURCE_ASSIGNMENT_UL 10
#define FREQ_DOM_RESOURCE_ASSIGNMENT_DL 11
#define TIME_DOM_RESOURCE_ASSIGNMENT    12
#define VRB_TO_PRB_MAPPING              13
#define PRB_BUNDLING_SIZE_IND           14
#define RATE_MATCHING_IND               15
#define ZP_CSI_RS_TRIGGER               16
#define FREQ_HOPPING_FLAG               17
#define TB1_MCS                         18
#define TB1_NDI                         19
#define TB1_RV                          20
#define TB2_MCS                         21
#define TB2_NDI                         22
#define TB2_RV                          23
#define MCS                             24
#define NDI                             25
#define RV                              26
#define HARQ_PROCESS_NUMBER             27
#define DAI_                            28
#define FIRST_DAI                       29
#define SECOND_DAI                      30
#define TB_SCALING                      31
#define TPC_PUSCH                       32
#define TPC_PUCCH                       33
#define PUCCH_RESOURCE_IND              34
#define PDSCH_TO_HARQ_FEEDBACK_TIME_IND 35
#define SRS_RESOURCE_IND                36
#define PRECOD_NBR_LAYERS               37
#define ANTENNA_PORTS                   38
#define TCI                             39
#define SRS_REQUEST                     40
#define TPC_CMD                         41
#define CSI_REQUEST                     42
#define CBGTI                           43
#define CBGFI                           44
#define PTRS_DMRS                       45
#define BETA_OFFSET_IND                 46
#define DMRS_SEQ_INI                    47
#define UL_SCH_IND                      48
#define PADDING_NR_DCI                  49
#define SUL_IND_0_0                     50
#define RA_PREAMBLE_INDEX               51
#define SUL_IND_1_0                     52
#define SS_PBCH_INDEX                   53
#define PRACH_MASK_INDEX                54
#define RESERVED_NR_DCI                 55

// Define the UE L2 states with X-Macro
#define NR_UE_L2_STATES \
  UE_STATE(UE_NOT_SYNC) \
  UE_STATE(UE_BARRED) \
  UE_STATE(UE_RECEIVING_SIB) \
  UE_STATE(UE_PERFORMING_RA) \
  UE_STATE(UE_CONNECTED) \
  UE_STATE(UE_DETACHING)

// ===============================================
// SSB to RO mapping public defines and structures
// ===============================================
#define MAX_SSB_PER_RO (16) // Maximum number of SSBs that can be mapped to a single RO
#define MAX_TDM (7) // Maximum nb of PRACH occasions TDMed in a slot
#define MAX_FDM (8) // Maximum nb of PRACH occasions FDMed in a slot


// Maximum number of GF configurations per UE
// Allows flexibility for different traffic types (e.g., periodic sensor data, VoIP)
#define NR_MAX_GF_CONFIGS 4

// Grant-Free HARQ process range (can use subset of available processes)
#define NR_GF_HARQ_PROCESS_START 0
#define NR_GF_HARQ_PROCESS_END   3  // Use HARQ processes 0-3 for GF

typedef struct nr_gf_config {
  
  
  bool enabled;           // GF configuration is valid and can be used
  bool active;            // GF is currently active (can be temporarily deactivated)
  
  /* 
   * GF occasions occur when: (absolute_slot - offset) % periodicity == 0
   * 
   * Example with periodicity=4, offset=1:
   *   Slot:      0  1  2  3  4  5  6  7  8  9  10 11 12 ...
   *   GF:           X        X        X        X
   */
  
  uint16_t periodicity;        // Period in slots (e.g., 1,2,4,5,8,10,16,20,40,80,160...)
                               // Smaller = lower latency, higher overhead
                               // Larger = higher latency, lower overhead
  
  uint16_t offset;             // Slot offset within periodicity (0 to periodicity-1)
                               // Used to avoid collisions when multiple UEs use same period
  
  uint8_t time_domain_allocation;  // Time Domain Resource Assignment index
                                   // References PUSCH-TimeDomainResourceAllocationList
                                   // For , we use explicit start_symbol/nr_of_symbols
  
  uint8_t start_symbol;        // Starting OFDM symbol within slot (0-13)
                               // Typically 2 or later to avoid DMRS/control regions
  
  uint8_t nr_of_symbols;       // Number of OFDM symbols for PUSCH (1-14)
                               // Common values: 12-14 for maximum throughput
  
  uint8_t mapping_type;        // PUSCH mapping type: 0=Type A (slot-based), 1=Type B (mini-slot)
                               // Type A: DMRS in symbol 2 or 3
                               // Type B: DMRS in first symbol of allocation
  
  /*  
   * Defines which PRBs (Physical Resource Blocks) are used for GF transmission.
   * For , we use contiguous allocation (Type 1).
   * 
   * Example with rb_start=10, rb_size=20, BWP_size=100:
   *   PRB:  0  1  2 ... 9  10 11 12 ... 29 30 31 ... 99
   *   GF:                   [=====GF=====]
   */
  
  uint16_t rb_start;           // Starting PRB index within BWP (0 to BWP_size-1)
  uint16_t rb_size;            // Number of contiguous PRBs (1 to BWP_size)
  
  uint8_t frequency_hopping;   // Frequency hopping: 0=disabled, 1=intra-slot, 2=inter-slot
                               // For , we disable frequency hopping (0)
  
  /*  
   * Fixed MCS for GF (no link adaptation in )
   */
  
  uint8_t mcs;                 // Modulation and Coding Scheme index (0-28)
                               // MCS 0-9: QPSK
                               // MCS 10-16: 16QAM  
                               // MCS 17-28: 64QAM
                               // For reliable GF, typically use conservative MCS (5-12)
  
  uint8_t mcs_table;           // MCS table: 0=Table1, 1=Table2 (lower SE), 2=Table3 (64QAM)
                               // For , use Table 1 (default)
  
  /* 
   * HARQ allows retransmission of failed packets.
   * GF typically uses dedicated HARQ processes to avoid conflicts with dynamic grants.
   */
  
  uint8_t harq_process_id;     // HARQ process ID (0 to NR_MAX_HARQ_PROCESSES-1)
                               // Can be fixed or rotating based on implementation
  
  uint8_t rv_sequence[4];      // Redundancy Version sequence for retransmissions
                               // Standard sequence: {0, 2, 3, 1}
                               // Each retransmission uses next RV in sequence
  
  uint8_t ndi_toggle;          // New Data Indicator toggle state
                               // Toggles between 0 and 1 for each new TB
  
  uint8_t repK;                // Number of repetitions (1, 2, 4, or 8)
                               // For , use 1 (no repetition)
  
  /* 
   * Demodulation Reference Signals for channel estimation at gNB
   */
  
  uint8_t dmrs_config_type;    // DMRS type: 0=Type1 (6 RE/RB), 1=Type2 (4 RE/RB)
  
  uint8_t dmrs_ports;          // DMRS antenna port(s) - bitmap
                               // For single layer: typically port 0 (value=1)
  
  uint16_t dmrs_scrambling_id; // DMRS scrambling ID (0-65535)
                               // If 0, use physCellId as scrambling ID
  
  uint8_t num_dmrs_cdm_grps_no_data;  // Number of CDM groups without data (1 or 2)
  
  uint8_t dmrs_add_pos;        // Additional DMRS positions (0-3)
                               // More DMRS = better channel tracking but less data capacity
  
  /*
   * Transmit power settings for GF PUSCH
   */
  
  int8_t delta_power;          // Power offset relative to normal PUSCH (dB)
                               // Positive = higher power (more reliable)
                               // Negative = lower power (less interference)
  
  bool transform_precoding;    // Enable DFT-s-OFDM (true) or CP-OFDM (false)
                               // DFT-s-OFDM has lower PAPR, better for coverage
  
  uint8_t config_index;        // Configuration index (0 to NR_MAX_GF_CONFIGS-1)
                               // Used when multiple GF configs are active
  
  uint16_t rnti;               // RNTI for GF transmission
                               // Can be C-RNTI or dedicated CS-RNTI
                               // If 0, use mac->crnti
  
  
  uint32_t tx_count;           // Total number of GF transmissions
  uint32_t tx_with_data;       // Transmissions that carried actual data
  uint32_t tx_empty;           // Transmissions with no data (empty or BSR only)
  uint32_t harq_nack_count;    // Number of NACKs received
  uint32_t harq_dtx_count;     // Number of DTX (no feedback)

  // Transmission History
  frame_t last_tx_frame;
  int last_tx_slot;
  
  // HARQ Management 
  uint8_t max_retransmissions;      // Maximum HARQ retransmissions (1-8)
  uint8_t current_rv_index;         // Current position in rv_sequence
  
  // Extended Statistics 
  uint32_t successful_tx_count;     // Transmissions with ACK
  uint32_t failed_tx_count;         // Transmissions that failed after max retx
  uint32_t total_bytes_transmitted; // Total bytes sent successfully
  uint32_t total_bsr_sent;          // Number of BSRs included in GF
  
  // Data Tracking 
  uint32_t last_tbs;                // TBS of last transmission
  uint8_t last_mcs_used;            // MCS of last transmission
  
  // Behavior Configuration 
  uint8_t no_data_behavior;         // What to do when no data: 0=skip, 1=send_bsr, 2=padding
  bool include_bsr_always;          // Always include BSR even if not triggered
  
} nr_gf_config_t;

// Grant-Free no-data behavior
typedef enum {
  GF_NO_DATA_SKIP = 0,        // Don't transmit if no data (default)
  GF_NO_DATA_SEND_BSR = 1,    // Send BSR only
  GF_NO_DATA_SEND_PADDING = 2 // Send padding
} gf_no_data_behavior_t;

// Default behavior - can be configured
#define GF_NO_DATA_DEFAULT_BEHAVIOR  GF_NO_DATA_SKIP

// PRACH occasion details
typedef struct prach_occasion_info {
  int start_symbol; // 0 - 13 (14 symbols in a slot)
  int fdm; // 0-7 (possible values of msg1-FDM: 1, 2, 4 or 8)
  int slot;
  int format; // RO preamble format
  int frame_info[2];
  int association_period_idx;
} prach_occasion_info_t;

typedef enum {
  phr_cause_prohibit_timer = 0,
  phr_cause_periodic_timer,
  phr_cause_phr_config,
} NR_UE_PHR_Reporting_cause_t;

/*!\brief UE layer 2 status */
typedef enum {
#define UE_STATE(state) state,
  NR_UE_L2_STATES
#undef UE_STATE
} NR_UE_L2_STATE_t;

typedef struct {
  pucch_format_nr_t format;
  uint8_t startingSymbolIndex;
  uint8_t nrofSymbols;
  uint16_t PRB_offset;
  uint8_t nb_CS_indexes;
  uint8_t initial_CS_indexes[MAX_NB_CYCLIC_SHIFT];
} initial_pucch_resource_t;

typedef enum {
  GO_TO_IDLE,
  DETACH,
  T300_EXPIRY,
  RE_ESTABLISHMENT,
  RRC_SETUP_REESTAB_RESUME,
  UL_SYNC_LOST_T430_EXPIRED,
  REJECT,
} NR_UE_MAC_reset_cause_t;

typedef struct {
  // after multiplexing buffer remain for each lcid
  int32_t LCID_buffer_remain;
  // logical channel group id of this LCID
  long LCGID;
  // Bj bucket usage per lcid
  int32_t Bj;
  NR_timer_t Bj_timer;
} NR_LC_SCHEDULING_INFO;

typedef struct {
  bool active_SR_ID;
  /// SR pending as defined in 38.321
  bool pending;
  /// SR_COUNTER as defined in 38.321
  uint32_t counter;
  /// sr ProhibitTimer
  NR_timer_t prohibitTimer;
  // Maximum number of SR transmissions
  uint32_t maxTransmissions;
} nr_sr_info_t;

typedef struct {
  bool is_configured;
  ///timer before triggering a periodic PHR
  NR_timer_t periodicPHR_Timer;
  ///timer before triggering a prohibit PHR
  NR_timer_t prohibitPHR_Timer;
  ///DL Pathloss change value
  uint16_t PathlossLastValue;
  ///number of subframe before triggering a periodic PHR
  int16_t periodicPHR_SF;
  ///number of subframe before triggering a prohibit PHR
  int16_t prohibitPHR_SF;
  ///DL Pathloss Change in db
  uint16_t PathlossChange_db;
  int phr_reporting;
  bool was_mac_reset;
} nr_phr_info_t;

// LTE structure, might need to be adapted for NR
typedef struct {
  // lcs scheduling info
  NR_LC_SCHEDULING_INFO lc_sched_info[NR_MAX_NUM_LCID];
  // SR INFO
  nr_sr_info_t sr_info[NR_MAX_SR_ID];
  /// BSR report flag management
  uint8_t BSR_reporting_active;
  // LCID triggering BSR
  NR_LogicalChannelIdentity_t regularBSR_trigger_lcid;
  // logicalChannelSR-DelayTimer
  NR_timer_t sr_DelayTimer;
  /// retxBSR-Timer
  NR_timer_t retxBSR_Timer;
  /// periodicBSR-Timer
  NR_timer_t periodicBSR_Timer;

  nr_phr_info_t phr_info;
} NR_UE_SCHEDULING_INFO;

typedef enum {
  nrRA_UE_IDLE,
  nrRA_GENERATE_PREAMBLE,
  nrRA_WAIT_RAR,
  nrRA_WAIT_MSGB,
  nrRA_WAIT_CONTENTION_RESOLUTION,
  nrRA_SUCCEEDED,
  nrRA_FAILED,
} nrRA_UE_state_t;

static const char *const nrra_ue_text[] =
    {"UE_IDLE", "GENERATE_PREAMBLE", "WAIT_RAR", "WAIT_MSGB", "WAIT_CONTENTION_RESOLUTION", "RA_SUCCEEDED", "RA_FAILED"};

typedef struct {
  /// Preamble Tx Counter
  uint8_t preamble_tx_counter;
  /// Preamble Power Ramping Counter
  uint8_t preamble_power_ramping_cnt;
  /// 2-step RA power offset
  int power_offset_2step;
  /// Target received power at gNB. Baseline is range -202..-60 dBm. Depends on delta preamble, power ramping counter and step.
  int ra_preamble_rx_target_power;
  /// RA Preamble Power Ramping Step in dB
  uint32_t preamble_power_ramping_step;
  /// UE configured maximum output power
  int Pc_max;
} NR_PRACH_RESOURCES_t;

typedef struct {
  float ssb_per_ro;
  int preambles_per_ssb;
} ssb_ro_preambles_t;

typedef struct {
  bool active;
  uint32_t preamble_index;
  uint32_t ssb_index;
  uint32_t prach_mask;
} NR_pdcch_order_config_t;

typedef struct {
  // pointer to RACH config dedicated
  NR_RACH_ConfigDedicated_t *rach_ConfigDedicated;
  /// state of RA procedure
  nrRA_UE_state_t ra_state;
  /// RA contention type
  bool cfra;
  /// RA type
  nr_ra_type_t ra_type;
  /// MsgB SuccessRAR MAC subheader
  int8_t MsgB_R;
  int8_t MsgB_CH_ACESS_CPEXT;
  uint8_t MsgB_TPC;
  int8_t MsgB_HARQ_FTI;
  uint16_t timing_advance_command;
  int8_t PUCCH_RI;
  /// RA-rnti
  uint16_t ra_rnti;
  /// MsgB RNTI
  uint16_t MsgB_rnti;
  /// Temporary CRNTI
  uint16_t t_crnti;
  /// Random-access procedure flag
  bool RA_active;
  /// Random-access preamble index
  int ra_PreambleIndex;
  int zeroCorrelationZoneConfig;
  int restricted_set_config;
  // selected SSB for RACH (not the SSB-Index but the cumulative index, excluding not trasmitted SSBs)
  int ra_ssb;
  /// Random-access response window timer
  NR_timer_t response_window_timer;
  int response_window_setup_time;
  /// Random-access backoff timer
  NR_timer_t RA_backoff_timer;
  int RA_backoff_limit;
  uint8_t scaling_factor_bi;
  /// Flag to indicate whether preambles Group A is selected
  bool RA_GroupA;
  /// RA max number of preamble transmissions
  int preambleTransMax;
  /// Received TPC command (in dB) from RAR
  int8_t Msg3_TPC;
  /// RA Msg3 size in bytes
  uint8_t Msg3_size;
  /// Msg3 buffer
  uint8_t *Msg3_buffer;
  // initial Random Access Preamble power
  int preambleRxTargetPower;
  int msg3_deltaPreamble;
  int preambleReceivedTargetPower_config;
  /// Random-access Contention Resolution Timer
  NR_timer_t contention_resolution_timer;
  /// Transmitted UE Contention Resolution Identifier
  uint8_t cont_res_id[6];

  NR_pdcch_order_config_t pdcch_order;

  NR_PRACH_RESOURCES_t prach_resources;

  bool new_ssb;
  int num_fd_occasions;
  int ra_config_index;
  ssb_ro_preambles_t ssb_ro_config;
  int association_periods;
  prach_occasion_info_t sched_ro_info;
  int ro_mask_index;
} RA_config_t;

typedef struct {
  bool active;
  bool ack_received;
  uint8_t  pucch_resource_indicator;
  frame_t ul_frame;
  int ul_slot;
  uint8_t ack;
  int n_CCE;
  int N_CCE;
  int dai_cumul;
  int8_t delta_pucch;
  uint32_t R;
  uint32_t TBS;
  int last_ndi;
  int round;
} NR_UE_DL_HARQ_STATUS_t;

typedef struct {
  uint32_t R;
  uint32_t TBS;
  int last_ndi; //ndi
  int round;

  // Add for GF suport, need to be checked!!
  uint8_t *tx_buffer;        // Pointer to MAC PDU for retransmission
  uint32_t tx_buffer_size;   // Size of stored MAC PDU
  bool is_grant_free;        // True if this HARQ process is used for GF
} NR_UE_UL_HARQ_INFO_t;

typedef struct {
  uint8_t freq_hopping;
  uint8_t mcs;
  uint8_t Msg3_t_alloc;
  uint16_t Msg3_f_alloc;
} RAR_grant_t;

typedef struct {
  NR_PUCCH_Resource_t *pucch_resource;
  uint32_t ack_payload;
  uint8_t sr_payload;
  uint32_t csi_part1_payload;
  uint32_t csi_part2_payload;
  int n_sr;
  int n_csi;
  int n_harq;
  int n_CCE;
  int N_CCE;
  int initial_pucch_id;
} PUCCH_sched_t;

typedef struct {
  uint32_t ssb_index;
  /// SSB RSRP in dBm
  short ssb_rsrp_dBm;
  float_t ssb_sinr_dB;
} NR_SSB_meas_t;

typedef enum ta_type {
  no_ta = 0,
  adjustment_ta,
  rar_ta,
} ta_type_t;

typedef struct NR_UL_TIME_ALIGNMENT {
  /// TA command received from the gNB
  ta_type_t ta_apply;
  int ta_command;
  int frame;
  int slot;
} NR_UL_TIME_ALIGNMENT_t;

// List of all the possible SSBs and their details
typedef struct ssb_list_info {
  int nb_tx_ssb;
  int nb_ssb_per_index[MAX_NB_SSB];
} ssb_list_info_t;

typedef struct nr_lcordered_info_s {
  // logical channels ids ordered as per priority
  NR_LogicalChannelIdentity_t lcid;
  int sr_id;
  long priority;
  uint32_t pbr; // in B/s (UINT_MAX = infinite)
  // Bucket size per lcid
  uint32_t bucket_size;
  bool sr_DelayTimerApplied;
  bool lc_SRMask;
  nr_lcid_rb_t rb;
  bool rb_suspended;
} nr_lcordered_info_t;

typedef struct {
  uint8_t payload[NR_CCCH_PAYLOAD_SIZE_MAX];
} __attribute__ ((__packed__)) NR_CCCH_PDU;

typedef struct {
  long otherSI_SS_id;
  long ra_SS_id;
  long paging_SS_id;
  NR_ControlResourceSet_t *commonControlResourceSet;
  A_SEQUENCE_OF(NR_ControlResourceSet_t) list_Coreset;
  A_SEQUENCE_OF(NR_SearchSpace_t) list_common_SS;
  A_SEQUENCE_OF(NR_SearchSpace_t) list_SS;
} NR_BWP_PDCCH_t;

typedef struct csi_payload {
  uint32_t part1_payload;
  uint32_t part2_payload;
  int p1_bits;
  int p2_bits;
} csi_payload_t;

typedef enum {
  WIDEBAND_ON_PUCCH,
  SUBBAND_ON_PUCCH,
  ON_PUSCH
} CSI_mapping_t;

typedef struct {
  uint64_t rounds[NR_MAX_HARQ_ROUNDS_FOR_STATS];
  uint64_t total_bits;
  uint64_t total_symbols;
  uint64_t target_code_rate;
  uint64_t qam_mod_order;
  uint64_t rb_size;
  uint64_t nr_of_symbols;
} ue_mac_dir_stats_t;

typedef struct {
  ue_mac_dir_stats_t dl;
  ue_mac_dir_stats_t ul;
  uint32_t bad_dci;
  uint32_t ulsch_DTX;
  uint64_t ulsch_total_bytes_scheduled;
  uint32_t pucch0_DTX;
  int cumul_rsrp;
  uint8_t num_rsrp_meas;
  char srs_stats[50]; // Statistics may differ depending on SRS usage
  int pusch_snrx10;
  int deltaMCS;
  int NPRB;
} ue_mac_stats_t;

typedef enum {
  NR_SI_INFO,
  NR_SI_INFO_v1700
} nr_si_info_type;

typedef struct {
  nr_si_info_type type;
  long si_Periodicity;
  long si_WindowPosition;
} si_schedinfo_config_t;

typedef struct {
  int si_window_start;
  int si_WindowLength;
  A_SEQUENCE_OF(si_schedinfo_config_t) si_SchedInfo_list;
} si_schedInfo_t;

typedef struct ntn_timing_advance_components {
  int epoch_sfn;
  int epoch_subframe;

  // N_common_ta_adj represents common round-trip-time between gNB and SAT received in SIB19 (ms)
  double N_common_ta_adj;
  // drift rate of common ta in µs/s
  double N_common_ta_drift;
  // change rate of common ta drift in µs/s²
  double N_common_ta_drift_variant;
  // N_UE_TA_adj calculated round-trip-time between UE and SAT (ms)
  double N_UE_TA_adj;
  // drift rate of N_UE_TA in µs/s
  double N_UE_TA_drift;
  // change rate of N_UE_TA drift in µs/s²
  double N_UE_TA_drift_variant;
  // cell scheduling offset expressed in terms of 15kHz SCS
  long cell_specific_k_offset;

  bool ntn_params_changed;
} ntn_timing_advance_componets_t;

typedef struct {
  // Counters
  uint64_t total_occasions;        // Total GF occasions since start
  uint64_t occasions_with_data;    // Occasions where data was transmitted
  uint64_t occasions_skipped;      // Occasions skipped (no data, skip behavior)
  
  // Throughput
  uint64_t total_bytes_sent;       // Total bytes transmitted
  uint64_t total_tbs_allocated;    // Total TBS across all transmissions
  
  // HARQ Statistics
  uint32_t acks_received;
  uint32_t nacks_received;
  uint32_t dtx_detected;
  uint32_t retransmissions;
  
  // Timing
  uint64_t first_tx_timestamp;     // Timestamp of first GF transmission
  uint64_t last_tx_timestamp;      // Timestamp of last GF transmission
  
  // Efficiency Metrics
  float avg_utilization;           // avg(bytes_sent / tbs)
  float success_rate;              // acks / (acks + failed)
  
} nr_gf_statistics_t;

/*!\brief Top level UE MAC structure */
typedef struct NR_UE_MAC_INST_s {
  module_id_t ue_id;
  NR_UE_L2_STATE_t state;
  int servCellIndex;
  long physCellId;
  bool get_sib1;
  bool get_otherSI[MAX_SI_GROUPS];
  NR_MIB_t *mib;

  si_schedInfo_t si_SchedInfo;
  ssb_list_info_t ssb_list;

  NR_UE_ServingCell_Info_t sc_info;
  A_SEQUENCE_OF(NR_UE_DL_BWP_t) dl_BWPs;
  A_SEQUENCE_OF(NR_UE_UL_BWP_t) ul_BWPs;
  NR_BWP_PDCCH_t config_BWP_PDCCH[MAX_NUM_BWP_UE];
  NR_ControlResourceSet_t *coreset0;
  NR_SearchSpace_t *search_space_zero;
  NR_UE_DL_BWP_t *current_DL_BWP;
  NR_UE_UL_BWP_t *current_UL_BWP;

  bool harq_ACK_SpatialBundlingPUCCH;
  bool harq_ACK_SpatialBundlingPUSCH;

  uint32_t uecap_maxMIMO_PDSCH_layers;
  uint32_t uecap_maxMIMO_PUSCH_layers_cb;
  uint32_t uecap_maxMIMO_PUSCH_layers_nocb;

  NR_UL_TIME_ALIGNMENT_t ul_time_alignment;
  NR_TDD_UL_DL_ConfigCommon_t *tdd_UL_DL_ConfigurationCommon;
  frame_structure_t frame_structure;

  /* Random Access */
  /// CRNTI
  uint16_t crnti;
  /// RA configuration
  RA_config_t ra;
  /// SSB index from MIB decoding
  uint8_t mib_ssb;
  uint32_t mib_additional_bits;
  int mib_frame;

  nr_csi_report_t csi_report_template[MAX_CSI_REPORTCONFIG];

  /// measurements from SS or CSI-RS
  fapi_nr_l1_measurements_t l1_measurements;

  ////	FAPI-like interface message
  fapi_nr_ul_config_request_t *ul_config_request;
  fapi_nr_dl_config_request_t *dl_config_request;

  ///     Interface module instances
  nr_ue_if_module_t       *if_module;
  nr_phy_config_t         phy_config;
  nr_synch_request_t      synch_request;

  // order lc info
  A_SEQUENCE_OF(nr_lcordered_info_t) lc_ordered_list;
  NR_UE_SCHEDULING_INFO scheduling_info;
  NR_timer_t *data_inactivity_timer;

  int dmrs_TypeA_Position;
  int p_Max;
  int p_Max_alt;

  ntn_timing_advance_componets_t ntn_ta;

  long pdsch_HARQ_ACK_Codebook;

  NR_Type0_PDCCH_CSS_config_t type0_PDCCH_CSS_config;
  frequency_range_t frequency_range;
  uint16_t nr_band;
  uint8_t ssb_subcarrier_offset;
  int ssb_start_subcarrier;

  NR_SSB_meas_t ssb_measurements;

  dci_pdu_rel15_t def_dci_pdu_rel15[NR_MAX_SLOTS_PER_FRAME][8];

  // Defined for abstracted mode
  nr_downlink_indication_t dl_info;
  NR_UE_DL_HARQ_STATUS_t dl_harq_info[NR_MAX_HARQ_PROCESSES];
  NR_UE_UL_HARQ_INFO_t ul_harq_info[NR_MAX_HARQ_PROCESSES];

  NR_TAG_Id_t tag_Id;
  A_SEQUENCE_OF(NR_TAG_t) TAG_list;
  NR_TimeAlignmentTimer_t timeAlignmentTimerCommon;
  NR_timer_t time_alignment_timer;

  nr_emulated_l1_t nr_ue_emul_l1;

  pthread_mutex_t mutex_dl_info;

  //SIDELINK MAC PARAMETERS
  sl_nr_ue_mac_params_t *SL_MAC_PARAMS;
  // PUCCH closed loop power control state
  int G_b_f_c;
  bool pucch_power_control_initialized;
  int f_b_f_c;
  bool pusch_power_control_initialized;
  int delta_msg2;
  bool msg3_C_RNTI;
  pthread_mutex_t if_mutex;
  ue_mac_stats_t stats;
  notifiedFIFO_t input_nf;

  nr_gf_config_t gf_config[NR_MAX_GF_CONFIGS];  // Array of GF configurations
  uint8_t num_gf_configs;                        // Number of active GF configs
  bool gf_enabled;                               // Global GF enable flag

  nr_gf_statistics_t gf_stats;                   // Global GF statistics
  
  frame_t frame;                                 // Current frame (for RLC callbacks)

} NR_UE_MAC_INST_t;


static inline int GET_NTN_UE_K_OFFSET(const ntn_timing_advance_componets_t *ntn_ta, int scs)
{
  return (int)ntn_ta->cell_specific_k_offset << scs;
}

static inline long GET_DURATION_RX_TO_TX(const ntn_timing_advance_componets_t *ntn_ta, int scs)
{
  return NR_UE_CAPABILITY_SLOT_RX_TO_TX + (ntn_ta->cell_specific_k_offset << scs);
}

static inline double get_total_TA_ms(const ntn_timing_advance_componets_t *ntn_ta)
{
  return ntn_ta->N_common_ta_adj + ntn_ta->N_UE_TA_adj;
}

static inline double get_total_TA_drift(const ntn_timing_advance_componets_t *ntn_ta)
{
  return ntn_ta->N_common_ta_drift + ntn_ta->N_UE_TA_drift;
}

static inline double get_total_TA_drift_variant(const ntn_timing_advance_componets_t *ntn_ta)
{
  return ntn_ta->N_common_ta_drift_variant + ntn_ta->N_UE_TA_drift_variant;
}

/*@}*/
#endif /*__LAYER2_MAC_DEFS_H__ */
