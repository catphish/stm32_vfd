#include <stdint.h>
#include <stm32l476xx.h>
#define nop()  __asm__ __volatile__ ("nop" ::)

int ontime  = 1000000;

void uart_write_string(char* str);
void uart_write_uint(uint32_t i);
void uart_write_nl();

int main() {
  uart_write_string("Main!\n");
  int n;
  while (1) {
    ADC1->CR |= ADC_CR_ADSTART;
    while(ADC1->CR & ADC_CR_ADSTART);
    uart_write_string("Hello!\n");
    uart_write_uint(ADC1->DR);
    uart_write_nl();
    for(n=0;n<ontime;n++) nop();
  }
  return(0);

}
