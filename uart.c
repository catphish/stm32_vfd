#include <stdint.h>
#include <stm32l476xx.h>

void uart_write_char(uint8_t c) {
  while(!(USART2->ISR & USART_ISR_TXE));
  USART2->TDR = c;
}

void uart_write_string(char* str)
{
  while (*str)
  {
    uart_write_char(*str++);
  }
}

void uart_write_int(int32_t i) {
  if (i < 0) {
    uart_write_char('-');
    i = 0-i;
  }
  static char buffer[20] = {0};
  if(i == 0){
    uart_write_char('0');
  } else {
    int j = 0;
    while(i) {
      buffer[j++] = (i % 10) + 48;
      i /= 10;
    }
    while(j) {
      uart_write_char(buffer[--j]);
    }
  }
}

void uart_write_nl() {
  uart_write_char(10);
}
