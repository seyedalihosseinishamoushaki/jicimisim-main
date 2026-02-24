#include "nr_mac_gNB_grant_free_scheduler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Initialize the grant-free scheduler
 */
void nr_gf_scheduler_init(grant_free_scheduler_t *scheduler,
                          uint16_t max_prbs,
                          uint8_t numerology,
                          grant_free_config_t *config) {
    if (scheduler == NULL) {
        printf("[GF-SCHED] Error: NULL scheduler pointer\n");
        return;
    }
    
    memset(scheduler, 0, sizeof(grant_free_scheduler_t));
    
    scheduler->max_prbs = max_prbs;
    scheduler->numerology = numerology;
    
    // Initialize UE context
    memset(&scheduler->ue_context, 0, sizeof(grant_free_ue_context_t));
    
    if (config != NULL) {
        memcpy(&scheduler->ue_context.gf_config, config, sizeof(grant_free_config_t));
        
        // Adjust num_prb if it exceeds maximum
        if (scheduler->ue_context.gf_config.num_prb > max_prbs) {
            printf("[GF-SCHED] Adjusting num_prb from %d to %d (max available)\n",
                   scheduler->ue_context.gf_config.num_prb, max_prbs);
            scheduler->ue_context.gf_config.num_prb = max_prbs;
        }
        
        // Validate configuration
        if (validate_grant_free_config(&scheduler->ue_context.gf_config, max_prbs)) {
            scheduler->initialized = true;
            //printf("[GF-SCHED] Scheduler initialized successfully\n");
            print_grant_free_config(&scheduler->ue_context.gf_config);
        } else {
            //printf("[GF-SCHED] Error: Configuration validation failed\n");
        }
    } else {
        //printf("[GF-SCHED] Warning: Initialized without configuration\n");
    }
}

/**
 * Configure resources for a UE in grant-free mode
 */
bool nr_gf_configure_ue(grant_free_scheduler_t *scheduler,
                        uint32_t rnti,
                        grant_free_config_t *config) {
    if (scheduler == NULL || config == NULL) {
        printf("[GF-SCHED] Error: NULL pointer in configure_ue\n");
        return false;
    }
    
    // Validate configuration
    if (!validate_grant_free_config(config, scheduler->max_prbs)) {
        printf("[GF-SCHED] Error: Invalid configuration for RNTI 0x%04X\n", rnti);
        return false;
    }
    
    // Store configuration
    scheduler->ue_context.rnti = rnti;
    memcpy(&scheduler->ue_context.gf_config, config, sizeof(grant_free_config_t));
    scheduler->ue_context.resources_configured = true;
    scheduler->ue_context.harq_process_id = 0;
    scheduler->ue_context.current_harq_round = 0;
    
    // Reset statistics
    scheduler->ue_context.tx_opportunities = 0;
    scheduler->ue_context.data_received = 0;
    scheduler->ue_context.crc_errors = 0;
    
    printf("[GF-SCHED] Configured UE RNTI=0x%04X for grant-free operation\n", rnti);
    print_grant_free_config(&scheduler->ue_context.gf_config);
    
    return true;
}

/**
 * Check if current slot is a grant-free transmission opportunity
 */
bool nr_gf_is_tx_slot(grant_free_scheduler_t *scheduler,
                      uint16_t frame,
                      uint8_t slot) {
    if (scheduler == NULL || !scheduler->initialized) {
        return false;
    }
    
    if (!scheduler->ue_context.resources_configured) {
        return false;
    }
    
    if (!scheduler->ue_context.gf_config.enabled) {
        return false;
    }
    
    uint8_t periodicity = scheduler->ue_context.gf_config.periodicity_slots;
    
    // Calculate slots per frame based on numerology
    // Numerology 0 (15kHz): 10 slots/frame
    // Numerology 1 (30kHz): 20 slots/frame
    // Numerology 2 (60kHz): 40 slots/frame
    uint8_t slots_per_frame = 10 * (1 << scheduler->numerology);
    
    // Calculate absolute slot number
    uint32_t abs_slot = frame * slots_per_frame + slot;
    
    // Check if this is a transmission opportunity based on periodicity
    bool is_tx_slot = (abs_slot % periodicity) == 0;
    
    if (is_tx_slot) {
        //printf("[GF-SCHED] Frame=%d Slot=%d: Grant-free TX opportunity\n", frame, slot);
    }
    
    return is_tx_slot;
}

