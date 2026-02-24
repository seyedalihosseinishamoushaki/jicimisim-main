/*
 * ============================================================================
 * GRANT-FREE SCHEDULING - gNB Scheduler Functions
 * File: openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_gf.c 
 * 
 * Core Grant-Free scheduling functions for gNB.
 * ============================================================================
 */

#include "nr_mac_gNB.h"
#include "mac_proto.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_COMMON/nr_mac_common.h"
#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "executables/softmodem-common.h"

#include <string.h>
#include <stdlib.h>

#ifndef NUMBER_OF_NR_UL_SLOTS_IN_FRAME
#define NUMBER_OF_NR_UL_SLOTS_IN_FRAME 20 // Standard OAI default for many configs
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */
uint32_t nr_gf_gnb_calculate_tbs(nr_gf_gnb_ue_config_t *gf_config);
void nr_gf_gnb_update_slot_bitmap(gNB_MAC_INST *gNB);
void nr_gf_gnb_send_harq_feedback(gNB_MAC_INST *gNB, frame_t frame, int slot);
void nr_gf_gnb_schedule_harq_feedback(gNB_MAC_INST *gNB, int ue_id, uint8_t harq_pid, bool ack, frame_t rx_frame, int rx_slot);
nr_gf_gnb_ue_config_t* nr_gf_gnb_get_config_by_harq(gNB_MAC_INST *gNB, int ue_id, uint8_t harq_pid);

/* ============================================================================
 * MACROS & HELPERS
 * ============================================================================
 */

// Calculate absolute slot number
#define GET_ABSOLUTE_SLOT(frame, slot, slots_per_frame) \
    ((uint64_t)(frame) * (slots_per_frame) + (slot))

// Check if current absolute slot is a Grant-Free occasion for the UE
// Formula: (absolute_slot - offset) % periodicity == 0
#define IS_GF_OCCASION(gf_conf, abs_slot) \
    ((abs_slot) >= (gf_conf)->offset && \
     ((abs_slot) - (gf_conf)->offset) % (gf_conf)->periodicity == 0)


/* ============================================================================
 * INITIALIZATION
 * ============================================================================
 */

/**
 * @brief Initialize the Grant-Free manager at gNB
 */
void nr_gf_gnb_init(gNB_MAC_INST *gNB)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  memset(gf_mgr, 0, sizeof(nr_gf_gnb_manager_t));
  
  gf_mgr->gf_enabled = true;
  gf_mgr->num_gf_ues = 0;
  gf_mgr->gf_slot_bitmap = 0;
  gf_mgr->max_periodicity = 0;
  gf_mgr->gf_harq_process_bitmap = 0;
  
  LOG_I(NR_MAC, "[gNB] Grant-Free manager initialized\n");
}


/* ============================================================================
 * CONFIGURATION
 * ============================================================================
 */

/**
 * @brief Validate GF configuration parameters
 */
int nr_gf_gnb_validate_config(gNB_MAC_INST *gNB, nr_gf_gnb_ue_config_t *gf_config)
{
  // Check periodicity
  if (gf_config->periodicity == 0) {
    LOG_E(NR_MAC, "[GF gNB] Invalid periodicity: 0\n");
    return -1;
  }
  
  // Check offset
  if (gf_config->offset >= gf_config->periodicity) {
    LOG_E(NR_MAC, "[GF gNB] Offset (%d) >= periodicity (%d)\n",
          gf_config->offset, gf_config->periodicity);
    return -2;
  }
  
  // Check symbol allocation
  if (gf_config->start_symbol > 13) {
    LOG_E(NR_MAC, "[GF gNB] Invalid start_symbol: %d\n", gf_config->start_symbol);
    return -3;
  }
  
  if (gf_config->nr_of_symbols == 0 || gf_config->nr_of_symbols > 14) {
    LOG_E(NR_MAC, "[GF gNB] Invalid nr_of_symbols: %d\n", gf_config->nr_of_symbols);
    return -4;
  }
  
  if (gf_config->start_symbol + gf_config->nr_of_symbols > 14) {
    LOG_E(NR_MAC, "[GF gNB] Symbol allocation exceeds slot: %d + %d > 14\n",
          gf_config->start_symbol, gf_config->nr_of_symbols);
    return -5;
  }
  
  // Check RB allocation
  if (gf_config->rb_size == 0) {
    LOG_E(NR_MAC, "[GF gNB] Invalid rb_size: 0\n");
    return -6;
  }
  
  // Check MCS
  if (gf_config->mcs > 28) {
    LOG_E(NR_MAC, "[GF gNB] Invalid MCS: %d (max 28)\n", gf_config->mcs);
    return -7;
  }
  
  // Check HARQ process
  if (gf_config->harq_process_id >= 16) {  // NR supports up to 16 HARQ processes
    LOG_E(NR_MAC, "[GF gNB] Invalid HARQ process: %d\n", gf_config->harq_process_id);
    return -8;
  }
  
  return 0;  // Valid
}


