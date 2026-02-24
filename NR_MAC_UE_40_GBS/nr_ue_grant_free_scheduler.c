#include "nr_ue_grant_free_scheduler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Get redundancy version based on HARQ round */
static uint8_t get_rv_index(uint8_t harq_round) {
    const uint8_t rv_sequence[4] = {0, 2, 3, 1};
    return rv_sequence[harq_round % 4];
}

/**
 * Initialize the UE grant-free scheduler
 */
void nr_ue_gf_scheduler_init(ue_grant_free_scheduler_t *scheduler,
                              uint16_t max_prbs,
                              uint8_t numerology,
                              ue_grant_free_config_t *config) {
    if (scheduler == NULL) {
        printf("[UE-GF-SCHED] Error: NULL scheduler pointer\n");
        return;
    }
    
    memset(scheduler, 0, sizeof(ue_grant_free_scheduler_t));
    
    scheduler->max_prbs = max_prbs;
    scheduler->numerology = numerology;
    scheduler->state = UE_GF_STATE_IDLE;
    
    // Initialize HARQ processes
    for (int i = 0; i < 8; i++) {
        scheduler->harq_processes[i].harq_pid = i;
        scheduler->harq_processes[i].active = false;
        scheduler->harq_processes[i].current_round = 0;
        scheduler->harq_processes[i].ndi = 0;
        scheduler->harq_processes[i].mac_pdu = NULL;
        scheduler->harq_processes[i].pdu_length = 0;
    }
    scheduler->current_harq_pid = 0;
    
    // Configure if config provided
    if (config != NULL) {
        if (nr_ue_gf_configure(scheduler, config)) {
            printf("[UE-GF-SCHED] Scheduler initialized successfully\n");
            ue_print_grant_free_config(&scheduler->config);
        } else {
            printf("[UE-GF-SCHED] Error: Configuration failed\n");
        }
    } else {
        printf("[UE-GF-SCHED] Warning: Initialized without configuration\n");
    }
}

/**
 * Configure UE for grant-free transmission
 */
bool nr_ue_gf_configure(ue_grant_free_scheduler_t *scheduler,
                        ue_grant_free_config_t *config) {
    if (scheduler == NULL || config == NULL) {
        printf("[UE-GF-SCHED] Error: NULL pointer in configure\n");
        return false;
    }
    
    // Validate configuration
    if (!ue_validate_grant_free_config(config, scheduler->max_prbs)) {
        printf("[UE-GF-SCHED] Error: Invalid configuration\n");
        return false;
    }
    
    // Calculate TBS if not already done
    if (config->tbs == 0) {
        config->tbs = ue_gf_calculate_tbs(config);
    }
    
    // Store configuration
    memcpy(&scheduler->config, config, sizeof(ue_grant_free_config_t));
    scheduler->configured = true;
    
    // Allocate transmission buffer (TBS + some overhead)
    scheduler->tx_buffer_size = (config->tbs / 8) + 100;  // TBS in bytes + overhead
    scheduler->tx_buffer = (uint8_t*)malloc(scheduler->tx_buffer_size);
    
    if (scheduler->tx_buffer == NULL) {
        printf("[UE-GF-SCHED] Error: Failed to allocate TX buffer\n");
        scheduler->configured = false;
        return false;
    }
    
    // Allocate buffers for HARQ processes
    for (int i = 0; i < 8; i++) {
        scheduler->harq_processes[i].mac_pdu = (uint8_t*)malloc(scheduler->tx_buffer_size);
        if (scheduler->harq_processes[i].mac_pdu == NULL) {
            printf("[UE-GF-SCHED] Error: Failed to allocate HARQ buffer %d\n", i);
            // Clean up previously allocated buffers
            for (int j = 0; j < i; j++) {
                free(scheduler->harq_processes[j].mac_pdu);
            }
            free(scheduler->tx_buffer);
            scheduler->configured = false;
            return false;
        }
    }
    
    printf("[UE-GF-SCHED] Configured for grant-free operation\n");
    printf("[UE-GF-SCHED]   RNTI: 0x%04X\n", config->rnti);
    printf("[UE-GF-SCHED]   TBS: %u bytes\n", config->tbs / 8);
    printf("[UE-GF-SCHED]   TX buffer: %u bytes\n", scheduler->tx_buffer_size);
    
    return true;
}

/**
 * Check if current slot is a grant-free transmission opportunity
 */
