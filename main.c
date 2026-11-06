#include "printf.h"

int main(void) {

    mini_printf("Integer: %d\n", 0xF);

    mini_printf("String: %s\n", "Hello World");

    mini_printf("Character: %c\n", 'A');

    mini_printf("Float: %.2f\n", 123.456789); //dynamic precision

    mini_printf("Negative float: %f\n", -12.345678);

    mini_printf("Percentage: %%\n");

    return 0;
}