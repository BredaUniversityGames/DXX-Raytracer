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
	sgv_index_entry* entries;
} RT_Vault;

typedef struct RT_VaultNode
{
	RT_String		vault_name;
	RT_Vault		vault_data;
	RT_VaultNode*	next;
} RT_VaultNode;

RT_API RT_StringNode* RT_GetListOfVaults();
RT_API bool RT_GetFileFromVaults(const RT_String file_name, char* buffer, uint32_t& buffer_length);
RT_API bool RT_GetFileFromVault(const RT_String vault_name, const RT_String file_name, char* buffer, uint32_t& buffer_length);
RT_API bool RT_LoadVaultIndex(const char* vault_name, RT_Vault& vault);