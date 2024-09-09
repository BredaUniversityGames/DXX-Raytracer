#pragma once

#include <string>
#include <vector>

#include "Common.h"

struct sgv_index_entry {

	char			key[256];
	uint32_t		loc;
	uint32_t		length;

};

struct sgv_header {
	char			magic[4];							// SGV!
	char			version_major;
	char			version_minor;
	char			index_hash_function;				// 0 = No Hash
														// 1 = Multiplication 1 : hash = ( hash + modifier * value )  % index_size;
														// 2 = Multiplication 2 : hash = ( hash + ( modifier * value ) + value ) % index_size;
	uint32_t		index_hash_function_modifier;
	uint32_t		index_size;
	uint32_t		index_entries;
};


typedef struct RT_Vault
{
	sgv_header header;
	//sgv_index_entry* entries;		// NOTE: here in case caching of the entire index is needed
} RT_Vault;

typedef struct RT_VaultNode
{
	char*			vault_name;
	RT_Vault		vault_data;
	RT_VaultNode*	next;
} RT_VaultNode;

RT_API RT_StringNode* RT_GetListOfVaults();
RT_API bool RT_GetFileFromVaults(const RT_String file_name, RT_String& buffer);
RT_API bool RT_GetFileFromVault(const RT_VaultNode* vault, const RT_String file_name, RT_String& buffer);
RT_API bool RT_LoadVaultIndex(const RT_String& vault_name, RT_Vault& vault);
RT_API uint32_t RT_VaultHash(const char* string_to_hash, const char hash_function, const uint32_t modifier, const uint32_t hash_map_size);