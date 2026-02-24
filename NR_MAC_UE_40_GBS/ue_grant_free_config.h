#ifndef __UE_GRANT_FREE_CONFIG_H__
#define __UE_GRANT_FREE_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

/* UE Grant-Free Configuration Structure */
typedef struct {
    /* Enable/disable grant-free transmission */
    bool enabled;
    
    /* Resource allocation parameters - must match gNB configuration */
    uint16_t start_prb;           // Starting PRB index
    uint16_t num_prb;             // Number of PRBs allocated
    
    /* Time domain parameters */
    uint8_t periodicity_slots;    // Periodicity in slots (e.g., every N slots)
    uint8_t start_symbol;         // Starting OFDM symbol in slot
    uint8_t num_symbols;          // Number of OFDM symbols
    
    /* Transmission parameters */
    uint8_t mcs;                  // Modulation and Coding Scheme
    uint8_t max_harq_rounds;      // Maximum HARQ retransmissions
    int8_t target_power_dbm;      // Target transmission power
    
    /* Advanced parameters */
    uint8_t dmrs_config;          // DMRS configuration
    uint32_t rnti;                // Radio Network Temporary Identifier
    
    /* Transport Block Size */
    uint32_t tbs;                 // Calculated transport block size
    
} ue_grant_free_config_t;

/* Initialize UE grant-free configuration with default values */
void ue_init_grant_free_config(ue_grant_free_config_t *config);

/* Validate UE grant-free configuration */
bool ue_validate_grant_free_config(ue_grant_free_config_t *config, uint16_t max_prbs);

/* Calculate Transport Block Size based on configuration */
uint32_t ue_gf_calculate_tbs(ue_grant_free_config_t *config);

/* Print UE grant-free configuration for debugging */
void ue_print_grant_free_config(ue_grant_free_config_t *config);

#endif /* __UE_GRANT_FREE_CONFIG_H__ */