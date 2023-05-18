#pragma once

#include <stdint.h>
#include <stddef.h>

// Murmur3 implementation courtesy of Demetri Spanos

static inline uint32_t RT_Rotl32(uint32_t x, int8_t r) 
{ 
    return (x << r) | (x >> (32 - r)); 
}

static inline uint32_t RT_FMix(uint32_t h ) 
{
	h ^= h >> 16; h *= 0x85ebca6b;
	h ^= h >> 13; h *= 0xc2b2ae35;
	return h ^= h >> 16;
}

static inline uint32_t RT_Murmur3(const void *key, uint32_t len, uint32_t h1) 
{
	const uint8_t *tail = (const uint8_t *)key + (len/4)*4; // handle this separately

	uint32_t c1 = 0xcc9e2d51, c2 = 0x1b873593;

	// body (full 32-bit blocks) handled uniformly
	for (uint32_t *p = (uint32_t *)key; p < (const uint32_t *)tail; p++)
	{
		uint32_t k1 = *p; k1 *= c1; k1 = RT_Rotl32(k1, 15); k1 *= c2; // MUR1
		h1 ^= k1; h1 = RT_Rotl32(h1, 13); h1 = h1*5+0xe6546b64; // MUR2
	}

	uint32_t t = 0; // handle up to 3 tail bytes
	switch (len & 3)
	{
		case 3: t ^= tail[2] << 16;
		case 2: t ^= tail[1] << 8;
		case 1: {t ^= tail[0]; t *= c1; t = RT_Rotl32(t, 15); t *= c2; h1 ^= t; };
	}
	return RT_FMix(h1 ^ len);
}