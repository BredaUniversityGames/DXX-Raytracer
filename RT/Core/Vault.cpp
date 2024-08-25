
#include <Core/Vault.h>
#include <Core/String.h>

RT_VaultNode* vault_data = nullptr;

RT_StringNode* RT_GetListOfVaults()
{
	char* vaults_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/vaults");

	RT_StringNode* vault_list = nullptr;

	FILE* f = fopen(vaults_file, "rb");

	if (!f)
	{
		//ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Failed to open file '%s'.", file_name));
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
		//ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Read a weird amount of bytes from file: Expected %zu, got %zu.", file_size, bytes_read));
		return vault_list;
	}

	// split the string to lines (each line should be a vault entry)
	while (file.count > 0)
	{
		RT_String line;
		file = RT_StringSplitLine(file, &line);

		line = RT_StringTrim(line);

		// verify that the vault exists
		char* vault_file = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s", line.bytes);

		FILE* f2 = fopen(vault_file, "rb");

		if (f2)
		{
			// we found a vault file
			//vault_list.push_back(line.bytes);
			printf("Found Vault: %s\n", line.bytes);

			RT_StringNode* vault_list_node = vault_list;

			while (vault_list_node != nullptr)
				vault_list_node = vault_list_node->next;

			vault_list_node = RT_ArenaAllocStruct(&g_thread_arena, RT_StringNode);
			
			fclose(f2);
		}

		
		
	}

	return vault_list;
}


bool RT_GetFileFromVaults(const RT_String file_name, char* buffer, uint32_t& buffer_length)
{
	// Check to see if the vault data has not been cached
	if (vault_data == nullptr)
	{
		// cache the vault data/indexes
		
		// Get list of vaults
		RT_StringNode* vault_list = RT_GetListOfVaults();

		// loop through the list and build the vault data
		RT_StringNode* cur_node = vault_list;
		RT_VaultNode* cur_vault_node = vault_data;

		while ( cur_node != nullptr )
		{
			//RT_GetFileFromVault(vault_list->string, file_name, buffer, buffer_length);
			RT_Vault vault;
			if (RT_LoadVaultIndex(cur_node->string.bytes, vault))
			{
				// successfully loaded vault data, put it into list.
				cur_vault_node = new RT_VaultNode();
				cur_vault_node->vault_name = RT_CopyString(&g_thread_arena, cur_node->string);
				cur_vault_node->vault_data = vault;
				cur_vault_node->next = nullptr;
				cur_vault_node = cur_vault_node->next;
			}

			cur_node = cur_node->next;
		}
	}
	
	// Loop over list of vaults and search each one for the file
	RT_VaultNode* cur_vault = vault_data;

	while (cur_vault != nullptr && buffer != nullptr)
	{
		RT_GetFileFromVault(cur_vault->vault_name, file_name, buffer, buffer_length);
		cur_vault = cur_vault->next;
	}

	if (buffer != nullptr && buffer_length != 0)
		return true;

	return false;
}

bool RT_GetFileFromVault(const RT_String vault_name, const RT_String file_name, char* buffer, uint32_t& buffer_length)
{
	// loop through the vault list to find our vault

	RT_VaultNode* cur_vault = vault_data;
	bool found_vault = false;

	while (!found_vault && cur_vault != nullptr)
	{
		if (RT_StringsAreEqual(vault_name, cur_vault->vault_name))
		{
			// found the vault

			// get the file

			// temp garbage to get to compile
			buffer = new char();
			buffer_length = 1;
			printf("%s", file_name.bytes);

			//return true;
		}
		else
		{
			// not found continue search
			cur_vault = cur_vault->next;
		}
	}


	return false;

}

bool RT_LoadVaultIndex(const char* vault_name, RT_Vault& vault)
{
	char* vault_with_path = RT_ArenaPrintF(&g_thread_arena, "assets/textures/%s", vault_name);

	FILE* f = fopen(vault_with_path, "rb");

	if (!f)
	{
		//ConfigError(cfg, RT_ArenaPrintF(cfg->arena, "Failed to open file '%s'.", file_name));
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
	fread(&vault.header.index_hash_function, sizeof(char), 1, f);
	fread(&vault.header.index_hash_function_modifier, sizeof(uint32_t), 1, f);
	fread(&vault.header.index_size, sizeof(uint32_t), 1, f);
	fread(&vault.header.index_entries, sizeof(uint32_t), 1, f);
	
	// load the index
	vault.entries = (sgv_index_entry*)malloc(sizeof(sgv_index_entry) * vault.header.index_size);
	fread(&vault.entries, sizeof(sgv_index_entry), vault.header.index_size, f);

	return true;
}