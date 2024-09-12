#include <vector>
#include <numeric>
#include <cmath>

#include "sgv-utils.h"
#include "sgv-index-entry.h"

void printHelp()
{
    printf("\nsgv-archiver v1.0\n\n");

    printf("============\n");
    printf("    HELP\n");
    printf("============\n");

    printf("Takes an input directory and creates a seekable game vault (SGV) file.\n\n");

    printf("\t-h, --help\tPrint this help screen\n\n");
    printf("\t-i\t\tThe directory to use as the source of the content to be included in the vault.  All files and folders inside this directory will be included in the vault.\n");
    printf("\t\t\tex: C:/vault-content/\n");
    printf("\t\t\t( REQUIRED )\n\n");
    printf("\t-o\t\tThe file name of the vault to be created.\n");
    printf("\t\t\tex: C:/vault-output/vault.sgv\n");
    printf("\t\t\t( REQUIRED )\n\n");
    printf("\t-p\t\tNumber of passes to optimize hashing algorithm for faster seeking.\n");
    printf("\t\t\t( Default=25 )\n\n");
}

uint32_t hashString(const std::string& string, char hash_function, uint32_t size, uint32_t modifier)
{
    uint32_t string_hash = 0;

    if (hash_function == 0)
        return 0;

    else if (hash_function == 1)
    { 
        for (auto char_index = 0; char_index < string.size(); char_index++) {

            string_hash = (string_hash * modifier + string.at(char_index)) % size;
        }
    }

    return string_hash;
}

// Function to calculate the spread score
// It takes the distance between entries and finds the standard deviation of the distances between entries. 
// The more spread out the entries across the array the lower the deviation/score.  0 = perfectly spread entries.
double calculateSpread(const sgv_index_entry* index_entries, size_t index_size) {

    std::vector<int> filled_indices;

    // Collect indices of filled entries
    for (int i = 0; i < index_size; ++i) {

        if (index_entries[i].key[0] != 0) { // Assuming 0 indicates an empty entry

            filled_indices.push_back(i);
            //printf("#");
        }
        //else
            //printf(" ");
    }
    //printf("\n");

    if (filled_indices.size() < 2)
    {
        return 0.0; // Not enough entries to calculate spread
    }

    // Calculate distances between consecutive filled entries

    std::vector<int> distances;
    if (filled_indices[0] > 0)
    {
        distances.push_back(filled_indices[0]); // Distance from start of array to first filled entry
    }

    for (int i = 1; i < filled_indices.size(); ++i)
    {
        distances.push_back(filled_indices[i] - filled_indices[i - 1] - 1); // Subtract 1 to count only empty spaces
    }

    if (filled_indices.back() < index_size - 1)
    {
        distances.push_back(index_size - 1 - filled_indices.back()); // Distance from last filled entry to end of array
    }

    // Calculate the mean distance
    double meanDistance = std::accumulate(distances.begin(), distances.end(), 0.0) / distances.size();

    // Calculate the standard deviation of distances
    double variance = 0.0;
    for (int distance : distances) 
    {
        variance += std::pow(distance - meanDistance, 2);
    }

    variance /= distances.size();
    double stdDeviation = std::sqrt(variance);

    return stdDeviation;
}