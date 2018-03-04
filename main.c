#include <stdint.h>

#define MMIO32(addr)    (*(volatile uint32_t *)(addr))
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

/* work out end of RAM address as initial stack pointer */
#define SRAM_BASE       0x20000000
#define SRAM_SIZE       0x18000 // 96KiB RAM
#define SRAM_END        (SRAM_BASE + SRAM_SIZE)

# define nop()  __asm__ __volatile__ ("nop" ::)

int main(void) {
  RCC_AHB2ENR |= 1;
  nop();
  nop();
  GPIOA_MODER = 0xAB555555;
  GPIOA_ODR = 0xFFFFFFFF;
  int n;
  while (1) {
    for(n=0;n<500000;n++) nop();
    GPIOA_ODR = 0x0;
    for(n=0;n<500000;n++) nop();
    GPIOA_ODR = 0xFFFF;
  }
}

/* vector table */
uint32_t vector_table[] __attribute__((section(".vector_table"))) =
{
  (uint32_t)SRAM_END,   // initial stack pointer
  (uint32_t)main        // main as Reset_Handler
};
