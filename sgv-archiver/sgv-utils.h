#pragma once

#include <string>
#include "sgv-index-entry.h"

void printHelp();
uint32_t hashString(const std::string& string, char hash_function, uint32_t size, uint32_t modifier);
double calculateSpread(const sgv_index_entry* index_entries, size_t  index_size);