/**
 * @brief Configure Grant-Free for a specific UE
 */
int nr_gf_gnb_configure_ue(gNB_MAC_INST *gNB,
                            int ue_id,
                            rnti_t rnti,
                            nr_gf_gnb_ue_config_t *gf_config)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  // Validate UE ID
  if (ue_id < 0 || ue_id >= NR_MAX_GF_UES) {
    LOG_E(NR_MAC, "[GF gNB] Invalid ue_id: %d\n", ue_id);
    return -1;
  }
  
  // Validate configuration
  int ret = nr_gf_gnb_validate_config(gNB, gf_config);
  if (ret != 0) {
    LOG_E(NR_MAC, "[GF gNB] Configuration validation failed: %d\n", ret);
    return -1;
  }
  
  // Find slot for this config
  uint8_t *num_configs = &gf_mgr->num_configs_per_ue[ue_id];
  if (*num_configs >= NR_MAX_GF_CONFIGS_PER_UE) {
    LOG_E(NR_MAC, "[GF gNB] Max GF configs reached for UE %d\n", ue_id);
    return -1;
  }
  
  // Check for HARQ process conflict
  if (gf_mgr->gf_harq_process_bitmap & (1 << gf_config->harq_process_id)) {
    LOG_W(NR_MAC, "[GF gNB] HARQ process %d already in use by another GF config\n",
          gf_config->harq_process_id);
    // Allow it but warn - might be intentional for same UE
  }
  
  // Store configuration
  int config_idx = *num_configs;
  nr_gf_gnb_ue_config_t *stored = &gf_mgr->ue_configs[ue_id][config_idx];
  
  memcpy(stored, gf_config, sizeof(nr_gf_gnb_ue_config_t));
  
  // Set UE identification
  stored->ue_id = ue_id;
  stored->rnti = rnti;
  stored->enabled = true;
  stored->active = true;
  
  // Set defaults if not provided
  if (stored->rv_sequence[0] == 0 && stored->rv_sequence[1] == 0 &&
      stored->rv_sequence[2] == 0 && stored->rv_sequence[3] == 0) {
    stored->rv_sequence[0] = 0;
    stored->rv_sequence[1] = 2;
    stored->rv_sequence[2] = 3;
    stored->rv_sequence[3] = 1;
  }
  
  // Initialize statistics
  stored->rx_count = 0;
  stored->rx_success = 0;
  stored->rx_failure = 0;
  stored->harq_ack_sent = 0;
  stored->harq_nack_sent = 0;
  stored->total_bytes_received = 0;
  stored->expected_ndi = 0;
  stored->current_round = 0;
  
  // Calculate expected TBS
  stored->expected_tbs = nr_gf_gnb_calculate_tbs(stored);
  
  // Update manager state
  (*num_configs)++;
  gf_mgr->gf_harq_process_bitmap |= (1 << stored->harq_process_id);
  
  if (stored->periodicity > gf_mgr->max_periodicity) {
    gf_mgr->max_periodicity = stored->periodicity;
  }
  
  if (*num_configs == 1) {
    gf_mgr->num_gf_ues++;
  }
  
  gf_mgr->gf_enabled = true;
  
  // Update slot bitmap
  nr_gf_gnb_update_slot_bitmap(gNB);
  
  LOG_I(NR_MAC, "[GF gNB] Configured GF for UE %d (RNTI 0x%04x), config %d:\n"
        "        Periodicity: %d slots, Offset: %d\n"
        "        Time: symbol %d to %d (%d symbols)\n"
        "        Frequency: PRB %d to %d (%d PRBs)\n"
        "        MCS: %d (table %d)\n"
        "        HARQ: process %d, Expected TBS: %d bytes\n",
        ue_id, rnti, config_idx,
        stored->periodicity, stored->offset,
        stored->start_symbol, stored->start_symbol + stored->nr_of_symbols - 1,
        stored->nr_of_symbols,
        stored->rb_start, stored->rb_start + stored->rb_size - 1,
        stored->rb_size,
        stored->mcs, stored->mcs_table,
        stored->harq_process_id, stored->expected_tbs);
  
  return config_idx;
}


