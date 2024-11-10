#pragma once

#include <inttypes.h>

struct sgv_index_entry{

	char			key[256];
	uint32_t		loc;
	uint32_t		length;

};