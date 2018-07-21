#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)

uint16_t adc_data[4];

int angle = 0;
int frequency = 0;

float i1, i2;

void uart_write_string(char* str);
void uart_write_int(int32_t i);
void uart_write_nl();

// Phase should rotate from 0 - (2^24)*6-1
// Voltage should be 0 - 2^12-1
void update_svm(uint32_t phase, uint32_t voltage)
{
  uint32_t sine_angle;  // The angle within the current segment (10 bits)
  uint32_t sine_segment; // The current segment of rotation (0-5)

  // Discard the least significant 14 bits and most significant
  // 8 bits from 32-bit sine position. We can only make use of 10 bits
  sine_angle = ((phase & 0xFFC000) >> 14);

  // The most significant 8 bits contain the SVM segment (0-5)
  sine_segment = phase >> 24;

  // Set up space vector modulation according to segment, angle, and voltage
  // Output is 14 bits
  switch(sine_segment) {
  case 0:
    // 100 -> 110
    TIM1->CCR1 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    TIM1->CCR2 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 19;
    TIM1->CCR3 = 0;
    break;

  case 1:
    // 110 -> 010
    TIM1->CCR1 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 19;
    TIM1->CCR2 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    TIM1->CCR3 = 0;
    break;

  case 2:
    // 010 -> 011
    TIM1->CCR1 = 0;
    TIM1->CCR2 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    TIM1->CCR3 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 19;
    break;

  case 3:
    // 011 -> 001
    TIM1->CCR1 = 0;
    TIM1->CCR2 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 19;
    TIM1->CCR3 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    break;

  case 4:
    // 001 -> 101
    TIM1->CCR1 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 19;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    break;

  case 5:
    // 101 -> 100
    TIM1->CCR1 = ((voltage+1) * table[sine_angle] - 1) >> 9;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 19;
    break;
  }
}

// This runs at 80000000/8192 = 9765.625Hz
void DMA1_Channel1_IRQHandler(void) {
  i1 = adc_data[0] - 32840;
  i2 = adc_data[1] - 32840;

  // 10308 = 1Hz
  frequency = 10308 * 5;
  int voltage = 4000;

  // Increment angle
  sine_angle += frequency;
  if(sine_angle > 100663295) sine_angle -= 100663296;
  if(sine_angle < 0) sine_angle += 100663296;

  // Output SVM using PWM.
  update_svm(sine_angle, voltage);
  DMA1->IFCR = 0xFFFFFFFF;
}

void TIM1_UP_TIM16_IRQHandler(void) {
  // Start ADC conversions after each PWM refresh
  ADC1->CR |= ADC_CR_ADSTART;
  TIM1->SR = ~TIM_SR_UIF;
}

int main() {
  while(1) {
    // Debug output
     uart_write_int(i1*1000);
     uart_write_string(",");
     uart_write_int(i2*1000);
     uart_write_nl();
     int n; for(n=0;n<10000;n++) nop();

  }
}
