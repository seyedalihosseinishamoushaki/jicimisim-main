#include "grant_free_config.h"
#include <stdio.h>
#include <string.h>

/**
 * Initialize grant-free configuration with default values
 * These defaults allocate all resources to a single UE
 */
void init_grant_free_config(grant_free_config_t *config) {
    if (config == NULL) {
        printf("[GRANT-FREE] Error: NULL config pointer\n");
        return;
    }
    
    memset(config, 0, sizeof(grant_free_config_t));
    
    // Default configuration for basic grant-free operation
    config->enabled = true;
    
    // Allocate all available resources
    config->start_prb = 0;
    config->num_prb = 106;  // Default for 100MHz BW, will be adjusted based on config
    
    // Time domain: allocate full slot periodically
    config->periodicity_slots = 1;  // Every slot (most aggressive)
    config->start_symbol = 0;       // Start from first symbol
    config->num_symbols = 14;       // All symbols in a slot (adjust for PDCCH/PUCCH)
    
    // Use moderate MCS for reliability
    config->mcs = 16;  // This gives good balance between throughput and reliability
    config->max_harq_rounds = 4;
    
    // Advanced features disabled by default
    config->adaptive_mcs = false;
    config->dmrs_config = 0;  // Default DMRS configuration
    config->rnti = 0;    // Default RNTI, should be set properly
    
    printf("[GRANT-FREE] Initialized with default configuration\n");
}

/**
 * Validate grant-free configuration parameters
 * Returns true if configuration is valid, false otherwise
 */
bool validate_grant_free_config(grant_free_config_t *config, uint16_t max_prbs) {
    if (config == NULL) {
        printf("[GRANT-FREE] Error: NULL config pointer\n");
        return false;
    }
    
    // Check PRB allocation
    if (config->start_prb + config->num_prb > max_prbs) {
        printf("[GRANT-FREE] Error: PRB allocation exceeds maximum (%d + %d > %d)\n",
               config->start_prb, config->num_prb, max_prbs);
        return false;
    }
    
    if (config->num_prb == 0) {
        printf("[GRANT-FREE] Error: Number of PRBs cannot be zero\n");
        return false;
    }
    
    // Check time domain allocation
    if (config->start_symbol + config->num_symbols > 14) {
        printf("[GRANT-FREE] Error: Symbol allocation exceeds slot boundary (%d + %d > 14)\n",
               config->start_symbol, config->num_symbols);
        return false;
    }
    
    if (config->num_symbols == 0) {
        printf("[GRANT-FREE] Error: Number of symbols cannot be zero\n");
        return false;
    }
    
    // Check MCS range (0-28 for NR)
    if (config->mcs > 28) {
        printf("[GRANT-FREE] Warning: MCS %d exceeds maximum (28), capping\n", config->mcs);
        config->mcs = 28;
    }
    
    // Check periodicity
    if (config->periodicity_slots == 0) {
        printf("[GRANT-FREE] Error: Periodicity cannot be zero\n");
        return false;
    }
    
    // Check HARQ rounds
    if (config->max_harq_rounds > 8 || config->max_harq_rounds == 0) {
        printf("[GRANT-FREE] Warning: HARQ rounds %d out of range [1,8], adjusting\n",
               config->max_harq_rounds);
        config->max_harq_rounds = 4;
    }
    
    printf("[GRANT-FREE] Configuration validated successfully\n");
    return true;
}

/**
 * Print grant-free configuration for debugging
 */
void print_grant_free_config(grant_free_config_t *config) {
    if (config == NULL) {
        printf("[GRANT-FREE] Error: NULL config pointer\n");
        return;
    }
    
    printf("\n========================================\n");
    printf("  Grant-Free Scheduler Configuration\n");
    printf("========================================\n");
    printf("Enabled:           %s\n", config->enabled ? "YES" : "NO");
    printf("\nFrequency Domain:\n");
    printf("  Start PRB:       %d\n", config->start_prb);
    printf("  Number of PRBs:  %d\n", config->num_prb);
    printf("  Total BW:        %.2f MHz (approx)\n", config->num_prb * 0.18); // ~180kHz per PRB
    printf("\nTime Domain:\n");
    printf("  Periodicity:     %d slots\n", config->periodicity_slots);
    printf("  Start Symbol:    %d\n", config->start_symbol);
    printf("  Number Symbols:  %d\n", config->num_symbols);
    printf("\nTransmission:\n");
    printf("  MCS:             %d\n", config->mcs);
    printf("  Adaptive MCS:    %s\n", config->adaptive_mcs ? "YES" : "NO");
    printf("  Max HARQ Rounds: %d\n", config->max_harq_rounds);
    printf("  DMRS Config:     %d\n", config->dmrs_config);
    printf("  RNTI:            0x%04X\n", config->rnti);
    printf("========================================\n\n");
}