bool nr_ue_gf_is_tx_slot(ue_grant_free_scheduler_t *scheduler,
                         uint16_t frame,
                         uint8_t slot) {
    if (scheduler == NULL || !scheduler->configured) {
        return false;
    }
    
    if (!scheduler->config.enabled) {
        return false;
    }
    
    uint8_t periodicity = scheduler->config.periodicity_slots;
    
    // Calculate slots per frame based on numerology
    uint8_t slots_per_frame = 10 * (1 << scheduler->numerology);
    
    // Calculate absolute slot number
    uint32_t abs_slot = frame * slots_per_frame + slot;
    
    // Check if this is a transmission opportunity based on periodicity
    return (abs_slot % periodicity) == 0;
}

/**
 * Add data to transmission buffer (called by upper layers)
 */
bool nr_ue_gf_add_data(ue_grant_free_scheduler_t *scheduler,
                       uint8_t *data,
                       uint32_t length) {
    if (scheduler == NULL || !scheduler->configured) {
        printf("[UE-GF-SCHED] Error: Scheduler not configured\n");
        return false;
    }
    
    if (data == NULL || length == 0) {
        printf("[UE-GF-SCHED] Error: Invalid data\n");
        return false;
    }
    
    // Check if buffer has space
    if (scheduler->data_available + length > scheduler->tx_buffer_size) {
        printf("[UE-GF-SCHED] Warning: Buffer full, dropping %u bytes\n", length);
        return false;
    }
    
    // Copy data to buffer
    memcpy(scheduler->tx_buffer + scheduler->data_available, data, length);
    scheduler->data_available += length;
    
    // Update state
    if (scheduler->state == UE_GF_STATE_IDLE) {
        scheduler->state = UE_GF_STATE_WAIT_TX_OPPORTUNITY;
    }
    
    printf("[UE-GF-SCHED] Added %u bytes to TX buffer (total: %u bytes)\n",
           length, scheduler->data_available);
    
    return true;
}

/**
 * Main UE scheduling function - called every slot
 * Returns true if PUSCH transmission should occur
 */
bool nr_ue_gf_schedule_pusch(ue_grant_free_scheduler_t *scheduler,
                              uint16_t frame,
                              uint8_t slot) {
    if (scheduler == NULL || !scheduler->configured) {
        return false;
    }
    
    // Update current frame/slot
    scheduler->current_frame = frame;
    scheduler->current_slot = slot;
    
    // Check if we have data to transmit
    if (scheduler->data_available == 0 && scheduler->state != UE_GF_STATE_RETRANSMIT) {
        return false;
    }
    
    // Check if this is a transmission opportunity
    if (!nr_ue_gf_is_tx_slot(scheduler, frame, slot)) {
        return false;
    }
    
    // This is a transmission opportunity!
    printf("[UE-GF-SCHED] Frame=%d Slot=%d: Grant-free TX opportunity\n", frame, slot);
    
    bool should_transmit = false;
    
    switch (scheduler->state) {
        case UE_GF_STATE_WAIT_TX_OPPORTUNITY:
        case UE_GF_STATE_IDLE:
            if (scheduler->data_available > 0) {
                // Start new transmission
                ue_gf_harq_process_t *harq = &scheduler->harq_processes[scheduler->current_harq_pid];
                
                // Prepare MAC PDU
                uint32_t pdu_size = (scheduler->config.tbs / 8);  // TBS in bytes
                if (scheduler->data_available < pdu_size) {
                    pdu_size = scheduler->data_available;
                }
                
                memcpy(harq->mac_pdu, scheduler->tx_buffer, pdu_size);
                harq->pdu_length = pdu_size;
                harq->active = true;
                harq->current_round = 0;
                harq->ndi++;  // Toggle NDI for new transmission
                harq->tx_frame = frame;
                harq->tx_slot = slot;
                
                // Remove data from buffer
                memmove(scheduler->tx_buffer, 
                       scheduler->tx_buffer + pdu_size,
                       scheduler->data_available - pdu_size);
                scheduler->data_available -= pdu_size;
                
                scheduler->state = UE_GF_STATE_TRANSMITTING;
                scheduler->tx_attempts++;
                should_transmit = true;
                
                printf("[UE-GF-SCHED]   New transmission: HARQ PID=%d, PDU size=%u bytes\n",
                       scheduler->current_harq_pid, pdu_size);
            }
            break;
            
        case UE_GF_STATE_RETRANSMIT:
            // HARQ retransmission
            {
                ue_gf_harq_process_t *harq = &scheduler->harq_processes[scheduler->current_harq_pid];
                
                if (harq->active && harq->current_round < scheduler->config.max_harq_rounds) {
                    harq->current_round++;
                    harq->tx_frame = frame;
                    harq->tx_slot = slot;
                    
                    scheduler->state = UE_GF_STATE_TRANSMITTING;
                    scheduler->tx_attempts++;
                    scheduler->harq_retransmissions++;
                    should_transmit = true;
                    
                    printf("[UE-GF-SCHED]   HARQ retransmission: PID=%d, Round=%d\n",
                           scheduler->current_harq_pid, harq->current_round);
                } else {
                    // Max retransmissions reached
                    printf("[UE-GF-SCHED]   Max HARQ rounds reached, discarding TB\n");
                    harq->active = false;
                    scheduler->tx_failures++;
                    
                    // Move to next HARQ process
                    scheduler->current_harq_pid = (scheduler->current_harq_pid + 1) % 8;
                    
                    if (scheduler->data_available > 0) {
                        scheduler->state = UE_GF_STATE_WAIT_TX_OPPORTUNITY;
                    } else {
                        scheduler->state = UE_GF_STATE_IDLE;
                    }
                }
            }
            break;
            
        case UE_GF_STATE_TRANSMITTING:
        case UE_GF_STATE_WAIT_FEEDBACK:
            // Waiting for previous transmission to complete
            break;
    }
    
    if (should_transmit) {
        scheduler->last_tx_frame = frame;
        scheduler->last_tx_slot = slot;
    }
    
    return should_transmit;
}

