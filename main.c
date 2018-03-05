#include <stm32l476xx.h>
#define nop()  __asm__ __volatile__ ("nop" ::)

int ontime  = 20000000;
int offtime = 10000000;

int main() {
  /* Enbale GPIOA clock */
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  nop();nop();nop();
  /* Configure GPIOA pin 5 as output */
  GPIOA->MODER = 0xAB555555;

  int n;
  while (1) {
    GPIOA->ODR = 0xFFFF;
    for(n=0;n<ontime;n++) nop();
    GPIOA->ODR = 0x0;
    for(n=0;n<offtime;n++) nop();
  }
  return(0);

}
