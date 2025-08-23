#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_LINE 16// Number of bytes to display per line in the output C array

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Usage: %s input_file output_file\n", argv[0]);// Print usage information if the number of command-line arguments is not 3
		return 1;
	}

	FILE *inputFile, *outputFile;
	char *inputFileName = argv[1]; // Name of the input binary file
	char *outputFileName = argv[2];// Name of the output C array file
	char *funcName = argv[3];	   // Name of the output C array
	int fileSize;

	// Open the input binary file
	inputFile = fopen(inputFileName, "rb");
	if (inputFile == NULL) {
		printf("Unable to open input file\n");// Print an error message if the input file cannot be opened
		return 1;
	}

	// Get the size of the file
	fseek(inputFile, 0, SEEK_END);
	fileSize = ftell(inputFile);
	fseek(inputFile, 0, SEEK_SET);

	// Create the output C array file
	outputFile = fopen(outputFileName, "w");
	if (outputFile == NULL) {
		printf("Unable to create output file\n");// Print an error message if the output file cannot be created
		fclose(inputFile);
		return 1;
	}

	// Write the C array declaration to the output file
	fprintf(outputFile, "const unsigned char __attribute__((section(\".%s\"))) %s[%d] = {\n\t", funcName, funcName, fileSize);

	// Read the binary file byte by byte and write to the C array file
	for (int i = 0; i < fileSize; i++) {
		unsigned char byte;
		if (fread(&byte, 1, 1, inputFile) != 1) {
			printf("Error occurred while reading the file\n");// Print an error message if there is an error reading the file
			fclose(inputFile);
			fclose(outputFile);
			return 1;
		}
		fprintf(outputFile, "0x%02X", byte);// Write the byte value in hexadecimal format
		if (i < fileSize - 1) {
			fprintf(outputFile, ", ");// Write a comma as a separator
		}
		if ((i + 1) % BYTES_PER_LINE == 0) {
			fprintf(outputFile, "\n\t");// Move to a new line every 16 bytes
		}
	}

	fprintf(outputFile, "\n};");// Add a semicolon at the end

	// Write the C array length declaration to the output file
	fprintf(outputFile, "\n\nunsigned long long %s_length = %d;\n\t", funcName, fileSize);

	// Close the files
	fclose(inputFile);
	fclose(outputFile);

	printf("Conversion complete %s => %s\n", inputFileName, outputFileName);// Print a completion message

	return 0;
}
