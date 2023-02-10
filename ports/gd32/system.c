/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include "gd32w51x.h"
#include "py/gc.h"
#include "py/mperrno.h"

#define RCU_GPIO_RX         RCU_GPIOA
#define RCU_GPIO_TX         RCU_GPIOB
#define RCU_UART            RCU_USART1
#define USART               USART1
#define UART_TX_GPIO        GPIOB
#define UART_TX_GPIO_PIN    GPIO_PIN_15
#define UART_RX_GPIO        GPIOA
#define UART_RX_GPIO_PIN    GPIO_PIN_8
#define UART_TX_AF  GPIO_AF_7
#define UART_RX_AF  GPIO_AF_3

void bare_main(void);

static void gd32_init(void);

static char *stack_top;
static char heap[8096];

// The CPU runs this function after a reset.
int main() {
    int stack_dummy;

    // Initialise the cpu and peripherals.
    gd32_init();

    stack_top = (char *)&stack_dummy;

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

    // Now that there is a basic system up and running, call the main application code.
    bare_main();

    // This function must not return.
    for (;;) {
    }
}

volatile static uint32_t delay;
void systick_config(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        /* capture error */
        while (1)
        {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

void delay_1ms(uint32_t count)
{
    delay = count;

    while (0U != delay)
    {
    }
}

void delay_decrement(void)
{
    if (0U != delay)
    {
        delay--;
    }
}

void SysTick_Handler(void)
{
    delay_decrement();
}


// Set up the GD32 MCU.
static void gd32_init(void) {
    /* configure systick for more precise delays */
    systick_config();

    rcu_periph_clock_enable(RCU_GPIO_RX);
    rcu_periph_clock_enable(RCU_GPIO_TX);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_UART);

    /* connect port to USARTx_Tx and USARTx_Rx  */
    gpio_af_set(UART_TX_GPIO, UART_TX_AF, UART_TX_GPIO_PIN);
    gpio_af_set(UART_RX_GPIO, UART_RX_AF, UART_RX_GPIO_PIN);

    gpio_mode_set(UART_TX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART_TX_GPIO_PIN);
    gpio_output_options_set(UART_TX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, UART_TX_GPIO_PIN);
    gpio_mode_set(UART_RX_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, UART_RX_GPIO_PIN);
    gpio_output_options_set(UART_RX_GPIO, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, UART_RX_GPIO_PIN);

    /* USART configure 115200 8N1 */
    usart_deinit(USART);
    usart_word_length_set(USART, USART_WL_8BIT);
    usart_stop_bit_set(USART, USART_STB_1BIT);
    usart_parity_config(USART, USART_PM_NONE);
    usart_baudrate_set(USART, 115200U);
    usart_receive_config(USART, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART, USART_TRANSMIT_ENABLE);
    usart_enable(USART);
}

// Write a character out to the UART.
static inline void uart_write_char(int c) {
    // Wait for TXE, then write the character.
    while (RESET == usart_flag_get(USART, USART_FLAG_TBE))
        ;
    usart_data_transmit(USART, (uint8_t)c);
}

// Send string of given length to stdout, converting \n to \r\n.
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    while (len--) {
        if (*str == '\n') {
            uart_write_char('\r');
        }
        uart_write_char(*str++);
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_hal_stdout_tx_strn_cooked(str, len);
}


// Send zero-terminated string
void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

int mp_hal_stdin_rx_chr(void) {
    // wait until we get a receive interrupt
    while(RESET == usart_flag_get(USART, USART_FLAG_RBNE))
        ;
    // receive data
    return (uint8_t) usart_data_receive(USART);
}

#include "shared/runtime/gchelper.h"

void gc_collect(void) {
    gc_collect_start();
    //gc_helper_collect_regs_and_stack();
    void *dummy;
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));

    gc_collect_end();
    //gc_dump_info();
}

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/mperrno.h"

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
