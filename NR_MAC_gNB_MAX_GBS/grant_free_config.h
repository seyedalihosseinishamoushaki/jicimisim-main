#ifndef __GRANT_FREE_CONFIG_H__
#define __GRANT_FREE_CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

/* Grant-Free Configuration Structure */
typedef struct {
    /* Enable/disable grant-free scheduling */
    bool enabled;
    
    /* Resource allocation parameters */
    uint16_t start_prb;           // Starting PRB index
    uint16_t num_prb;             // Number of PRBs allocated (all available)
    
    /* Time domain parameters */
    uint8_t periodicity_slots;    // Periodicity in slots (e.g., every N slots)
    uint8_t start_symbol;         // Starting OFDM symbol in slot
    uint8_t num_symbols;          // Number of OFDM symbols
    
    /* Transmission parameters */
    uint8_t mcs;                  // Modulation and Coding Scheme
    uint8_t max_harq_rounds;      // Maximum HARQ retransmissions
    
    /* Advanced parameters */
    bool adaptive_mcs;            // Enable MCS adaptation based on CQI
    uint8_t dmrs_config;          // DMRS configuration
    uint32_t rnti;                // Radio Network Temporary Identifier for UE
} grant_free_config_t;

/* Initialize grant-free configuration with default values */
void init_grant_free_config(grant_free_config_t *config);

/* Validate grant-free configuration */
bool validate_grant_free_config(grant_free_config_t *config, uint16_t max_prbs);

/* Print grant-free configuration for debugging */
void print_grant_free_config(grant_free_config_t *config);

#endif /* __GRANT_FREE_CONFIG_H__ */