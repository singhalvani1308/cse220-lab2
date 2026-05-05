
#ifndef MISS_CLASSIFIER_3C_H
#define MISS_CLASSIFIER_3C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void miss_classifier_3c_init(uint64_t cache_size_bytes,
                              uint32_t assoc,
                              uint32_t line_size_bytes);

void classify_dcache_miss(uint64_t addr);

void miss_classifier_3c_dump_stats(void);

uint64_t miss_classifier_get_compulsory(void);
uint64_t miss_classifier_get_capacity(void);
uint64_t miss_classifier_get_conflict(void);
uint64_t miss_classifier_get_total(void);


#ifdef __cplusplus
}
#endif

#endif
