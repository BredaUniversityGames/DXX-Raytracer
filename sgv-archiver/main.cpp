

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stack>
#include <vector>
#include <limits>
#include <iostream>
#include <fstream>
#include <string>
#include "sgv-header.h"
#include "sgv-index-entry.h"
#include "sgv-utils.h"

int main(int argc, char* argv[])
{

	long long headerSize = 19;		// manually set the header size of sgv as sizeof(header) will return 20 due to padding;
	
	char* arg_input = nullptr;
	char* arg_output = nullptr;

	int index_size_multiplier = 2;
	int passes = 25;

	uint32_t prime_list[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997 , 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1289, 1291, 1297, 1301, 1303, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1487, 1493, 1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 1619, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 1733, 1747, 1753, 1759, 1777, 1783, 1787, 1801, 1811, 1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1973, 1979, 1987, 1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311, 2339, 2341, 2357, 2371, 2377, 2381, 2383, 2387, 2393, 2399, 2411, 2417, 2423, 2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2549, 2551, 2557, 2579, 2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2687, 2689, 2693, 2699, 2707, 2711, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903, 2909, 2917, 2927, 2939, 2953, 2957, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 3037, 3041, 3049, 3061, 3067, 3079, 3083, 3087, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3257, 3259, 3271, 3299, 3307, 3313, 3319, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461, 3463, 3467, 3471, 3473, 3487, 3491, 3499, 3511, 3517, 3527, 3539, 3541, 3547, 3557, 3571, 3581, 3583, 3593, 3607, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727, 3733, 3739, 3749, 3761, 3767, 3769, 3779, 3787, 3793, 3797, 3803, 3821, 3827, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 3893, 3907, 3911, 3917, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003, 4007, 4013, 4019, 4027, 4049, 4051, 4057, 4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129, 4133, 4139 };

	bool vault_has_been_split = false;		// tracks if an vault has been split so a number can be appended to vault name.

	for (auto arg_index = 1; arg_index < argc; arg_index++)  // start at 1 because arg 0 is the executable path
	{
		if (strcmp("-h", argv[arg_index]) == 0 || strcmp("--help", argv[arg_index]) == 0)
		{
			// help argument detected
			// print help screen and exit
			printHelp();

			return 0;
		}
		else if (strcmp("-i", argv[arg_index]) == 0)
		{
			// next argument should be the input path
			arg_index++;
			if(arg_index < argc)
				arg_input = argv[arg_index];
		}
		else if (strcmp("-o", argv[arg_index]) == 0)
		{
			// next argument should be the output file
			arg_index++;
			if (arg_index < argc)
				arg_output = argv[arg_index];
		}
		else if (strcmp("-p", argv[arg_index]) == 0)
		{
			// next argument should be the number of passes
			arg_index++;
			if (arg_index < argc)
			{
				if (atoi(argv[arg_index]) != 0)
				{
					passes = atoi(argv[arg_index]);
					passes = std::min(passes, 500);
				}
			}
		}
		else
			printf("Unknown Argument: %s\n", argv[arg_index]);
	}
	
	printf("\tInput: %s\n", arg_input);
	printf("\tOutput: %s\n", arg_output);

	if( arg_input == nullptr || arg_output == nullptr)
	{ 
		printf("One or more required arguments are missing. Exiting\n");
		return 1;
	}

	// Validate the input path
	if (!std::filesystem::exists(arg_input))
	{
		printf("Input path does not exist. Exiting\n");
		return 1;

	}
	
	if (!std::filesystem::is_directory(arg_input))
	{
		printf("Input path is not a directory. Exiting\n");
		return 1;
	}

	// Validate the output path
	std::filesystem::path output_path = std::filesystem::path(arg_output);
	std::filesystem::path output_path_extension = output_path.extension();
	std::filesystem::path output_path_wo_extension = output_path.replace_extension("");
	std::filesystem::path output_path_parent = output_path.parent_path();
	
	printf("Output parent directory: %s\n", output_path_parent.generic_string().c_str());

	if (!std::filesystem::exists(output_path_parent))
	{
		printf("Output file path does not exist. Exiting\n");
		return 1;
	}

	// now create the list of files that need to be added to this archive
	std::stack<std::filesystem::path> directories;
	std::vector<std::filesystem::path> files;

	directories.push(arg_input);

	while (!directories.empty())
	{
		std::filesystem::path cur_dir = directories.top();
		directories.pop();

		for (auto const& dir_entry : std::filesystem::directory_iterator(cur_dir) )
		{
			if (std::filesystem::is_regular_file( dir_entry ) )
			{
				//printf("File: %s \n", std::filesystem::relative( dir_entry.path(), std::filesystem::path(arg_input)).generic_string().c_str());
				files.push_back( dir_entry );
			}
			else if (std::filesystem::is_directory( dir_entry ))
			{
				//printf("Directory: %s \n", std::filesystem::relative( dir_entry.path(), std::filesystem::path(arg_input)).generic_string().c_str());
				directories.push( dir_entry );
			}
		}
	}

	printf("Total files to add to archives: %zd\n", files.size());

	// iterate over the list of files to add them to archives splitting them every 4GB.
	constexpr uint32_t max_file_size = std::numeric_limits<uint32_t>::max();

	int archive_count = 0;

	while (!files.empty())
	{
		printf("Remaining files to be archived: %zd\n", files.size());
		
		std::vector<std::filesystem::path> files_for_current_archive;
		uint32_t current_data_size = 0;
		bool current_archive_filled = false;

		while (!(files.empty() || current_archive_filled))
		{


			// get the last element from the files list
			std::filesystem::path& current_file = files.back();

			// get its file size.
			uintmax_t current_file_size = std::filesystem::file_size(current_file);

			// check if there is enough room to fit it into the current file archive
			uintmax_t current_archive_size = sizeof(sgv_header);
			current_archive_size += sizeof(sgv_index_entry) * files_for_current_archive.size() * index_size_multiplier;
			current_archive_size += current_data_size;

			if (current_archive_size + (sizeof(sgv_index_entry) * index_size_multiplier) + current_file_size < max_file_size)
			{
				// there is room to fit the new file into the archive
				current_data_size += current_file_size;
				files_for_current_archive.push_back(current_file);
				files.pop_back();
			}
			else
			{
				// there is not enough room to fit the new file into the archive
				printf("Current Archive Full, Starting new...");
				current_archive_filled = true;
				vault_has_been_split = true;
			}
		}

		// we have the files for the archive... process them into an archive.

		// determine the best hash modifier value to use for these index entries that maximize the spread of entries across the array to reduce collisions 
		uint32_t modifier_min = 1;
		uint32_t modifier_max = 1024;

		uint32_t modifier_selected = 0;
		double modifier_selected_spread_score = 1000000000.0;// std::numeric_limits<double>::max();

		// loop through the series hash modifier values and test build the index to test for how evenly spread the entries are.  The one with the most even spread is selected.  
		for (uint32_t modifier_current_index = 0; modifier_current_index < passes; modifier_current_index++)
		{
			printf("Optimizing Hash: Pass# %zd/%zd \r", modifier_current_index + 1, passes);
			fflush(stdout);

			uint32_t modifier_current = prime_list[modifier_current_index];

			// allocate space for the current test index
			auto index_size = files_for_current_archive.size() * index_size_multiplier;
			sgv_index_entry* index_entries = new sgv_index_entry[index_size]();

			for (auto file_index = 0; file_index < files_for_current_archive.size(); file_index++)
			{
				std::filesystem::path current_path = files_for_current_archive[file_index];
				std::string file_string = std::filesystem::relative(current_path, std::filesystem::path(arg_input)).generic_string();
					
				uint32_t file_string_hash = hashString(file_string, 1, index_size, modifier_current);

				// starting with the hashed index, find a place in the index for this file.
				bool found_place = false;

				while (!found_place)
				{
					if (index_entries[file_string_hash].key[0] == 0)
					{
						// we've found an empty index, put the data here.
						found_place = true;
						strcpy( index_entries[file_string_hash].key, file_string.c_str());
					}
					else
					{
						// index slot is already used

						// move index to next slot
						file_string_hash = ( file_string_hash + 1 ) % index_size;
					}
				}
			}

			// files are all placed in the index

			// see if this index is better than the current best index
			// calculateSpread returns a lower score for more event spread, so we want the lowest score to be chosen.
			double modifier_current_spread_score = calculateSpread(index_entries,index_size);

			if (modifier_current_spread_score < modifier_selected_spread_score)
			{
				modifier_selected = modifier_current;
				modifier_selected_spread_score = modifier_current_spread_score;
			}


			delete[] index_entries;
		}

		printf("Selected Hash Modifier: %d, Spread Score: %f\n", modifier_selected, modifier_selected_spread_score);

		// create the actual archive now
		printf("Writing Vault\n");
		uint32_t total_file_size = 0;
		std::filesystem::path final_output_path;


		if(vault_has_been_split)
			final_output_path = output_path_wo_extension.string() + std::to_string(archive_count) + output_path_extension.string(); 
		else
			final_output_path = output_path_wo_extension.string() + output_path_extension.string();

		std::ofstream sgv_fs(final_output_path, std::ios::out | std::ios::binary);

		if (!sgv_fs)
		{
			printf("Error opening file for writing: %s\n", final_output_path.generic_string().c_str());
			return 1;
		}

		// create the header
		sgv_header header;
		header.magic[0] = 'S';
		header.magic[1] = 'G';
		header.magic[2] = 'V';
		header.magic[3] = '!';
		header.version_major = 1;
		header.version_minor = 0;
		header.index_entries = files_for_current_archive.size();
		header.index_size = header.index_entries * index_size_multiplier;	// increase number of index entries for faster seeking.
		header.index_hash_function = 1;										// set hash_function to 1 (multiplication hash 1), NOTE may allow other values in future.
		header.index_hash_function_modifier = modifier_selected;

		sgv_fs.write((char*)&header.magic, sizeof(char) * 4);
		sgv_fs.write((char*)&header.version_major, sizeof(char));
		sgv_fs.write((char*)&header.version_minor, sizeof(char));
		sgv_fs.write((char*)&header.index_entries, sizeof(uint32_t));
		sgv_fs.write((char*)&header.index_size, sizeof(uint32_t));
		sgv_fs.write((char*)&header.index_hash_function, sizeof(char));
		sgv_fs.write((char*)&header.index_hash_function_modifier, sizeof(uint32_t));

		// build the archive index.
		auto index_size = files_for_current_archive.size() * index_size_multiplier;
		sgv_index_entry* index_entries = new sgv_index_entry[index_size]();

		// write the empty index to file for now (we'll go back and rewrite it once we have the real data).. just need to move the write position.
		sgv_fs.write((char*)index_entries, sizeof(sgv_index_entry) * index_size);

		// go through all the files to archive and both write them to archive file and record its info into the index.
		for (auto file_index = 0; file_index < files_for_current_archive.size(); file_index++)
		{
			std::filesystem::path current_path = files_for_current_archive[file_index];
			std::string file_string = std::filesystem::relative(current_path, std::filesystem::path(arg_input)).generic_string();
			auto current_file_size = std::filesystem::file_size(current_path);

			uint32_t file_string_hash = hashString(file_string, 1, index_size, modifier_selected);

			// starting with the hashed index, find a place in the index for this file.
			bool found_place = false;

			while (!found_place)
			{
				if (index_entries[file_string_hash].key[0] == 0)
				{
					// we've found an empty index, put the data here.
					found_place = true;
					strcpy(index_entries[file_string_hash].key, file_string.c_str());
					index_entries[file_string_hash].loc = sgv_fs.tellp();
					index_entries[file_string_hash].length = current_file_size;
					
				}
				else
				{
					// index slot is already used
					// move index to next slot
					file_string_hash = (file_string_hash + 1) % index_size;
				}
			}

			// read the file and copy its contents to the archive
			std::ifstream sourceFile(current_path, std::ios::in | std::ios::binary);
			if (!sourceFile.is_open()) {
				std::cerr << "Error opening source file!" << std::endl;
				return 1;
			}

			// Buffer to hold data while reading and writing
			const size_t bufferSize = 8192;
			char buffer[bufferSize];

			// Read from the source file and write to the destination file
			while (sourceFile.read(buffer, bufferSize)) {
				sgv_fs.write(buffer, sourceFile.gcount()); // Write the number of bytes read
			}

			// Write any remaining bytes
			sgv_fs.write(buffer, sourceFile.gcount());

			// Close the files
			sourceFile.close();
			
		}

		// seek back to the index location and write the final hashed index.
		sgv_fs.seekp(headerSize);
		sgv_fs.write((char*)index_entries, sizeof(sgv_index_entry)* index_size);

		
		sgv_fs.close();

		archive_count++;
		
	}
	
	return 0;
	
}