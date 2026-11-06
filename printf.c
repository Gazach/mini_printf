#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

//================= FLOAT SHIT =======================

#define MAX_WHOLE_DIGITS 20
#define MAX_PRECISION 12
#define FLOAT_BUFFER_SIZE (1 + MAX_WHOLE_DIGITS + 1 + MAX_PRECISION + 1)

//======================================================

// Linux x86-64 assembly output

// part that pissed me off </3.

void print(const char *s) {
    __asm__ volatile (
        // Copy the string pointer (s) into RSI.
        // RSI will hold the address of our string.
        "mov %0, %%rsi\n\t"

        // Set RDX to 0.
        // We'll use RDX as our string length counter.
        "xor %%rdx, %%rdx\n\t"

        // Label 1: beginning of our loop.
        "1:\n\t"

        // Compare the current character (s + length) with 0.
        // A value of 0 means we've reached the null terminator ('\0').
        "cmpb $0, (%%rsi, %%rdx)\n\t"

        // If the character is 0, jump to label 2.
        // This means we've reached the end of the string.
        "je 2f\n\t"

        // Increment the string length counter.
        "inc %%rdx\n\t"

        // Jump backward to label 1 to check the next character.
        "jmp 1b\n\t"

        // Label 2: string length calculation is finished.
        "2:\n\t"

        // Linux x86-64 syscall number 1 = write().
        "mov $1, %%rax\n\t"

        // First syscall argument: file descriptor 1 (stdout).
        "mov $1, %%rdi\n\t"

        // RSI already contains the string's address.
        // RDX already contains the string's length.

        // Invoke the Linux kernel to perform write().
        "syscall\n\t"

        // OUTPUT OPERANDS:
        // None. We don't return a value to C.

        :

        // INPUT OPERANDS:
        // %0 represents the input variable s.
        // The compiler chooses a register to hold this pointer.
        : "r"(s)

        // CLOBBERS:
        // Tell the compiler which registers or machine state
        // are modified by this assembly block.
        :
          "rax",  // System call number
          "rdi",  // File descriptor
          "rsi",  // String pointer
          "rdx",  // String length
          "rcx",  // Modified by the syscall instruction
          "r11",  // Modified by the syscall instruction
          "cc",   // CPU condition flags are modified
          "memory" // Assembly accesses memory
    );
}

// String length

size_t strlen(const char *str) {
    size_t len = 0;

    while (str[len])
        len++;

    return len;
}

//================= INTEGER CONVERSION =================

static void int_to_string(int n, char *buffer) {
    int i = 0;

    bool isNeg = n < 0;

    unsigned int n1 =
        isNeg ? 0u - (unsigned int)n : (unsigned int)n;

    do {
        buffer[i++] = (char)(n1 % 10 + '0');
        n1 /= 10;
    } while (n1 != 0);

    if (isNeg)
        buffer[i++] = '-';

    buffer[i] = '\0';

    // Reverse

    for (int t = 0; t < i / 2; t++) {
        char temp = buffer[t];
        buffer[t] = buffer[i - t - 1];
        buffer[i - t - 1] = temp;
    }
}

//================= FLOAT CONVERSION ===================

static int float_to_string(double f, char *str, int precision)
{
    char digits[MAX_WHOLE_DIGITS];

    unsigned long long whole, scaled, scale = 1;

    double x, frac, rest;

    int n = 0;
    int i = 0;
    int l;

    if (precision < 0 || precision > MAX_PRECISION)
        return -1;

    if (isnan(f)) {
        strcpy(str, "nan");
        return 0;
    }

    if (isinf(f)) {
        strcpy(str, signbit(f) ? "-inf" : "inf");
        return 0;
    }

    x = fabs(f);

    if (x >= 18446744073709551616.0)
        return -1;

    if (signbit(f))
        str[i++] = '-';

    for (l = 0; l < precision; l++)
        scale *= 10;

    whole = (unsigned long long)x;

    frac = (x - (double)whole) * (double)scale;

    scaled = (unsigned long long)frac;

    rest = frac - (double)scaled;

    // Round to nearest, ties to even

    if (rest > 0.5 ||
        (rest == 0.5 &&
         (precision > 0 ? scaled : whole) % 2 == 1))
        scaled++;

    if (scaled >= scale) {
        scaled -= scale;
        whole++;
    }

    // Whole part

    do {
        digits[n++] = (char)('0' + whole % 10);
        whole /= 10;
    } while (whole > 0);

    while (n > 0)
        str[i++] = digits[--n];

    // Decimal part

    if (precision > 0) {
        str[i++] = '.';

        for (l = precision - 1; l >= 0; l--) {
            str[i + l] = (char)('0' + scaled % 10);
            scaled /= 10;
        }

        i += precision;
    }

    str[i] = '\0';

    return 0;
}

//================= MINI PRINTF ========================

void mini_printf(const char *f, ...) {
    va_list args;
    va_start(args, f);

    int count = strlen(f);

    for (int i = 0; i < count; i++) {

        if (f[i] == '%') {

            i++;

            // Handle literal %
            if (f[i] == '%') {
                print("%");
                continue;
            }

            // Parse precision
            int precision = -1;

            if (f[i] == '.') {
                precision = 0;
                i++;

                while (f[i] >= '0' && f[i] <= '9') {
                    precision = precision * 10 + (f[i] - '0');
                    i++;
                }
            }

            // Handle format specifier
            switch (f[i]) {

                // Integer
                case 'i':
                case 'd': {
                    int n = va_arg(args, int);

                    char buffer[32];

                    int_to_string(n, buffer);

                    print(buffer);
                    break;
                }

                // String
                case 's': {
                    const char *s = va_arg(args, const char *);

                    print(s ? s : "(null)");
                    break;
                }

                // Character
                case 'c': {
                    char c = (char)va_arg(args, int);

                    char buffer[] = { c, '\0' };

                    print(buffer);
                    break;
                }

                // Float
                case 'f': {
                    double n = va_arg(args, double);

                    char buffer[FLOAT_BUFFER_SIZE];

                    int p = precision == -1 ? 6 : precision;

                    if (float_to_string(n, buffer, p) == 0)
                        print(buffer);
                    else
                        print("[float error]");

                    break;
                }

                default:
                    print("%");

                    if (f[i] != '\0') {
                        char buffer[] = { f[i], '\0' };
                        print(buffer);
                    }

                    break;
            }

        } else {

            char buffer[] = { f[i], '\0' };

            print(buffer);
        }
    }

    va_end(args);
}