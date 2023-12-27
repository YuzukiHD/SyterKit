#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#define SUNXI_UART0_BASE 0x05000000

// Function to transmit a single character via UART
void sunxi_uart_putc(char c) {
    // Wait until the TX FIFO is not full
    while ((read32(SUNXI_UART0_BASE + 0x14) & (0x1 << 6)) == 0)
        ;

    // Write the character to the UART data register
    write32(SUNXI_UART0_BASE + 0x00, c);
}

void sunxi_serial_putc(char c) {
    // Check if the character is a newline '\n'
    if (c == '\n') {
        // If it is, send a carriage return '\r' before newline
        sunxi_uart_putc('\r');
    }
    // Send the character via UART
    sunxi_uart_putc(c);
}

static int vpf_str_to_num(const char *fmt, int *num) {
    const char *p;
    int res, d, isd;

    // Initialize the result and iterate over the format string
    res = 0;
    for (p = fmt; *fmt != '\0'; p++) {
        isd = (*p >= '0' && *p <= '9');
        if (!isd)
            break;
        // Convert the digit character to its corresponding integer value
        d = *p - '0';
        // Multiply the result by 10 and add the digit value
        res *= 10;
        res += d;
    }
    // Store the final result in the provided pointer
    *num = res;
    // Return the number of characters processed
    return ((int) (p - fmt));
}

static void vpf_num_to_str(uint32_t a, int ish, int pl, int pc) {
    char buf[32];
    uint32_t base;
    int idx, i, t;

    // Initialize the buffer with padding characters
    for (i = 0; i < sizeof(buf); i++)
        buf[i] = pc;

    // Determine the base for number conversion (10 for decimal, 16 for hexadecimal)
    base = 10;
    if (ish)
        base = 16;

    idx = 0;
    // Convert the number to string representation
    do {
        t = a % base;
        if (t >= 10)
            buf[idx] = t - 10 + 'a';// Convert digit greater than 9 to corresponding character 'a' to 'f'
        else
            buf[idx] = t + '0';// Convert digit to corresponding character '0' to '9'
        a /= base;
        idx++;
    } while (a > 0);

    // Handle padding length
    if (pl > 0) {
        if (pl >= sizeof(buf))
            pl = sizeof(buf) - 1;
        if (idx < pl)
            idx = pl;
    }

    buf[idx] = '\0';// Null-terminate the string

    // Print the string in reverse order
    for (i = idx - 1; i >= 0; i--)
        sunxi_serial_putc(buf[i]);
}

// Output a null-terminated string to the serial port
static void vpf_str(const char *s) {
    while (*s != '\0') {        // Loop until the end of the string ('\0') is reached
        sunxi_serial_putc(*s++);// Output the current character and move to the next character in the string
    }
}

// Format and output a string using variable arguments
static int vpf(const char *fmt, va_list va) {
    const char *p, *q;       // Pointers for iterating through the format string
    int f, c, vai, pl, pc, i;// Variables for formatting and conversion
    unsigned char t;         // Temporary variable for storing characters

    pc = ' ';                       // Default padding character is space (' ')
    for (p = fmt; *p != '\0'; p++) {// Loop until the end of the format string is reached
        f = 0;                      // Initialize formatting flag
        pl = 0;                     // Initialize padding length
        c = *p;                     // Get the current character from the format string
        q = p;                      // Set a pointer to the current position in the format string
        if (*p == '%') {            // Check if the current character is a formatting specifier (%)
            q = p;                  // Update the pointer to the start of the formatting specifier
            p++;                    // Move to the next character after the %
            if (*p >= '0' && *p <= '9')
                p += vpf_str_to_num(p, &pl);// Parse the number after % as padding length
            f = *p;                         // Store the formatting specifier
        }
        if ((f == 'd') || (f == 'x')) {           // Check if the formatting specifier is for decimal or hexadecimal
            vai = va_arg(va, int);                // Retrieve the next argument from the variable arguments list (va_list) as an integer
            vpf_num_to_str(vai, f == 'x', pl, pc);// Convert the integer to a string representation and output it
        } else if (f == 's') {                    // Check if the formatting specifier is for a string
            char *str = va_arg(va, char *);       // Retrieve the next argument from the variable arguments list as a string pointer
            vpf_str(str);                         // Output the string
        } else {
            for (i = 0; i < (p - q); i++)
                sunxi_serial_putc(q[i]);         // Output the characters before the formatting specifier
            t = (unsigned char) (f != 0 ? f : c);// If there is no formatting specifier, output the current character
            sunxi_serial_putc(t);
        }
    }
    return 0;// Return 0 to indicate successful completion
}

void printf(const char *fmt, ...) {
    uart_printf("[    DRAMINIT][I] ");
    va_list va;
    va_start(va, fmt);
    vpf(fmt, va); // 标准输出格式化打印
    va_end(va);
}

// Output a formatted string to the standard output
void uart_printf(const char *fmt, ...) {
    va_list va;// Declare a variable argument list

    va_start(va, fmt);// Initialize the variable argument list with the provided format string
    vpf(fmt, va);     // Format and output the string using the format string and the variable argument list
    va_end(va);       // Clean up the variable argument list
}
