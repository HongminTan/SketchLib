#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32(const unsigned char* s, size_t len);

#endif  // CRC32_H
