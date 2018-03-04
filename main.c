#include <stdint.h>

#define MMIO32(addr)        (*(volatile uint32_t *)(addr))
#define PERIPH_BASE_AHB2    (0x48000000U)
#define GPIO_PORT_A_BASE    (PERIPH_BASE_AHB2 + 0x0000)
#define GPIOA               GPIO_PORT_A_BASE
#define GPIO_MODER(port)    MMIO32((port) + 0x00)
#define GPIO_ODR(port)      MMIO32((port) + 0x14)
#define GPIOA_MODER         GPIO_MODER(GPIOA)
#define GPIOA_ODR           GPIO_ODR(GPIOA)

#define RCC_BASE            (0x40021000U)
#define RCC_AHB2ENR_OFFSET  0x4c
#define RCC_AHB2ENR         MMIO32(RCC_BASE + RCC_AHB2ENR_OFFSET)

# define nop()  __asm__ __volatile__ ("nop" ::)

int ontime  = 500000;
int offtime = 250000;

void setup() {
  RCC_AHB2ENR |= 1;
  nop();
  nop();
  GPIOA_MODER = 0xAB555555;
}

int main() {
  int n;
  while (1) {
    GPIOA_ODR = 0xFFFF;
    for(n=0;n<ontime;n++) nop();
    GPIOA_ODR = 0x0;
    for(n=0;n<offtime;n++) nop();
  }
  return(0);
}