/**
 * @brief Configure default GF for testing
 */
void nr_gf_gnb_configure_default(gNB_MAC_INST *gNB, int ue_id, rnti_t rnti)
{
  nr_gf_gnb_ue_config_t gf_config = {0};
  
  // Default configuration matching UE-side defaults
  gf_config.periodicity = 4;       // Every 4 slots
  gf_config.offset = 0;            // Start at slot 0
  gf_config.start_symbol = 2;      // Start at symbol 2
  gf_config.nr_of_symbols = 12;    // 12 symbols
  gf_config.mapping_type = 0;      // Type A
  
  gf_config.rb_start = 0;          // Start at PRB 0
  gf_config.rb_size = 10;          // 10 PRBs
  gf_config.frequency_hopping = 0; // No hopping
  
  gf_config.mcs = 9;               // MCS 9 (QPSK)
  gf_config.mcs_table = 0;         // Table 1
  
  gf_config.harq_process_id = 0;   // HARQ process 0
  gf_config.rv_sequence[0] = 0;
  gf_config.rv_sequence[1] = 2;
  gf_config.rv_sequence[2] = 3;
  gf_config.rv_sequence[3] = 1;
  
  gf_config.dmrs_config_type = 0;  // Type 1
  gf_config.dmrs_ports = 1;        // Port 0
  gf_config.num_dmrs_cdm_grps_no_data = 2;
  
  nr_gf_gnb_configure_ue(gNB, ue_id, rnti, &gf_config);
}


/**
 * @brief Release Grant-Free configuration for a UE
 */
void nr_gf_gnb_release_ue(gNB_MAC_INST *gNB, int ue_id, int config_index)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  if (ue_id < 0 || ue_id >= NR_MAX_GF_UES) {
    return;
  }
  
  if (config_index == -1) {
    // Release all configs for this UE
    for (int i = 0; i < gf_mgr->num_configs_per_ue[ue_id]; i++) {
      nr_gf_gnb_ue_config_t *cfg = &gf_mgr->ue_configs[ue_id][i];
      gf_mgr->gf_harq_process_bitmap &= ~(1 << cfg->harq_process_id);
      memset(cfg, 0, sizeof(nr_gf_gnb_ue_config_t));
    }
    gf_mgr->num_configs_per_ue[ue_id] = 0;
    gf_mgr->num_gf_ues--;
    
    LOG_I(NR_MAC, "[GF gNB] Released all GF configs for UE %d\n", ue_id);
  } else {
    // Release specific config
    if (config_index < gf_mgr->num_configs_per_ue[ue_id]) {
      nr_gf_gnb_ue_config_t *cfg = &gf_mgr->ue_configs[ue_id][config_index];
      gf_mgr->gf_harq_process_bitmap &= ~(1 << cfg->harq_process_id);
      
      // Shift remaining configs
      for (int i = config_index; i < gf_mgr->num_configs_per_ue[ue_id] - 1; i++) {
        memcpy(&gf_mgr->ue_configs[ue_id][i],
               &gf_mgr->ue_configs[ue_id][i + 1],
               sizeof(nr_gf_gnb_ue_config_t));
      }
      gf_mgr->num_configs_per_ue[ue_id]--;
      
      LOG_I(NR_MAC, "[GF gNB] Released GF config %d for UE %d\n", config_index, ue_id);
    }
  }
  
  // Update slot bitmap
  nr_gf_gnb_update_slot_bitmap(gNB);
  
  // Check if any GF configs remain
  gf_mgr->gf_enabled = false;
  for (int i = 0; i < NR_MAX_GF_UES; i++) {
    if (gf_mgr->num_configs_per_ue[i] > 0) {
      gf_mgr->gf_enabled = true;
      break;
    }
  }
}