/**
 * Main scheduling function - called every slot
 * This prepares the gNB to receive PUSCH in grant-free mode
 */
void nr_gf_schedule_ulsch(grant_free_scheduler_t *scheduler,
                          uint16_t frame,
                          uint8_t slot) {
    if (scheduler == NULL || !scheduler->initialized) {
        return;
    }
    
    // Update current frame/slot
    scheduler->ue_context.current_frame = frame;
    scheduler->ue_context.current_slot = slot;
    
    // Check if this is a grant-free transmission slot
    if (!nr_gf_is_tx_slot(scheduler, frame, slot)) {
        return;
    }
    
    // This is a grant-free transmission opportunity
    scheduler->ue_context.tx_opportunities++;
    /*
    printf("[GF-SCHED] Frame=%d Slot=%d: Preparing to receive grant-free PUSCH\n",
           frame, slot);
    printf("[GF-SCHED]   PRBs: %d-%d (%d total)\n",
           scheduler->ue_context.gf_config.start_prb,
           scheduler->ue_context.gf_config.start_prb + scheduler->ue_context.gf_config.num_prb - 1,
           scheduler->ue_context.gf_config.num_prb);
    printf("[GF-SCHED]   Symbols: %d-%d (%d total)\n",
           scheduler->ue_context.gf_config.start_symbol,
           scheduler->ue_context.gf_config.start_symbol + scheduler->ue_context.gf_config.num_symbols - 1,
           scheduler->ue_context.gf_config.num_symbols);
    printf("[GF-SCHED]   MCS: %d\n", scheduler->ue_context.gf_config.mcs);
    printf("[GF-SCHED]   HARQ Process: %d, Round: %d\n",
           scheduler->ue_context.harq_process_id,
           scheduler->ue_context.current_harq_round);
    */
    /*
     * At this point, you would:
     * 1. Configure the PHY to listen for PUSCH on the specified resources
     * 2. Set up HARQ process
     * 3. Prepare to receive data
     * 
     * In the actual OAI implementation, this would involve:
     * - Calling PHY configuration functions
     * - Setting up nFAPI messages
     * - Configuring PUSCH reception parameters
     * 
     * For example (pseudocode):
     * 
     * nfapi_nr_ul_tti_request_t *UL_tti_req = &scheduler->UL_tti_req[slot];
     * nfapi_nr_pusch_pdu_t *pusch_pdu = &UL_tti_req->pdus_list[0].pusch_pdu;
     * 
     * pusch_pdu->rnti = scheduler->ue_context.rnti;
     * pusch_pdu->resource_alloc = 1; // Type 1
     * pusch_pdu->rb_start = scheduler->ue_context.gf_config.start_prb;
     * pusch_pdu->rb_size = scheduler->ue_context.gf_config.num_prb;
     * pusch_pdu->start_symbol_index = scheduler->ue_context.gf_config.start_symbol;
     * pusch_pdu->nr_of_symbols = scheduler->ue_context.gf_config.num_symbols;
     * pusch_pdu->mcs_index = scheduler->ue_context.gf_config.mcs;
     * pusch_pdu->harq_process_id = scheduler->ue_context.harq_process_id;
     * pusch_pdu->rv_index = scheduler->ue_context.current_harq_round;
     */
}

/**
 * Process received PUSCH in grant-free mode
 * Called by PHY after attempting to decode PUSCH
 */
