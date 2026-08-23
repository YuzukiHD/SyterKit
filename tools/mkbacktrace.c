/* SPDX-License-Identifier: GPL-2.0+ */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Maximum symbol name accepted from nm output. */
#define SYMBOL_NAME_SIZE 1024U

/**
 * @brief Function symbol parsed from sorted nm output.
 */
struct symbol {
	uint64_t address; /**< Function start address. */
	uint64_t size; /**< Function size in bytes. */
	char *name; /**< Allocated function name. */
};

/**
 * @brief Report a failed file operation.
 * @param[in] path Path associated with the operation.
 * @return Process failure status.
 */
static int report_file_error(const char *path) {
	fprintf(stderr, "mkbacktrace: %s: %s\n", path, strerror(errno));
	return EXIT_FAILURE;
}

/**
 * @brief Release a parsed symbol array.
 * @param[in] symbols Symbol array to release.
 * @param[in] count Number of entries in the array.
 */
static void free_symbols(struct symbol *symbols, size_t count) {
	size_t index;

	for (index = 0; index < count; index++)
		free(symbols[index].name);
	free(symbols);
}

/**
 * @brief Append one symbol to a dynamically sized array.
 * @param[in,out] symbols Address of the symbol-array pointer.
 * @param[in,out] count Number of populated entries.
 * @param[in,out] capacity Number of allocated entries.
 * @param[in] address Function start address.
 * @param[in] size Function size in bytes.
 * @param[in] name Function name.
 * @return Zero on success, otherwise -1.
 */
static int append_symbol(struct symbol **symbols, size_t *count,
			 size_t *capacity, uint64_t address, uint64_t size,
			 const char *name) {
	struct symbol *resized;
	char *copy;

	if (*count == *capacity) {
		size_t new_capacity = *capacity ? *capacity * 2U : 128U;

		resized = realloc(*symbols, new_capacity * sizeof(**symbols));
		if (!resized)
			return -1;
		*symbols = resized;
		*capacity = new_capacity;
	}

	copy = strdup(name);
	if (!copy)
		return -1;
	(*symbols)[*count].address = address;
	(*symbols)[*count].size = size;
	(*symbols)[*count].name = copy;
	(*count)++;
	return 0;
}

/**
 * @brief Read text symbols from nm output.
 * @param[in] input Open nm-output stream.
 * @param[out] symbols Parsed symbol array.
 * @param[out] count Number of parsed symbols.
 * @param[in] address_bits Target pointer width in bits.
 * @return Zero on success, otherwise -1.
 */
static int read_symbols(FILE *input, struct symbol **symbols, size_t *count,
			unsigned int address_bits) {
	char line[2U * SYMBOL_NAME_SIZE];
	size_t capacity = 0;

	*symbols = NULL;
	*count = 0;
	while (fgets(line, sizeof(line), input)) {
		unsigned long long address;
		unsigned long long size = 0;
		char type;
		char name[SYMBOL_NAME_SIZE];
		int fields;

		fields = sscanf(line, "%llx %llx %c %1023s", &address, &size,
				&type, name);
		if (fields != 4 &&
		    (fields = sscanf(line, "%llx %c %1023s", &address, &type,
				      name)) != 3)
			continue;
		if (!strchr("TtWw", type))
			continue;
		if (address_bits == 32U &&
		    (address > UINT32_MAX || size > UINT32_MAX)) {
			fprintf(stderr,
				"mkbacktrace: symbol does not fit a 32-bit table\n");
			return -1;
		}
		if (append_symbol(symbols, count, &capacity, (uint64_t) address,
				  (uint64_t) size, name))
			return -1;
	}

	return ferror(input) ? -1 : 0;
}

/**
 * @brief Write one string as a GNU assembler quoted literal.
 * @param[in] output Destination stream.
 * @param[in] value String to quote.
 */
static void write_quoted(FILE *output, const char *value) {
	fputc('"', output);
	while (*value) {
		unsigned char byte = (unsigned char) *value++;

		if (byte == '\\' || byte == '"')
			fputc('\\', output);
		if (byte >= 0x20U && byte <= 0x7eU)
			fputc(byte, output);
		else
			fprintf(output, "\\%03o", byte);
	}
	fputc('"', output);
}