/**
 * Get transmission parameters for PHY
 */
bool nr_ue_gf_get_tx_params(ue_grant_free_scheduler_t *scheduler,
                             ue_gf_tx_params_t *tx_params) {
    if (scheduler == NULL || tx_params == NULL || !scheduler->configured) {
        return false;
    }
    
    ue_gf_harq_process_t *harq = &scheduler->harq_processes[scheduler->current_harq_pid];
    
    if (!harq->active) {
        return false;
    }
    
    // Fill in transmission parameters
    tx_params->start_prb = scheduler->config.start_prb;
    tx_params->num_prb = scheduler->config.num_prb;
    tx_params->start_symbol = scheduler->config.start_symbol;
    tx_params->num_symbols = scheduler->config.num_symbols;
    tx_params->mcs = scheduler->config.mcs;
    tx_params->harq_pid = scheduler->current_harq_pid;
    tx_params->rv_index = get_rv_index(harq->current_round);
    tx_params->tx_power_dbm = scheduler->config.target_power_dbm;
    tx_params->rnti = scheduler->config.rnti;
    tx_params->mac_pdu = harq->mac_pdu;
    tx_params->pdu_length = harq->pdu_length;
    
    printf("[UE-GF-SCHED] TX Parameters:\n");
    printf("[UE-GF-SCHED]   PRBs: %d-%d\n", 
           tx_params->start_prb, tx_params->start_prb + tx_params->num_prb - 1);
    printf("[UE-GF-SCHED]   Symbols: %d-%d\n",
           tx_params->start_symbol, tx_params->start_symbol + tx_params->num_symbols - 1);
    printf("[UE-GF-SCHED]   MCS: %d, RV: %d\n", tx_params->mcs, tx_params->rv_index);
    printf("[UE-GF-SCHED]   HARQ: PID=%d, Round=%d\n", 
           tx_params->harq_pid, harq->current_round);
    printf("[UE-GF-SCHED]   Power: %d dBm\n", tx_params->tx_power_dbm);
    
    return true;
}

/**
 * Process feedback (if any) - e.g., from PDCCH indicating ACK/NACK
 * In pure grant-free, there might be no feedback
 */
