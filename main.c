#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include "uart.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)

uint16_t adc_data[4];

int angle = 0;
int frequency = 0;
int voltage = 0;

int i1, i2;

// This runs 0.000048s (20,833Hz) after it's triggered
// This means data is collected during the first half of each
// pwm cycle ready for calculations in the second half.
void DMA1_Channel1_IRQHandler(void) {
  // Fetch the phase 1 and 2 currents
  i1 = adc_data[0] - 32840;
  i2 = adc_data[1] - 32840;

  // 13744 = 1Hz
  frequency = 13744 * 5;
  voltage = 4000;

  // Increment angle and wrap
  angle += frequency;
  if(angle > 134217728) angle -= 134217728;
  if(angle < 0) angle += 100663296;

  // Update PWM outputs from SVM style lookup table
  int phase = angle >> 14;
  TIM1->CCR1 = (voltage * table1[phase]) >> 13;
  TIM1->CCR2 = (voltage * table2[phase]) >> 13;
  TIM1->CCR3 = (voltage * table3[phase]) >> 13;

  // Clear interrupt
  DMA1->IFCR = 0xFFFFFFFF;
}

void TIM1_UP_TIM16_IRQHandler(void) {
  // Start ADC conversions each time PWM restarts
  // This runs at 80000000/8192 = 9765.625Hz
  ADC1->CR |= ADC_CR_ADSTART;
  TIM1->SR = ~TIM_SR_UIF;
}

int main() {
  while(1) {
    // A handy loop for debugging
    // All the real work is interrupt driven
    uart_write_int(TIM2->CNT);
    uart_write_nl();
    int n; for(n=0;n<10000;n++) nop();
  }
}
