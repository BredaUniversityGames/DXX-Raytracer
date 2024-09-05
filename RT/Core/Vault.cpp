
#include <cerrno>

#include <Core/Vault.h>
#include <Core/String.h>

bool vault_data_cached = false;
RT_VaultNode* vault_data = nullptr;

RT_StringNode* RT_GetListOfVaults()
{
	char* vaults_file = RT_ArenaPrintF(&g_thread_arena, "vaults");

	RT_StringNode* vault_list = nullptr;

	FILE* f = fopen(vaults_file, "rb");

	if (!f)
	{
		return vault_list;	// return empty list
	}

	defer{ fclose(f); };

	// read entire file into memory
	fseek(f, 0, SEEK_END);
	size_t file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	RT_String file;
	file.bytes = (char*)RT_ArenaAllocNoZero(&g_thread_arena, file_size + 1, 16); // NOTE(daniel): This could just use the thread arena but there's nuances here if the arena passed in is the thread arena...
	file.count = file_size;

	// Null terminate for good measure
	file.bytes[file.count] = 0;

	size_t bytes_read = fread(file.bytes, 1, file_size, f);
	if (bytes_read != file_size)
	{
		return vault_list;
	}

	// split the string to lines (each line should be a vault entry)
	while (file.count > 0)
	{
		RT_String line;
		file = RT_StringSplitLine(file, &line);

		line = RT_StringTrim(line);

		// verify that the vault exists
		const char* vault_file = RT_CopyStringNullTerm(&g_thread_arena,line); //RT_ArenaPrintF(&g_thread_arena, "%s", line.bytes);

		FILE* f2 = fopen(vault_file, "rb");

		if (f2)
		{
			// we found a vault file
			printf("Found Vault: %s\n", line.bytes);

			// add it to the end of the vault list

			if (vault_list == nullptr)
			{
				vault_list = RT_ArenaAllocStruct(&g_thread_arena, RT_StringNode);
				vault_list->string = RT_CopyString(&g_thread_arena, line);
				vault_list->next = nullptr;
			}
			else
			{
				RT_StringNode* vault_list_node = vault_list;

				while (vault_list_node->next != nullptr)
					vault_list_node = vault_list_node->next;

				vault_list_node->next = RT_ArenaAllocStruct(&g_thread_arena, RT_StringNode);
				vault_list_node->next->string = RT_CopyString(&g_thread_arena, line);
				vault_list_node->next->next = nullptr;
			}

			fclose(f2);
		}

		
		
	}

	return vault_list;
}


bool RT_GetFileFromVaults(const RT_String file_name, char*& buffer, uint32_t& buffer_length)
{
	// Check to see if the vault data has not been cached
	if (!vault_data_cached)
	{
		// cache the vault data/indexes
		vault_data_cached = true;		// just need to keep track if we attempted to cache the vault data (so it doesn't repeatedly try to cache the vaults in the event it fails or there are none)

		// Get list of vaults
		RT_StringNode* vault_list = RT_GetListOfVaults();

		// loop through the list and build the vault data
		RT_StringNode* cur_node = vault_list;
		RT_VaultNode* cur_vault_node = nullptr;	// vault_data;

		// build the first vault list node
		if (cur_node != nullptr)
		{
			RT_Vault vault;
			if (RT_LoadVaultIndex(cur_node->string, vault))
			{
				// successfully loaded vault data, put it into list.
				vault_data = new RT_VaultNode();
				const char* temp_vault_name = RT_CopyStringNullTerm(&g_thread_arena, cur_node->string);
				vault_data->vault_name = (char*)malloc(strlen(temp_vault_name) + 1);
				strcpy(vault_data->vault_name, temp_vault_name);
				vault_data->vault_data = vault;
				vault_data->next = nullptr;
				cur_vault_node = vault_data;
			}

			cur_node = cur_node->next;
		}

		// build remaining vault list nodes
		while ( cur_node != nullptr )
		{
			//RT_GetFileFromVault(vault_list->string, file_name, buffer, buffer_length);
			RT_Vault vault;
			if (RT_LoadVaultIndex(cur_node->string, vault))
			{
				// successfully loaded vault data, put it into list.
				cur_vault_node->next = new RT_VaultNode();
				const char* temp_vault_name = RT_CopyStringNullTerm(&g_thread_arena, cur_node->string);
				cur_vault_node->next->vault_name = (char*)malloc(strlen(temp_vault_name) + 1);
				strcpy(cur_vault_node->next->vault_name, temp_vault_name);
				cur_vault_node->next->vault_data = vault;
				cur_vault_node->next->next = nullptr;
				cur_vault_node = cur_vault_node->next;
			}

			cur_node = cur_node->next;
		}
	}
	
	// Loop over list of vaults and search each one for the file
	if(vault_data_cached && vault_data != nullptr)
	{
		RT_VaultNode* cur_vault = vault_data;

		while (cur_vault != nullptr && buffer == nullptr)
		{
			RT_GetFileFromVault(cur_vault, file_name, buffer, buffer_length);
			cur_vault = cur_vault->next;
		}

		if (buffer != nullptr && buffer_length != 0)
			return true;
	}

	return false;
}

