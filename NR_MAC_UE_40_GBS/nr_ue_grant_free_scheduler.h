#ifndef __NR_UE_GRANT_FREE_SCHEDULER_H__
#define __NR_UE_GRANT_FREE_SCHEDULER_H__

#include "ue_grant_free_config.h"
#include <stdint.h>
#include <stdbool.h>

/* UE Grant-Free Transmission State */
typedef enum {
    UE_GF_STATE_IDLE = 0,           // No data to transmit
    UE_GF_STATE_WAIT_TX_OPPORTUNITY, // Waiting for transmission slot
    UE_GF_STATE_TRANSMITTING,        // Currently transmitting
    UE_GF_STATE_WAIT_FEEDBACK,       // Waiting for feedback (optional)
    UE_GF_STATE_RETRANSMIT           // HARQ retransmission needed
} ue_gf_state_t;

/* UE Grant-Free HARQ Process */
typedef struct {
    uint8_t harq_pid;               // HARQ process ID (0-7)
    uint8_t current_round;          // Current HARQ round (0-3)
    bool active;                    // Is this HARQ process active
    uint32_t ndi;                   // New Data Indicator
    uint8_t *mac_pdu;               // Pointer to MAC PDU buffer
    uint32_t pdu_length;            // PDU length in bytes
    uint16_t tx_frame;              // Frame when transmitted
    uint8_t tx_slot;                // Slot when transmitted
} ue_gf_harq_process_t;

/* UE Grant-Free Scheduler Context */
typedef struct {
    /* Configuration */
    ue_grant_free_config_t config;
    bool configured;
    
    /* State */
    ue_gf_state_t state;
    uint16_t current_frame;
    uint8_t current_slot;
    
    /* System parameters */
    uint16_t max_prbs;
    uint8_t numerology;  // SCS: 0=15kHz, 1=30kHz, 2=60kHz, 3=120kHz
    
    /* HARQ management */
    ue_gf_harq_process_t harq_processes[8];  // 8 HARQ processes
    uint8_t current_harq_pid;
    
    /* Data buffer */
    uint8_t *tx_buffer;              // Transmission buffer
    uint32_t tx_buffer_size;         // Buffer size
    uint32_t data_available;         // Bytes available to transmit
    
    /* Statistics */
    uint32_t tx_attempts;            // Total transmission attempts
    uint32_t tx_success;             // Successful transmissions
    uint32_t tx_failures;            // Failed transmissions
    uint32_t harq_retransmissions;   // HARQ retransmissions
    
    /* Timing */
    uint32_t last_tx_frame;
    uint8_t last_tx_slot;
    
} ue_grant_free_scheduler_t;

/* Initialize the UE grant-free scheduler */
void nr_ue_gf_scheduler_init(ue_grant_free_scheduler_t *scheduler,
                              uint16_t max_prbs,
                              uint8_t numerology,
                              ue_grant_free_config_t *config);

/* Configure UE for grant-free transmission */
bool nr_ue_gf_configure(ue_grant_free_scheduler_t *scheduler,
                        ue_grant_free_config_t *config);

/* Check if current slot is a grant-free transmission opportunity */
bool nr_ue_gf_is_tx_slot(ue_grant_free_scheduler_t *scheduler,
                         uint16_t frame,
                         uint8_t slot);

/* Main UE scheduling function - called every slot */
/* Returns true if PUSCH transmission should occur */
bool nr_ue_gf_schedule_pusch(ue_grant_free_scheduler_t *scheduler,
                              uint16_t frame,
                              uint8_t slot);

/* Add data to transmission buffer (called by upper layers) */
bool nr_ue_gf_add_data(ue_grant_free_scheduler_t *scheduler,
                       uint8_t *data,
                       uint32_t length);

/* Get transmission parameters for PHY */
typedef struct {
    uint16_t start_prb;
    uint16_t num_prb;
    uint8_t start_symbol;
    uint8_t num_symbols;
    uint8_t mcs;
    uint8_t harq_pid;
    uint8_t rv_index;  // Redundancy version
    int8_t tx_power_dbm;
    uint32_t rnti;
    uint8_t *mac_pdu;
    uint32_t pdu_length;
} ue_gf_tx_params_t;

bool nr_ue_gf_get_tx_params(ue_grant_free_scheduler_t *scheduler,
                             ue_gf_tx_params_t *tx_params);

/* Process feedback (if any) - e.g., from PDCCH indicating ACK/NACK */
void nr_ue_gf_process_feedback(ue_grant_free_scheduler_t *scheduler,
                                uint8_t harq_pid,
                                bool ack);

/* Get statistics */
void nr_ue_gf_get_statistics(ue_grant_free_scheduler_t *scheduler);

/* Reset scheduler */
void nr_ue_gf_scheduler_reset(ue_grant_free_scheduler_t *scheduler);

/* Clean up and free resources */
void nr_ue_gf_scheduler_cleanup(ue_grant_free_scheduler_t *scheduler);

#endif /* __NR_UE_GRANT_FREE_SCHEDULER_H__ */