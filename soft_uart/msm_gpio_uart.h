#ifndef MSM_SOFT_UART_H
#define MSM_SOFT_UART_H

#include <linux/tty.h>

int msm_soft_uart_init(const int gpio_tx, const int gpio_rx);
int msm_soft_uart_finalize(void);
int msm_soft_uart_open(struct tty_struct* tty);
int msm_soft_uart_close(void);
int msm_soft_uart_set_baudrate(const int baudrate);
int msm_soft_uart_send_string(const unsigned char* string, int string_size);
int msm_soft_uart_get_tx_queue_room(void);
int msm_soft_uart_get_tx_queue_size(void);

#endif