bool RT_GetFileFromVault(const RT_VaultNode* vault, const RT_String file_name, char*& buffer, uint32_t& buffer_length)
{
	

	if (vault != nullptr)
	{
		// open the vault
		char* vault_file = (char*)malloc(strlen(vault->vault_name) + 1); //  RT_CopyStringNullTerm(&g_thread_arena, vault->vault_name); //RT_ArenaPrintF(&g_thread_arena, "%s", line.bytes);
		strcpy(vault_file, vault->vault_name);

		FILE* f2 = fopen(vault_file, "rb");

		if (f2)
		{
			// query index to find if file present
			const char* file_name_cstr = RT_CopyStringNullTerm(&g_thread_arena, file_name);

			uint32_t file_name_hash = RT_VaultHash(
				file_name_cstr,
				vault->vault_data.header.index_hash_function,
				vault->vault_data.header.index_hash_function_modifier,
				vault->vault_data.header.index_size);

			bool found = false;
			bool empty = false;

			while (!empty && !found)
			{
				uint32_t headerSize = 19;		// manually set the header size as sizeof(sgv_header) will return 20 due to padding;
				fseek(f2, headerSize + (file_name_hash * sizeof(sgv_index_entry)), SEEK_SET);
				char index_entry[264];
				fread(&index_entry, sizeof(char), 264, f2);

				// check to see if the index entry is an empty string.  if so the file is not in the vault.
				if (index_entry[0] == 0)
					empty = true;

				else if (strcmp(file_name_cstr, index_entry) == 0)
				{
					found = true;  // we've found the file in the index

					// extract the file here
					
					uint32_t file_pos = 0;
					uint32_t file_len = 0;
					
					memcpy(&file_pos, &index_entry[256], 4);
					memcpy(&file_len, &index_entry[260], 4);

					buffer = (char*)malloc(file_len);
					_fseeki64(f2, file_pos, SEEK_SET);
					
					fread(buffer, sizeof(char), file_len, f2);
					
					buffer_length = file_len;

				}
				else
				{
					// we've hit an entry in the index, but it wasn't what we were looking for.  move the index to the next entry and try again
					file_name_hash = (file_name_hash+1) % vault->vault_data.header.index_size;
				}
			}

			// close vault
			fclose(f2);
		}

	}


	return false;

}

bool RT_LoadVaultIndex(const RT_String& vault_name, RT_Vault& vault)
{
	const char* vault_with_path = RT_CopyStringNullTerm(&g_thread_arena, vault_name);

	FILE* f = fopen(vault_with_path, "rb");

	if (!f)
	{
		return false;
	}

	defer{ fclose(f); };

	// start reading file.

	fread(&vault.header.magic, sizeof(char), 4, f);

	const char sgv_magic[] = { 'S','G','V','!' };
	if (memcmp(vault.header.magic, sgv_magic, 4) != 0)
	{
		// file is not an sgv
		return false;
	}

	fread(&vault.header.version_major, sizeof(char), 1, f);
	fread(&vault.header.version_minor, sizeof(char), 1, f);

	// check if version of file is 1.0.  The only version supported currently.
	if (vault.header.version_major != 1 || vault.header.version_minor != 0)
	{
		// unknown version of sgv
		return false;
	}

	// file is a valid sgv 1.0 file.  load the rest of the header.
	fread(&vault.header.index_entries, sizeof(uint32_t), 1, f);
	fread(&vault.header.index_size, sizeof(uint32_t), 1, f);
	fread(&vault.header.index_hash_function, sizeof(char), 1, f);
	fread(&vault.header.index_hash_function_modifier, sizeof(uint32_t), 1, f);
	
	
	// load the index 
	//vault.entries = (sgv_index_entry*)malloc(sizeof(sgv_index_entry) * vault.header.index_size);
	//fread(&vault.entries, sizeof(sgv_index_entry), vault.header.index_size, f);

	return true;
}

uint32_t RT_VaultHash(const char* string_to_hash, const char hash_function, const uint32_t modifier, const uint32_t hash_map_size)
{
	if (string_to_hash == nullptr)
		return 0;

	uint32_t hash = 0;

	if (hash_function == 1)
	{
		uint32_t char_index = 0;
		while(string_to_hash[char_index] != 0) 
		{

			hash = (hash * modifier + string_to_hash[char_index]) % hash_map_size;
			char_index++;
		}
	}

	return hash;
}