/**
 * @brief Update slot bitmap for quick lookup
 */
void nr_gf_gnb_update_slot_bitmap(gNB_MAC_INST *gNB)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  gf_mgr->gf_slot_bitmap = 0;
  gf_mgr->max_periodicity = 0;
  
  // Iterate all configs and mark slots
  for (int ue = 0; ue < NR_MAX_GF_UES; ue++) {
    for (int cfg = 0; cfg < gf_mgr->num_configs_per_ue[ue]; cfg++) {
      nr_gf_gnb_ue_config_t *gf = &gf_mgr->ue_configs[ue][cfg];
      
      if (!gf->enabled || !gf->active) continue;
      
      if (gf->periodicity > gf_mgr->max_periodicity) {
        gf_mgr->max_periodicity = gf->periodicity;
      }
      
      // Mark all slots in the bitmap that are GF occasions
      // (for periods <= 64, we can mark all; for larger, mark first 64)
      for (int s = gf->offset; s < 64; s += gf->periodicity) {
        gf_mgr->gf_slot_bitmap |= (1ULL << s);
      }
    }
  }
}


/* ============================================================================
 * SLOT PROCESSING
 * ============================================================================
 */

/**
 * @brief Check for GF occasions in the current slot
 */
int nr_gf_gnb_check_occasions(gNB_MAC_INST *gNB,
                               frame_t frame,
                               int slot,
                               nr_gf_gnb_slot_info_t *info)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  info->num_expected = 0;
  
  if (!gf_mgr->gf_enabled) {
    return 0;
  }
  
  // Get frame configuration
  NR_ServingCellConfigCommon_t *scc = gNB->common_channels[0].ServingCellConfigCommon;
  int slots_per_frame = get_slots_per_frame_from_scs(*scc->ssbSubcarrierSpacing);
  uint64_t abs_slot = GET_ABSOLUTE_SLOT(frame, slot, slots_per_frame);
  
  // Iterate all UE configs
  for (int ue = 0; ue < NR_MAX_GF_UES; ue++) {
    for (int cfg = 0; cfg < gf_mgr->num_configs_per_ue[ue]; cfg++) {
      nr_gf_gnb_ue_config_t *gf = &gf_mgr->ue_configs[ue][cfg];
      
      if (!gf->enabled || !gf->active) continue;
      
      // Check GF occasion formula
      if (IS_GF_OCCASION(gf, abs_slot)) {
        if (info->num_expected < NR_MAX_GF_PER_SLOT) {
          info->configs[info->num_expected++] = gf;
          
          LOG_D(NR_MAC, "[GF gNB] GF occasion for UE %d (RNTI 0x%04x) at %d.%d\n",
                ue, gf->rnti, frame, slot);
        } else {
          LOG_W(NR_MAC, "[GF gNB] Too many GF occasions in slot %d.%d\n", frame, slot);
        }
      }
    }
  }
  
  return info->num_expected;
}


/**
 * @brief Configure PUSCH PDU for GF reception
 */
