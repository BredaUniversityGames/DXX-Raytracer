#pragma once

#include <inttypes.h>

struct sgv_header{

	char			magic[4];							// SGV!
	char			version_major;
	char			version_minor;
	char			index_hash_function;				// 0 = No Hash
														// 1 = Multiplication 1 : hash = ( hash + modifier * value ) % index_size;
	uint32_t		index_hash_function_modifier;
	uint32_t		index_size;
	uint32_t		index_entries;

} ;