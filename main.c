#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"

#define nop()  __asm__ __volatile__ ("nop" ::)

unsigned long long int delay  = 100000;

void uart_write_string(char* str);
void uart_write_uint(uint32_t i);
void uart_write_nl();

// Phase should rotate from 0 - (2^24)*6
// Voltage should be 0 - 2^12
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
    TIM1->CCR1 = (voltage * table[sine_angle] - 1) >> 8;
    TIM1->CCR2 = ((sine_angle + 1) * voltage * table[sine_angle] - 1) >> 18;
    TIM1->CCR3 = 0;
    break;

  case 1:
    // 110 -> 010
    TIM1->CCR1 = ((1024 - sine_angle) * voltage * table[sine_angle] - 1) >> 18;
    TIM1->CCR2 = (voltage * table[sine_angle] - 1) >> 8;
    TIM1->CCR3 = 0;
    break;

  case 2:
    // 010 -> 011
    TIM1->CCR1 = 0;
    TIM1->CCR2 = (voltage * table[sine_angle] - 1) >> 8;
    TIM1->CCR3 = ((sine_angle + 1) * voltage * table[sine_angle] - 1) >> 18;
    break;

  case 3:
    // 011 -> 001
    TIM1->CCR1 = 0;
    TIM1->CCR2 = ((1024 - sine_angle) * voltage * table[sine_angle] - 1) >> 18;
    TIM1->CCR3 = (voltage * table[sine_angle] - 1) >> 8;
    break;

  case 4:
    // 001 -> 101
    TIM1->CCR1 = ((sine_angle + 1) * voltage * table[sine_angle] - 1) >> 18;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = (voltage * table[sine_angle] - 1) >> 8;
    break;

  case 5:
    // 101 -> 100
    TIM1->CCR1 = (voltage * table[sine_angle] - 1) >> 8;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = ((1024 - sine_angle) * voltage * table[sine_angle] - 1) >> 18;
    break;
  }
}

int main() {
  unsigned int sine_angle = 0;
  int vv1, vv2, vv3, ii1, ii2, ii3;
  unsigned int power=0;
  unsigned int n = 0;

  while (1) {
    update_svm(sine_angle, 4080);

    vv1 = TIM1->CCR1;
    vv1 -= 8192;
    vv2 = TIM1->CCR2;
    vv2 -= 8192;
    vv3 = TIM1->CCR3;
    vv3 -= 8192;

    ADC1->SQR1 = (1<<6);
    ADC2->SQR1 = (2<<6);
    ADC3->SQR1 = (3<<6);
    ADC1->CR |= ADC_CR_ADSTART;
    ADC2->CR |= ADC_CR_ADSTART;
    ADC3->CR |= ADC_CR_ADSTART;
    while(ADC1->CR & ADC_CR_ADSTART);
    while(ADC2->CR & ADC_CR_ADSTART);
    while(ADC3->CR & ADC_CR_ADSTART);
    ii1 = ADC1->DR - 1982;
    ii2 = ADC2->DR - 1988;
    ii3 = ADC3->DR - 1990;

    ADC1->SQR1 = (4<<6);
    ADC1->CR |= ADC_CR_ADSTART;
    while(ADC1->CR & ADC_CR_ADSTART);
    sine_angle += ADC1->DR*2;
    if(sine_angle > (16777216)*6) sine_angle -= (16777216)*6;

    power = (ii1 * vv1 + ii2 * vv2 + ii3 * vv3);
    n++;
    if(n%8192==0){
      uart_write_uint(power);
      // uart_write_string(",");
      // uart_write_uint(ii2);
      // uart_write_string(",");
      // uart_write_uint(ii3);
      uart_write_nl();
      power = 0;
    }
  }
  return(0);
}