void nr_gf_process_pusch(grant_free_scheduler_t *scheduler,
                         uint16_t frame,
                         uint8_t slot,
                         bool crc_ok) {
    if (scheduler == NULL || !scheduler->initialized) {
        return;
    }
    
    printf("[GF-SCHED] Frame=%d Slot=%d: Processing grant-free PUSCH reception\n",
           frame, slot);
    
    if (crc_ok) {
        // Successful reception
        scheduler->ue_context.data_received++;
        scheduler->ue_context.current_harq_round = 0;
        
        // Move to next HARQ process
        scheduler->ue_context.harq_process_id = 
            (scheduler->ue_context.harq_process_id + 1) % 8;
        
        printf("[GF-SCHED]   CRC OK - Data received successfully\n");
        printf("[GF-SCHED]   Total successful receptions: %d / %d (%.1f%%)\n",
               scheduler->ue_context.data_received,
               scheduler->ue_context.tx_opportunities,
               100.0 * scheduler->ue_context.data_received / 
               (scheduler->ue_context.tx_opportunities + 0.001));
        
        /*
         * Here you would:
         * 1. Extract MAC SDUs from the PUSCH
         * 2. Pass to upper layers (RLC)
         * 3. Update any adaptive parameters (MCS, power control)
         */
        
    } else {
        // CRC failure - HARQ retransmission needed
        scheduler->ue_context.crc_errors++;
        scheduler->ue_context.current_harq_round++;
        
        printf("[GF-SCHED]   CRC FAILED - HARQ round %d / %d\n",
               scheduler->ue_context.current_harq_round,
               scheduler->ue_context.gf_config.max_harq_rounds);
        
        if (scheduler->ue_context.current_harq_round >= 
            scheduler->ue_context.gf_config.max_harq_rounds) {
            // Max retransmissions reached - give up
            printf("[GF-SCHED]   Max HARQ rounds reached - discarding TB\n");
            scheduler->ue_context.current_harq_round = 0;
            scheduler->ue_context.harq_process_id = 
                (scheduler->ue_context.harq_process_id + 1) % 8;
        }
        
        /*
         * For HARQ retransmission:
         * 1. Keep the same HARQ process ID
         * 2. Increment redundancy version (RV)
         * 3. Store soft bits for combining
         * 4. Wait for next transmission opportunity
         */
    }
}

/**
 * Get and print statistics
 */
void nr_gf_get_statistics(grant_free_scheduler_t *scheduler) {
    if (scheduler == NULL || !scheduler->initialized) {
        printf("[GF-SCHED] Scheduler not initialized\n");
        return;
    }
    
    printf("\n========================================\n");
    printf("  Grant-Free Scheduler Statistics\n");
    printf("========================================\n");
    printf("UE RNTI:           0x%04X\n", scheduler->ue_context.rnti);
    printf("TX Opportunities:  %d\n", scheduler->ue_context.tx_opportunities);
    printf("Data Received:     %d\n", scheduler->ue_context.data_received);
    printf("CRC Errors:        %d\n", scheduler->ue_context.crc_errors);
    
    if (scheduler->ue_context.tx_opportunities > 0) {
        float success_rate = 100.0 * scheduler->ue_context.data_received / 
                            scheduler->ue_context.tx_opportunities;
        float error_rate = 100.0 * scheduler->ue_context.crc_errors / 
                          scheduler->ue_context.tx_opportunities;
        printf("Success Rate:      %.2f%%\n", success_rate);
        printf("Error Rate:        %.2f%%\n", error_rate);
    }
    printf("========================================\n\n");
}

/**
 * Reset scheduler
 */
void nr_gf_scheduler_reset(grant_free_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        return;
    }
    
    scheduler->ue_context.tx_opportunities = 0;
    scheduler->ue_context.data_received = 0;
    scheduler->ue_context.crc_errors = 0;
    scheduler->ue_context.current_harq_round = 0;
    scheduler->ue_context.harq_process_id = 0;
    
    printf("[GF-SCHED] Scheduler statistics reset\n");
}
