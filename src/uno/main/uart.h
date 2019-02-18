#pragma once

#ifdef __cplusplus
extern "C" {
#endif
void uart_putchar(char c);
void uart_putstr(const char *c);
void uart_init(void);
#ifdef __cplusplus
}
#endif