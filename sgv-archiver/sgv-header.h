#pragma once

#include <inttypes.h>

struct sgv_header{

	char			magic[4];							// SGV!
	char			version_major;
	char			version_minor;
	uint32_t		index_entries;
	uint32_t		index_size;
	char			index_hash_function;				// 0 = No Hash
														// 1 = Multiplication 1 : hash = ( hash + modifier * value ) % index_size;
	uint32_t		index_hash_function_modifier;
	
	

} ;