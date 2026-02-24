#ifndef __NR_MAC_GNB_GRANT_FREE_SCHEDULER_H__
#define __NR_MAC_GNB_GRANT_FREE_SCHEDULER_H__

#include "grant_free_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Grant-Free UE Context */
typedef struct {
    uint32_t rnti;
    grant_free_config_t gf_config;
    
    /* Scheduling state */
    bool resources_configured;
    uint16_t current_frame;
    uint8_t current_slot;
    
    /* Statistics */
    uint32_t tx_opportunities;    // Number of transmission opportunities
    uint32_t data_received;       // Number of successful receptions
    uint32_t crc_errors;          // Number of CRC errors
    
    /* HARQ management */
    uint8_t harq_process_id;
    uint8_t current_harq_round;
    
} grant_free_ue_context_t;

/* Grant-Free Scheduler Context */
typedef struct {
    grant_free_ue_context_t ue_context;
    bool initialized;
    
    /* System parameters */
    uint16_t max_prbs;
    uint8_t numerology;  // SCS: 0=15kHz, 1=30kHz, 2=60kHz, 3=120kHz
    
} grant_free_scheduler_t;

/* Initialize the grant-free scheduler */
void nr_gf_scheduler_init(grant_free_scheduler_t *scheduler,
                          uint16_t max_prbs,
                          uint8_t numerology,
                          grant_free_config_t *config);

/* Configure resources for a UE in grant-free mode */
bool nr_gf_configure_ue(grant_free_scheduler_t *scheduler,
                        uint32_t rnti,
                        grant_free_config_t *config);

/* Main scheduling function - called every slot */
void nr_gf_schedule_ulsch(grant_free_scheduler_t *scheduler,
                          uint16_t frame,
                          uint8_t slot);

/* Check if current slot is a grant-free transmission opportunity */
bool nr_gf_is_tx_slot(grant_free_scheduler_t *scheduler,
                      uint16_t frame,
                      uint8_t slot);

/* Process received PUSCH in grant-free mode */
void nr_gf_process_pusch(grant_free_scheduler_t *scheduler,
                         uint16_t frame,
                         uint8_t slot,
                         bool crc_ok);

/* Get statistics */
void nr_gf_get_statistics(grant_free_scheduler_t *scheduler);

/* Reset scheduler */
void nr_gf_scheduler_reset(grant_free_scheduler_t *scheduler);

#endif /* __NR_MAC_GNB_GRANT_FREE_SCHEDULER_H__ */