int nr_gf_gnb_config_pusch_pdu(gNB_MAC_INST *gNB,
                                nr_gf_gnb_ue_config_t *gf_config,
                                nfapi_nr_pusch_pdu_t *pusch_pdu,
                                frame_t frame,
                                int slot)
{
  NR_ServingCellConfigCommon_t *scc = gNB->common_channels[0].ServingCellConfigCommon;
  
  memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));
  
  // Basic parameters
  pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_pdu->rnti = gf_config->rnti;
  
  // BWP configuration
  NR_BWP_t *genericParameters = &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
  pusch_pdu->bwp_size = NRRIV2BW(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
  pusch_pdu->bwp_start = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
  pusch_pdu->subcarrier_spacing = *scc->ssbSubcarrierSpacing;
  pusch_pdu->cyclic_prefix = 0;  // Normal CP
  
  // Resource allocation
  pusch_pdu->resource_alloc = 1;  // Type 1
  pusch_pdu->rb_start = gf_config->rb_start;
  pusch_pdu->rb_size = gf_config->rb_size;
  pusch_pdu->vrb_to_prb_mapping = 0;  // Non-interleaved
  pusch_pdu->frequency_hopping = gf_config->frequency_hopping;
  
  // Time allocation
  pusch_pdu->start_symbol_index = gf_config->start_symbol;
  pusch_pdu->nr_of_symbols = gf_config->nr_of_symbols;
  
  // MCS
  pusch_pdu->mcs_index = gf_config->mcs;
  pusch_pdu->mcs_table = gf_config->mcs_table;
  pusch_pdu->target_code_rate = nr_get_code_rate_ul(gf_config->mcs, gf_config->mcs_table);
  pusch_pdu->qam_mod_order = nr_get_Qm_ul(gf_config->mcs, gf_config->mcs_table);
  pusch_pdu->transform_precoding = 0;  // Disabled
  
  // DMRS
  pusch_pdu->num_dmrs_cdm_grps_no_data = gf_config->num_dmrs_cdm_grps_no_data;
  pusch_pdu->dmrs_ports = gf_config->dmrs_ports;
  
  // Calculate DMRS symbol positions
  //int l0 = get_l0_ul(*scc->ssbSubcarrierSpacing, gf_config->mapping_type);
  uint16_t l_prime = get_l_prime(gf_config->nr_of_symbols,
                                  gf_config->mapping_type,
                                  (pusch_dmrs_AdditionalPosition_t)0,
                                  pusch_dmrs_pos1,
                                  gf_config->start_symbol,
                                  scc->dmrs_TypeA_Position ? 3 : 2);
  pusch_pdu->ul_dmrs_symb_pos = l_prime;
  pusch_pdu->dmrs_config_type = gf_config->dmrs_config_type;
  pusch_pdu->ul_dmrs_scrambling_id = gf_config->dmrs_scrambling_id;
  pusch_pdu->scid = 0;
  pusch_pdu->nrOfLayers = 1;
  
  // HARQ
  pusch_pdu->pusch_data.harq_process_id = gf_config->harq_process_id;
  pusch_pdu->pusch_data.new_data_indicator = gf_config->expected_ndi;
  pusch_pdu->pusch_data.rv_index = gf_config->rv_sequence[gf_config->current_round % 4];
  
  // Calculate TBS
  uint8_t N_PRB_DMRS = (gf_config->dmrs_config_type == 0) ? 6 : 4;
 // uint8_t dmrs_length = 1;  // Single symbol DMRS
  uint16_t dmrs_symbols = __builtin_popcount(l_prime);
//  uint16_t N_RE_prime = NR_NB_SC_PER_RB * gf_config->nr_of_symbols -
//                         N_PRB_DMRS * dmrs_symbols * dmrs_length;
  
  uint32_t TBS = nr_compute_tbs(pusch_pdu->qam_mod_order,
                                 pusch_pdu->target_code_rate,
                                 gf_config->rb_size,
                                 gf_config->nr_of_symbols,
                                 N_PRB_DMRS * dmrs_symbols,
                                 0,  // N_PRB_oh
                                 0,  // tb_scaling
                                 1); // nrOfLayers
  
  pusch_pdu->pusch_data.tb_size = TBS;
  pusch_pdu->pusch_data.num_cb = 0;  // Let PHY calculate
  
  LOG_D(NR_MAC, "[GF gNB] Configured PUSCH PDU for UE %d (RNTI 0x%04x):\n"
        "        RBs: %d-%d, Symbols: %d-%d\n"
        "        MCS: %d, TBS: %d bytes\n"
        "        HARQ: pid=%d, ndi=%d, rv=%d\n",
        gf_config->ue_id, gf_config->rnti,
        gf_config->rb_start, gf_config->rb_start + gf_config->rb_size - 1,
        gf_config->start_symbol, gf_config->start_symbol + gf_config->nr_of_symbols - 1,
        gf_config->mcs, TBS,
        gf_config->harq_process_id, gf_config->expected_ndi,
        pusch_pdu->pusch_data.rv_index);
  
  return 0;
}


