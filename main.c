#include <stdint.h>
#include <stm32l476xx.h>
#define nop()  __asm__ __volatile__ ("nop" ::)

int ontime  = 20000000;
int offtime = 10000000;

void uart_write_string(char* str);
void uart_write_uint(uint32_t i);
void uart_write_nl();

int main() {
  int n;
  while (1) {
    uart_write_string("Hello!\n");
    uart_write_uint(TIM1->CNT);
    uart_write_nl();
    for(n=0;n<ontime;n++) nop();
  }
  return(0);

}