void nr_ue_gf_process_feedback(ue_grant_free_scheduler_t *scheduler,
                                uint8_t harq_pid,
                                bool ack) {
    if (scheduler == NULL || !scheduler->configured) {
        return;
    }
    
    if (harq_pid >= 8) {
        printf("[UE-GF-SCHED] Error: Invalid HARQ PID %d\n", harq_pid);
        return;
    }
    
    ue_gf_harq_process_t *harq = &scheduler->harq_processes[harq_pid];
    
    if (!harq->active) {
        printf("[UE-GF-SCHED] Warning: Feedback for inactive HARQ PID %d\n", harq_pid);
        return;
    }
    
    printf("[UE-GF-SCHED] Feedback for HARQ PID %d: %s\n", 
           harq_pid, ack ? "ACK" : "NACK");
    
    if (ack) {
        // Successful transmission
        scheduler->tx_success++;
        harq->active = false;
        harq->current_round = 0;
        
        // Move to next HARQ process
        scheduler->current_harq_pid = (scheduler->current_harq_pid + 1) % 8;
        
        // Update state
        if (scheduler->data_available > 0) {
            scheduler->state = UE_GF_STATE_WAIT_TX_OPPORTUNITY;
        } else {
            scheduler->state = UE_GF_STATE_IDLE;
        }
        
        printf("[UE-GF-SCHED]   Transmission successful\n");
        
    } else {
        // NACK - need retransmission
        if (harq->current_round < scheduler->config.max_harq_rounds - 1) {
            scheduler->state = UE_GF_STATE_RETRANSMIT;
            printf("[UE-GF-SCHED]   Scheduling retransmission\n");
        } else {
            // Max retransmissions reached
            printf("[UE-GF-SCHED]   Max retransmissions reached, discarding\n");
            scheduler->tx_failures++;
            harq->active = false;
            
            scheduler->current_harq_pid = (scheduler->current_harq_pid + 1) % 8;
            
            if (scheduler->data_available > 0) {
                scheduler->state = UE_GF_STATE_WAIT_TX_OPPORTUNITY;
            } else {
                scheduler->state = UE_GF_STATE_IDLE;
            }
        }
    }
}

/**
 * Get and print statistics
 */
void nr_ue_gf_get_statistics(ue_grant_free_scheduler_t *scheduler) {
    if (scheduler == NULL || !scheduler->configured) {
        printf("[UE-GF-SCHED] Scheduler not configured\n");
        return;
    }
    
    printf("\n========================================\n");
    printf("  UE Grant-Free Scheduler Statistics\n");
    printf("========================================\n");
    printf("RNTI:              0x%04X\n", scheduler->config.rnti);
    printf("State:             %d\n", scheduler->state);
    printf("TX Attempts:       %u\n", scheduler->tx_attempts);
    printf("TX Success:        %u\n", scheduler->tx_success);
    printf("TX Failures:       %u\n", scheduler->tx_failures);
    printf("HARQ Retrans:      %u\n", scheduler->harq_retransmissions);
    printf("Data in Buffer:    %u bytes\n", scheduler->data_available);
    
    if (scheduler->tx_attempts > 0) {
        float success_rate = 100.0 * scheduler->tx_success / scheduler->tx_attempts;
        printf("Success Rate:      %.2f%%\n", success_rate);
    }
    
    // HARQ process status
    printf("\nHARQ Processes:\n");
    for (int i = 0; i < 8; i++) {
        if (scheduler->harq_processes[i].active) {
            printf("  PID %d: Active, Round=%d, NDI=%u\n",
                   i,
                   scheduler->harq_processes[i].current_round,
                   scheduler->harq_processes[i].ndi);
        }
    }
    printf("========================================\n\n");
}

/**
 * Reset scheduler statistics
 */
void nr_ue_gf_scheduler_reset(ue_grant_free_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        return;
    }
    
    scheduler->tx_attempts = 0;
    scheduler->tx_success = 0;
    scheduler->tx_failures = 0;
    scheduler->harq_retransmissions = 0;
    scheduler->data_available = 0;
    scheduler->state = UE_GF_STATE_IDLE;
    
    // Reset HARQ processes
    for (int i = 0; i < 8; i++) {
        scheduler->harq_processes[i].active = false;
        scheduler->harq_processes[i].current_round = 0;
    }
    
    printf("[UE-GF-SCHED] Scheduler reset\n");
}

/**
 * Clean up and free resources
 */
void nr_ue_gf_scheduler_cleanup(ue_grant_free_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        return;
    }
    
    // Free TX buffer
    if (scheduler->tx_buffer != NULL) {
        free(scheduler->tx_buffer);
        scheduler->tx_buffer = NULL;
    }
    
    // Free HARQ buffers
    for (int i = 0; i < 8; i++) {
        if (scheduler->harq_processes[i].mac_pdu != NULL) {
            free(scheduler->harq_processes[i].mac_pdu);
            scheduler->harq_processes[i].mac_pdu = NULL;
        }
    }
    
    scheduler->configured = false;
    
    printf("[UE-GF-SCHED] Cleanup complete\n");
}