/**
 * @brief Schedule GF PUSCH reception
 */
#define NR_MAX_UL_TTI_PDUS 8  // Define local limit if missing
void nr_gf_gnb_schedule_reception(gNB_MAC_INST *gNB,
                                   frame_t frame,
                                   int slot,
                                   nr_gf_gnb_slot_info_t *info)
{
  // For each expected GF transmission, configure PUSCH reception
  for (int i = 0; i < info->num_expected; i++) {
    nr_gf_gnb_ue_config_t *gf = info->configs[i];
    
    // Configure PUSCH PDU
    nfapi_nr_pusch_pdu_t pusch_pdu;
    nr_gf_gnb_config_pusch_pdu(gNB, gf, &pusch_pdu, frame, slot);
    
    /* ================================================
     * FAPI INTERFACE IMPLEMENTATION
     * ================================================
     */
     
    // Calculate the index in the look-ahead buffer
    const int n_slots_ahead = NUMBER_OF_NR_UL_SLOTS_IN_FRAME; 
    int tti_index = slot % n_slots_ahead;

    // Get the UL TTI request container for CC 0
    nfapi_nr_ul_tti_request_t *ul_tti_req = &gNB->UL_tti_req_ahead[0][tti_index];

    // Safety check using 'n_pdus'
    if (ul_tti_req->n_pdus < NR_MAX_UL_TTI_PDUS) {
        
        // Point to the next available PDU
        nfapi_nr_ul_tti_request_number_of_pdus_t *tti_pdu = 
            &ul_tti_req->pdus_list[ul_tti_req->n_pdus];
        
        // Set PDU Header
        tti_pdu->pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
        tti_pdu->pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
        
        // Copy PUSCH config
        memcpy(&tti_pdu->pusch_pdu, &pusch_pdu, sizeof(nfapi_nr_pusch_pdu_t));
        
        // Increment counter
        ul_tti_req->n_pdus++;
        
        // Set Timing
        ul_tti_req->SFN = frame;
        ul_tti_req->Slot = slot;
        
    } else {
        LOG_E(NR_MAC, "[GF gNB] UL TTI request full! Dropping GF UE %d\n", gf->ue_id);
        continue;
    }
    /* ==================================================================== */
    
    // Update reception stats
    gf->rx_count++;
    gf->last_rx_frame = frame;
    gf->last_rx_slot = slot;
    
    LOG_I(NR_MAC, "[GF gNB] Scheduled PUSCH reception for UE %d at %d.%d (attempt %d)\n",
          gf->ue_id, frame, slot, gf->rx_count);
  }
  
  gNB->gf_manager.total_gf_occasions += info->num_expected;
}

/**
 * @brief Main GF processing function for each slot
 */
void nr_gf_gnb_slot_process(gNB_MAC_INST *gNB, frame_t frame, int slot)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  if (!gf_mgr->gf_enabled) {
    return;
  }
  
  // Check for GF occasions in this slot
  nr_gf_gnb_slot_info_t slot_info;
  int num_gf = nr_gf_gnb_check_occasions(gNB, frame, slot, &slot_info);
  
  if (num_gf > 0) {
    LOG_D(NR_MAC, "[GF gNB] %d GF transmission(s) expected at %d.%d\n",
          num_gf, frame, slot);
    
    // Schedule PUSCH reception for each expected GF
    nr_gf_gnb_schedule_reception(gNB, frame, slot, &slot_info);
  }
  
  // Send any pending HARQ feedback scheduled for this slot
  nr_gf_gnb_send_harq_feedback(gNB, frame, slot);
}


