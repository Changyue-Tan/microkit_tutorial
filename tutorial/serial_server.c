#include <stdint.h>
#include <microkit.h>
#include "printf.h"

// This variable will have the address of the UART device
uintptr_t uart_base_vaddr;

char *input_buffer; // handle user input

char *output_buffer; // display game state

#define CLIENT_CHANNEL_ID 1

#define UART_CHANNEL_ID 0 // channel id = irq id = 0

#define RHR_MASK 0b111111111
#define UARTDR 0x000
#define UARTFR 0x018
#define UARTIMSC 0x038
#define UARTICR 0x044
#define PL011_UARTFR_TXFF (1 << 5)
#define PL011_UARTFR_RXFE (1 << 4)

#define REG_PTR(base, offset) ((volatile uint32_t *)((base) + (offset)))

void uart_init() {
    *REG_PTR(uart_base_vaddr, UARTIMSC) = 0x50;
}

int uart_get_char() {
    int ch = 0;

    if ((*REG_PTR(uart_base_vaddr, UARTFR) & PL011_UARTFR_RXFE) == 0) {
        ch = *REG_PTR(uart_base_vaddr, UARTDR) & RHR_MASK;
    }

    return ch;
}

void uart_put_char(int ch) {
    while ((*REG_PTR(uart_base_vaddr, UARTFR) & PL011_UARTFR_TXFF) != 0);

    *REG_PTR(uart_base_vaddr, UARTDR) = ch;
    if (ch == '\r') {
        uart_put_char('\n');
    }
}

void uart_handle_irq() {
    *REG_PTR(uart_base_vaddr, UARTICR) = 0x7f0;
}

void uart_put_str(char *str) {
    while (*str) {
        uart_put_char(*str);
        str++;
    }
}

void init(void) {
    // First we initialise the UART device, which will write to the
    // device's hardware registers. Which means we need access to
    // the UART device.
    uart_init();
    // After initialising the UART, print a message to the terminal
    // saying that the serial server has started.
    uart_put_str("SERIAL SERVER: starting\n");
    // microkit_notify(CLIENT_CHANNEL_ID);
}

void notified(microkit_channel channel) {
    switch (channel) {
        case UART_CHANNEL_ID: // something was inputed via keyboard
            char input = uart_get_char();
            // uart_put_char(input);

            input_buffer[0] = input; // write input char to input buffer
            microkit_notify(CLIENT_CHANNEL_ID); // tell the clinet to display the char

            uart_handle_irq();
            microkit_irq_ack(channel);
            break;
        case CLIENT_CHANNEL_ID: // clients wants to display something
            uart_put_str(output_buffer);
            // refresh the buffer
            for (int i = 0; output_buffer[i] != '\0'; i++) {
                output_buffer[i] = '\0';
            }
            break;
    }
}
