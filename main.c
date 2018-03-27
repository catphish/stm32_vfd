#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)

int sine_angle = 0;
float f1=0, f2=0, f3=0;
float i1, i2, i3;
float v1, v2, v3;
float ia, ib;
float fa, fb;
float torque;
float flux;
int throttle = 0;
int increment = 20616*10;

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
    TIM1->CCR1 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    TIM1->CCR2 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 18;
    TIM1->CCR3 = 0;
    break;

  case 1:
    // 110 -> 010
    TIM1->CCR1 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 18;
    TIM1->CCR2 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    TIM1->CCR3 = 0;
    break;

  case 2:
    // 010 -> 011
    TIM1->CCR1 = 0;
    TIM1->CCR2 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    TIM1->CCR3 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 18;
    break;

  case 3:
    // 011 -> 001
    TIM1->CCR1 = 0;
    TIM1->CCR2 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 18;
    TIM1->CCR3 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    break;

  case 4:
    // 001 -> 101
    TIM1->CCR1 = ((sine_angle + 1) * (voltage+1) * table[sine_angle] - 1) >> 18;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    break;

  case 5:
    // 101 -> 100
    TIM1->CCR1 = ((voltage+1) * table[sine_angle] - 1) >> 8;
    TIM1->CCR2 = 0;
    TIM1->CCR3 = ((1024 - sine_angle) * (voltage+1) * table[sine_angle] - 1) >> 18;
    break;
  }
}

void TIM1_UP_TIM16_IRQHandler(void) {
  // Start ADC conversions
  ADC1->SQR1 = (1<<6);
  ADC2->SQR1 = (2<<6);
  //ADC3->SQR1 = (3<<6);
  ADC1->CR |= ADC_CR_ADSTART;
  ADC2->CR |= ADC_CR_ADSTART;
  //ADC3->CR |= ADC_CR_ADSTART;
  TIM1->SR = ~TIM_SR_UIF;
}

void ADC1_2_IRQHandler(void) {
  while(ADC1->CR & ADC_CR_ADSTART);
  while(ADC2->CR & ADC_CR_ADSTART);
  i1 = ((float)(ADC1->DR) - 32812) * 0.000190738f;
  i2 = ((float)(ADC2->DR) - 32771) * 0.000190738f;
  i3 = 0 - i1 - i2;

  v1 = TIM1->CCR1 + TIM1->CCR1 - TIM1->CCR2 - TIM1->CCR3;
  v1 *= 0.000460452f;
  v2 = TIM1->CCR2 + TIM1->CCR2 - TIM1->CCR1 - TIM1->CCR3;
  v2 *= 0.000460452f;
  v3 = TIM1->CCR3 + TIM1->CCR3 - TIM1->CCR1 - TIM1->CCR2;
  v3 *= 0.000460452f;

  // Contol code here

  f1 *= 0.999f; f1 += v1; f1 -= i1*11;
  f2 *= 0.999f; f2 += v2; f2 -= i2*11;
  f3 *= 0.999f; f3 += v3; f3 -= i3*11;

  // Flux clarke transform
  fa = f1 - (f2 + f3) * 0.5f;
  fb = (f3 - f2) * 0.866025404f;

  // Current clarke transform
  ia = i1 - (i2 + i3) * 0.5f;
  ib = (i3 - i2) * 0.866025404f;

  torque = fa * ib - fb * ia;
  flux = sqrtf(fa * fa + fb * fb);
  //fangle = (atan2(fa, fb) / M_PI) * 50331647 + 50331648;

  sine_angle += increment;
  if(sine_angle > 100663295) sine_angle -= 100663296;
  update_svm(sine_angle, throttle);
}

int main() {
  ADC3->SQR1 = (4<<6);
  while (1) {
    ADC3->CR |= ADC_CR_ADSTART;
    while(ADC3->CR & ADC_CR_ADSTART);
    throttle = ADC3->DR;

    int n; for(n=0;n<20000;n++) nop();
    uart_write_int(v1 * 100);
    uart_write_string(",");
    uart_write_int(f1);
    uart_write_string(",");
    uart_write_int(i1*1000);
    uart_write_nl();
  }
  return(0);
}