/* ============================================================================
 * RECEPTION HANDLING
 * ============================================================================
 */

/**
 * @brief Process received GF PUSCH
 */
void nr_gf_gnb_process_rx(gNB_MAC_INST *gNB,
                           int ue_id,
                           uint8_t harq_pid,
                           bool crc_valid,
                           uint8_t *mac_pdu,
                           uint32_t pdu_length,
                           int8_t sinr_db)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  // Find the GF config for this HARQ process
  nr_gf_gnb_ue_config_t *gf = nr_gf_gnb_get_config_by_harq(gNB, ue_id, harq_pid);
  
  if (!gf) {
    LOG_W(NR_MAC, "[GF gNB] No GF config found for UE %d HARQ %d\n", ue_id, harq_pid);
    return;
  }
  
  gf->last_sinr_db = sinr_db;
  gf->last_rx_success = crc_valid;
  gf_mgr->total_gf_receptions++;
  
  if (crc_valid) {
    // Success - process MAC PDU
    LOG_I(NR_MAC, "[GF gNB] Successfully decoded GF PUSCH from UE %d (RNTI 0x%04x)\n"
          "        HARQ pid=%d, length=%d bytes, SINR=%d dB\n",
          ue_id, gf->rnti, harq_pid, pdu_length, sinr_db);
    
    gf->rx_success++;
    gf->total_bytes_received += pdu_length;
    gf_mgr->total_gf_successes++;
    
    // Process MAC PDU, using existing OAI function:
    //nr_process_mac_pdu(gNB, ue_id, mac_pdu, pdu_length);
    
    // Toggle expected NDI for next new transmission
    gf->expected_ndi = !gf->expected_ndi;
    gf->current_round = 0;
    
    // Schedule ACK
    nr_gf_gnb_schedule_harq_feedback(gNB, ue_id, harq_pid, true,
                                      gf->last_rx_frame, gf->last_rx_slot);
    
  } else {
    // Failure - schedule NACK for retransmission
    LOG_W(NR_MAC, "[GF gNB] Failed to decode GF PUSCH from UE %d (RNTI 0x%04x)\n"
          "        HARQ pid=%d, SINR=%d dB, round=%d\n",
          ue_id, gf->rnti, harq_pid, sinr_db, gf->current_round);
    
    gf->rx_failure++;
    gf->current_round++;
    
    // Check max retransmissions
    if (gf->current_round >= 4) {  // Max 4 retransmissions
      LOG_E(NR_MAC, "[GF gNB] Max retransmissions reached for UE %d HARQ %d\n",
            ue_id, harq_pid);
      gf->current_round = 0;
      gf->expected_ndi = !gf->expected_ndi;  // Move on
    }
    
    // Schedule NACK
    nr_gf_gnb_schedule_harq_feedback(gNB, ue_id, harq_pid, false,
                                      gf->last_rx_frame, gf->last_rx_slot);
  }
}


/* ============================================================================
 * HARQ FEEDBACK
 * ============================================================================
 */

/**
 * @brief Schedule HARQ feedback for GF transmission
 */
void nr_gf_gnb_schedule_harq_feedback(gNB_MAC_INST *gNB,
                                       int ue_id,
                                       uint8_t harq_pid,
                                       bool ack,
                                       frame_t rx_frame,
                                       int rx_slot)
{
  NR_ServingCellConfigCommon_t *scc = gNB->common_channels[0].ServingCellConfigCommon;
  int slots_per_frame = get_slots_per_frame_from_scs(*scc->ssbSubcarrierSpacing);
  
  // Calculate feedback timing (K1-like delay)
  int k1 = 4;
  int fb_slot = rx_slot + k1;
  frame_t fb_frame = rx_frame;
  
  while (fb_slot >= slots_per_frame) {
    fb_slot -= slots_per_frame;
    fb_frame = (fb_frame + 1) % 1024;
  }
  
  // Note: There is no actual feedback PDU configuration here. It has to be done.
  
  LOG_D(NR_MAC, "[GF gNB] Scheduled HARQ %s for UE %d HARQ %d at %d.%d\n",
        ack ? "ACK" : "NACK", ue_id, harq_pid, fb_frame, fb_slot);
}


