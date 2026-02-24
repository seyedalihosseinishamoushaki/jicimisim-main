#include "ue_grant_free_config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/**
 * Initialize UE grant-free configuration with default values
 * These defaults should match the gNB configuration
 */
void ue_init_grant_free_config(ue_grant_free_config_t *config) {
    if (config == NULL) {
        printf("[UE-GF] Error: NULL config pointer\n");
        return;
    }
    
    memset(config, 0, sizeof(ue_grant_free_config_t));
    
    // Default configuration - MUST match gNB settings
    config->enabled = true;
    
    // Resource allocation - match gNB
    config->start_prb = 0;
    config->num_prb = 106;  // Default for 100MHz BW
    
    // Time domain - match gNB
    config->periodicity_slots = 1;  // Every slot
    config->start_symbol = 2;       // After PDCCH
    config->num_symbols = 12;       // Remaining symbols
    
    // Transmission parameters
    config->mcs = 16;  // Moderate MCS
    config->max_harq_rounds = 4;
    config->target_power_dbm = 23;  // 23 dBm (200mW)
    
    // Advanced parameters
    config->dmrs_config = 0;
    config->rnti = 0xFFFE;  // Should be assigned by gNB
    
    // Calculate TBS
    config->tbs = ue_gf_calculate_tbs(config);
    
    printf("[UE-GF] Initialized with default configuration\n");
}

/**
 * Simplified TBS calculation for grant-free
 * In real implementation, use 3GPP TS 38.214 formulas
 */
uint32_t ue_gf_calculate_tbs(ue_grant_free_config_t *config) {
    if (config == NULL) {
        return 0;
    }
    
    // Simplified TBS calculation
    // Real calculation from TS 38.214 Section 5.1.3.2
    
    // Number of REs per PRB per symbol = 12 (subcarriers)
    // Minus DMRS overhead (assume 2 DMRS symbols, 6 REs per symbol)
    uint32_t dmrs_symbols = 2;
    uint32_t dmrs_overhead = dmrs_symbols * 6 * config->num_prb;
    
    // Total REs available
    uint32_t total_res = 12 * config->num_prb * config->num_symbols;
    uint32_t data_res = total_res - dmrs_overhead;
    
    // Get modulation order and code rate from MCS
    // Simplified mapping (real implementation uses MCS tables)
    uint8_t qm = 2;  // QPSK
    float code_rate = 0.5;
    
    if (config->mcs >= 10 && config->mcs < 17) {
        qm = 4;  // 16QAM
        code_rate = 0.4 + (config->mcs - 10) * 0.05;
    } else if (config->mcs >= 17 && config->mcs < 29) {
        qm = 6;  // 64QAM
        code_rate = 0.4 + (config->mcs - 17) * 0.04;
    }
    
    // TBS = number of information bits
    uint32_t tbs = (uint32_t)(data_res * qm * code_rate);
    
    // Round down to nearest byte
    tbs = (tbs / 8) * 8;
    
    printf("[UE-GF] Calculated TBS: %u bits (%u bytes)\n", tbs, tbs/8);
    printf("[UE-GF]   Data REs: %u, Qm: %u, Code Rate: %.2f\n", 
           data_res, qm, code_rate);
    
    return tbs;
}

/**
 * Validate UE grant-free configuration parameters
 */
bool ue_validate_grant_free_config(ue_grant_free_config_t *config, uint16_t max_prbs) {
    if (config == NULL) {
        printf("[UE-GF] Error: NULL config pointer\n");
        return false;
    }
    
    // Check PRB allocation
    if (config->start_prb + config->num_prb > max_prbs) {
        printf("[UE-GF] Error: PRB allocation exceeds maximum (%d + %d > %d)\n",
               config->start_prb, config->num_prb, max_prbs);
        return false;
    }
    
    if (config->num_prb == 0) {
        printf("[UE-GF] Error: Number of PRBs cannot be zero\n");
        return false;
    }
    
    // Check time domain allocation
    if (config->start_symbol + config->num_symbols > 14) {
        printf("[UE-GF] Error: Symbol allocation exceeds slot boundary (%d + %d > 14)\n",
               config->start_symbol, config->num_symbols);
        return false;
    }
    
    if (config->num_symbols == 0) {
        printf("[UE-GF] Error: Number of symbols cannot be zero\n");
        return false;
    }
    
    // Check MCS range (0-28 for NR)
    if (config->mcs > 28) {
        printf("[UE-GF] Warning: MCS %d exceeds maximum (28), capping\n", config->mcs);
        config->mcs = 28;
    }
    
    // Check periodicity
    if (config->periodicity_slots == 0) {
        printf("[UE-GF] Error: Periodicity cannot be zero\n");
        return false;
    }
    
    // Check HARQ rounds
    if (config->max_harq_rounds > 8 || config->max_harq_rounds == 0) {
        printf("[UE-GF] Warning: HARQ rounds %d out of range [1,8], adjusting\n",
               config->max_harq_rounds);
        config->max_harq_rounds = 4;
    }
    
    // Check power
    if (config->target_power_dbm > 23) {
        printf("[UE-GF] Warning: Power %d dBm exceeds maximum (23), capping\n",
               config->target_power_dbm);
        config->target_power_dbm = 23;
    }
    
    printf("[UE-GF] Configuration validated successfully\n");
    return true;
}

/**
 * Print UE grant-free configuration for debugging
 */
void ue_print_grant_free_config(ue_grant_free_config_t *config) {
    if (config == NULL) {
        printf("[UE-GF] Error: NULL config pointer\n");
        return;
    }
    
    printf("\n========================================\n");
    printf("  UE Grant-Free Configuration\n");
    printf("========================================\n");
    printf("Enabled:           %s\n", config->enabled ? "YES" : "NO");
    printf("\nFrequency Domain:\n");
    printf("  Start PRB:       %d\n", config->start_prb);
    printf("  Number of PRBs:  %d\n", config->num_prb);
    printf("  Total BW:        %.2f MHz (approx)\n", config->num_prb * 0.18);
    printf("\nTime Domain:\n");
    printf("  Periodicity:     %d slots\n", config->periodicity_slots);
    printf("  Start Symbol:    %d\n", config->start_symbol);
    printf("  Number Symbols:  %d\n", config->num_symbols);
    printf("\nTransmission:\n");
    printf("  MCS:             %d\n", config->mcs);
    printf("  Target Power:    %d dBm\n", config->target_power_dbm);
    printf("  Max HARQ Rounds: %d\n", config->max_harq_rounds);
    printf("  DMRS Config:     %d\n", config->dmrs_config);
    printf("  RNTI:            0x%04X\n", config->rnti);
    printf("  TBS:             %u bits (%u bytes)\n", config->tbs, config->tbs/8);
    printf("========================================\n\n");
}
