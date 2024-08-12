

#include <cstdio>
#include <cstring>
#include <filesystem>

int main(int argc, char* argv[])
{
	printf("GAV Archiver\n");
	
	char* arg_input = nullptr;
	char* arg_output = nullptr;

	printf("Parsing Arguments\n");

	for (auto arg_index = 1; arg_index < argc; arg_index++)  // start a 1 because arg 0 is the executable path
	{
		if (strcmp("-i", argv[arg_index]) == 0)
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

	// Validate the input
	if (!std::filesystem::exists(arg_input))
	{
		printf("Input path does not exist. Exiting\n");
		return 1;
	}

	return 0;
	
}