/**
 * @brief Send pending HARQ feedback
 */
void nr_gf_gnb_send_harq_feedback(gNB_MAC_INST *gNB, frame_t frame, int slot)
{
  // TODO: Implement HARQ feedback transmission.
  // This requires iterating over the active UE list in gNB->UE_info (NR_UEs_t)
  // and checking for pending feedback scheduled in nr_gf_gnb_schedule_harq_feedback.
  
  // For now, this is a placeholder to allow compilation.
}

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

/**
 * @brief Calculate TBS for GF configuration
 */
uint32_t nr_gf_gnb_calculate_tbs(nr_gf_gnb_ue_config_t *gf_config)
{
  uint8_t qam_order = nr_get_Qm_ul(gf_config->mcs, gf_config->mcs_table);
  uint16_t code_rate = nr_get_code_rate_ul(gf_config->mcs, gf_config->mcs_table);
  
  // Estimate DMRS overhead
  uint8_t N_PRB_DMRS = (gf_config->dmrs_config_type == 0) ? 6 : 4;
  uint8_t dmrs_symbols = 1;  // Single DMRS
  
  uint32_t TBS = nr_compute_tbs(qam_order,
                                 code_rate,
                                 gf_config->rb_size,
                                 gf_config->nr_of_symbols,
                                 N_PRB_DMRS * dmrs_symbols,
                                 0,  // N_PRB_oh
                                 0,  // tb_scaling
                                 1); // nrOfLayers
  
  return TBS;
}


/**
 * @brief Get GF config by HARQ process ID
 */
nr_gf_gnb_ue_config_t* nr_gf_gnb_get_config_by_harq(gNB_MAC_INST *gNB,
                                                     int ue_id,
                                                     uint8_t harq_pid)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  if (ue_id < 0 || ue_id >= NR_MAX_GF_UES) {
    return NULL;
  }
  
  for (int cfg = 0; cfg < gf_mgr->num_configs_per_ue[ue_id]; cfg++) {
    if (gf_mgr->ue_configs[ue_id][cfg].harq_process_id == harq_pid) {
      return &gf_mgr->ue_configs[ue_id][cfg];
    }
  }
  
  return NULL;
}


/* ============================================================================
 * STATISTICS
 * ============================================================================
 */

/**
 * @brief Print GF statistics to log
 */
void nr_gf_gnb_print_stats(gNB_MAC_INST *gNB)
{
  nr_gf_gnb_manager_t *gf_mgr = &gNB->gf_manager;
  
  LOG_I(NR_MAC, "\n========== Grant-Free Statistics (gNB) ==========\n");
  LOG_I(NR_MAC, "Global: enabled=%d, UEs=%d\n", gf_mgr->gf_enabled, gf_mgr->num_gf_ues);
  LOG_I(NR_MAC, "Occasions: %lu, Receptions: %lu, Successes: %lu (%.1f%%)\n",
        gf_mgr->total_gf_occasions,
        gf_mgr->total_gf_receptions,
        gf_mgr->total_gf_successes,
        gf_mgr->total_gf_receptions > 0 ?
          100.0 * gf_mgr->total_gf_successes / gf_mgr->total_gf_receptions : 0.0);
  
  for (int ue = 0; ue < NR_MAX_GF_UES; ue++) {
    for (int cfg = 0; cfg < gf_mgr->num_configs_per_ue[ue]; cfg++) {
      nr_gf_gnb_ue_config_t *gf = &gf_mgr->ue_configs[ue][cfg];
      LOG_I(NR_MAC, "UE %d (RNTI 0x%04x) config %d:\n", ue, gf->rnti, cfg);
      LOG_I(NR_MAC, "  RX: %d success, %d fail, %lu bytes\n",
            gf->rx_success, gf->rx_failure, gf->total_bytes_received);
      LOG_I(NR_MAC, "  HARQ: %d ACK, %d NACK\n", gf->harq_ack_sent, gf->harq_nack_sent);
    }
  }
  LOG_I(NR_MAC, "==================================================\n\n");
}