/**
 * @brief Emit the compact target-side symbol table.
 * @param[in] output Destination assembly stream.
 * @param[in] symbols Parsed symbols in ascending address order.
 * @param[in] count Number of symbols to emit.
 * @param[in] address_bits Target pointer width in bits.
 * @return Zero on success, otherwise -1.
 */
static int write_assembly(FILE *output, const struct symbol *symbols,
			  size_t count, unsigned int address_bits) {
	const char *directive = address_bits == 64U ? ".quad" : ".long";
	unsigned int alignment = address_bits / 8U;
	unsigned int width = address_bits / 4U;
	size_t index;

	fputs("/* SPDX-License-Identifier: GPL-2.0+ */\n"
	      ".section .rodata.backtrace_symbols,\"a\",%progbits\n", output);
	fprintf(output, ".balign %u\n", alignment);
	fputs(".global __backtrace_symbols\n"
	      ".type __backtrace_symbols, %object\n"
	      "__backtrace_symbols:\n", output);
	for (index = 0; index < count; index++) {
		fprintf(output, "\t%s 0x%0*llx\n\t%s 0x%0*llx\n"
			"\t%s .Lbacktrace_name_%zu - "
			"__backtrace_symbol_names\n",
			directive, width,
			(unsigned long long) symbols[index].address,
			directive, width,
			(unsigned long long) symbols[index].size,
			directive, index);
	}
	fputs(".size __backtrace_symbols, . - __backtrace_symbols\n"
	      ".global __backtrace_symbol_count\n"
	      ".type __backtrace_symbol_count, %object\n"
	      "__backtrace_symbol_count:\n", output);
	fprintf(output, "\t%s %zu\n"
		".size __backtrace_symbol_count, %u\n"
		".section .rodata.backtrace_names,\"a\",%%progbits\n"
		".global __backtrace_symbol_names\n"
		".type __backtrace_symbol_names, %%object\n"
		"__backtrace_symbol_names:\n", directive, count, alignment);
	for (index = 0; index < count; index++) {
		fprintf(output, ".Lbacktrace_name_%zu:\n\t.asciz ", index);
		write_quoted(output, symbols[index].name);
		fputc('\n', output);
	}
	fputs(".size __backtrace_symbol_names, . - "
	      "__backtrace_symbol_names\n"
	      ".section .note.GNU-stack,\"\",%progbits\n", output);

	return ferror(output) ? -1 : 0;
}

/**
 * @brief Convert sorted nm output into target assembly data.
 * @param[in] argc Number of command-line arguments.
 * @param[in] argv Target width followed by input and output paths.
 * @return Process success or failure status.
 */
int main(int argc, char **argv) {
	struct symbol *symbols;
	size_t count;
	unsigned int address_bits;
	const char *input_path;
	const char *output_path;
	FILE *input;
	FILE *output;
	int status = EXIT_FAILURE;

	if (argc != 4 || (strcmp(argv[1], "32") && strcmp(argv[1], "64"))) {
		fprintf(stderr,
			"Usage: %s <32|64> <nm-input> <assembly-output>\n",
			argv[0]);
		return EXIT_FAILURE;
	}
	address_bits = (unsigned int) strtoul(argv[1], NULL, 10);
	input_path = argv[2];
	output_path = argv[3];

	input = fopen(input_path, "r");
	if (!input)
		return report_file_error(input_path);
	if (read_symbols(input, &symbols, &count, address_bits)) {
		fprintf(stderr, "mkbacktrace: cannot parse %s\n", input_path);
		fclose(input);
		free_symbols(symbols, count);
		return EXIT_FAILURE;
	}
	if (fclose(input)) {
		free_symbols(symbols, count);
		return report_file_error(input_path);
	}

	output = fopen(output_path, "w");
	if (!output) {
		free_symbols(symbols, count);
		return report_file_error(output_path);
	}
	if (write_assembly(output, symbols, count, address_bits)) {
		fclose(output);
		report_file_error(output_path);
	} else if (fclose(output)) {
		report_file_error(output_path);
	} else {
		status = EXIT_SUCCESS;
	}
	free_symbols(symbols, count);
	return status;
}
