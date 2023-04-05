#include <stdio.h>

#include <bsp/board.h>
#include <tusb.h>

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/stdio.h>

#define SERIAL_MOUSE_RX_PIN 21
#define SERIAL_MOUSE_TX_PIN 20
#define SERIAL_MOUSE_UART uart1

uint16_t trans_report[3];
uint16_t rot_report[3];
uint8_t buttons_report[6];

uint8_t trans_pending = 0;
uint8_t rot_pending = 0;
uint8_t buttons_pending = 0;

uint8_t button_bits[] = { 12, 13, 14, 15, 22, 25, 23, 24, 0, 1, 2, 4, 5, 8, 26 };

int main() {
    board_init();
    tusb_init();
    stdio_init_all();

    printf("hello\n");

    gpio_set_function(SERIAL_MOUSE_RX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SERIAL_MOUSE_TX_PIN, GPIO_FUNC_UART);
    uart_init(SERIAL_MOUSE_UART, 9600);
    uart_set_translate_crlf(SERIAL_MOUSE_UART, false);
    uart_set_format(SERIAL_MOUSE_UART, 8, 1, UART_PARITY_NONE);

    sleep_ms(500);
    // make it send data without polling, set rate to 20ms
    uint8_t init_buf[] = { '\r', 'M', 'S', 'S', '\r', 'P', '@', 'T', '@', 'T', '\r' };
    uart_write_blocking(SERIAL_MOUSE_UART, init_buf, sizeof(init_buf));

    uint8_t buf[64];
    uint8_t idx = 0;
    bool escaped = false;

    while (true) {
        tud_task();
        if (tud_hid_ready()) {
            if (trans_pending) {
                tud_hid_report(1, trans_report, 6);
                trans_pending = 0;
            } else if (rot_pending) {
                tud_hid_report(2, rot_report, 6);
                rot_pending = 0;
            } else if (buttons_pending) {
                tud_hid_report(3, buttons_report, 6);
                buttons_pending = 0;
            }
        }

        if (uart_is_readable(SERIAL_MOUSE_UART)) {
            char c = uart_getc(SERIAL_MOUSE_UART);
            if (escaped) {
                switch (c) {
                    case 'M':
                        buf[idx++] = '\r';
                        break;
                    case '^':
                        buf[idx++] = '^';
                        break;
                }
                escaped = false;
            } else {
                if (c == '^') {
                    escaped = true;
                } else {
                    buf[idx++] = c;
                }
            }
            idx %= sizeof(buf);

            printf("%02x ", c);

            if (c == '\r') {
                printf("\n");

                if ((idx >= 2) && (buf[0] == '\n') && (buf[1] == '@')) {
                    for (int i = 1; i < idx - 1; i++) {
                        printf("%c", buf[i]);
                    }
                    printf("\n");
                }

                if ((buf[0] == 'D') && (idx == 16)) {
                    int16_t values[6];
                    for (int i = 0; i < 6; i++) {
                        values[i] = buf[3 + 2 * i] << 8 | buf[3 + 2 * i + 1];
                    }

                    trans_report[0] = values[0];
                    trans_report[1] = -values[2];
                    trans_report[2] = -values[1];
                    rot_report[0] = values[3];
                    rot_report[1] = -values[5];
                    rot_report[2] = -values[4];

                    trans_pending = 1;
                    rot_pending = 1;
                }

                if ((buf[0] == 'K') && (idx == 4)) {
                    uint16_t buttons = (buf[2] & 0x0F) | ((buf[1] & 0x0F) << 4);
                    memset(buttons_report, 0, sizeof(buttons_report));

                    for (int i = 0; i < 12; i++) {
                        if (buttons & (1 << i)) {
                            buttons_report[button_bits[i] / 8] |= 1 << (button_bits[i] % 8);
                        }
                    }

                    buttons_pending = 1;
                }
                idx = 0;
            }
        }
    }

    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
}

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return 0;
}
