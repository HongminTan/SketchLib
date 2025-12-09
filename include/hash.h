#ifndef HASH_H
#define HASH_H

#include "FlowKey.h"
#include "crc32.h"
#include "seed_list.h"

#ifdef __cplusplus
extern "C" {
#endif



uint32_t hash(const FlowKeyType* flow, uint64_t seed, uint64_t mod);

#ifdef __cplusplus
}
#endif

#endif  // HASH_H
