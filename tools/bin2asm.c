#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Usage: %s input_file output_file function_name\n", argv[0]);
		return 1;
	}

	FILE *inputFile, *outputFile;
	char *inputFileName = argv[1];
	char *outputFileName = argv[2];
	char *funcName = argv[3];
	int fileSize;

	inputFile = fopen(inputFileName, "rb");
	if (inputFile == NULL) {
		printf("Unable to open input file\n");
		return 1;
	}

	fseek(inputFile, 0, SEEK_END);
	fileSize = ftell(inputFile);
	fseek(inputFile, 0, SEEK_SET);

	outputFile = fopen(outputFileName, "w");
	if (outputFile == NULL) {
		printf("Unable to create output file\n");
		fclose(inputFile);
		return 1;
	}

	// Write the function declaration to the output file
	fprintf(outputFile, "void __attribute__((section(\".%s\"))) %s() {\n", funcName, funcName);

	// Read the binary file four bytes at a time and write to the C array file as inline assembly
	for (int i = 0; i < fileSize; i += 4) {
		unsigned int word;
		if (fread(&word, 4, 1, inputFile) == 1) {
			fprintf(outputFile, "\tasm volatile(\".word 0b");
			for (int j = 31; j >= 0; j--) { fprintf(outputFile, "%d", (word >> j) & 1); }
			fprintf(outputFile, "\");\n");
		}
	}

	fprintf(outputFile, "}\n");

	fclose(inputFile);
	fclose(outputFile);

	printf("Conversion complete %s => %s\n", inputFileName, outputFileName);

	return 0;
}