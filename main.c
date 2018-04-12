#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)

int sine_angle = 0;
int v1, v2, v3;
int i1, i2, i3;
int target_power;
int power = 0;
int current = 0;
int throttle = 0;
int voltage = 0;

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

uint16_t adc_data[4];
void DMA1_Channel1_IRQHandler(void) {
  // Debug output
  uart_write_int(adc_data[0]);
  uart_write_string(",");
  uart_write_int(adc_data[1]);
  uart_write_string(",");
  uart_write_int(adc_data[2]);
  uart_write_string(",");
  uart_write_int(adc_data[3]);
  uart_write_nl();
  ADC1->CR |= ADC_CR_ADSTART;
}

// void TIM1_UP_TIM16_IRQHandler(void) {
//   // Start ADC conversions.
//   // TODO: Try to cause these 2 conversions to run for a
//   // significant portion of the PWM cycle.
//   ADC1->SQR1 = (1<<6);
//   ADC2->SQR1 = (2<<6);
//   ADC1->CR |= ADC_CR_ADSTART;
//   ADC2->CR |= ADC_CR_ADSTART;
//   TIM1->SR = ~TIM_SR_UIF;
// }

// void ADC1_2_IRQHandler(void) {
//   // Fetch current values
//   // TODO: Instead of waiting for 2 ADCs, use combined mode and trust the interrupt.
//   while(ADC1->CR & ADC_CR_ADSTART);
//   while(ADC2->CR & ADC_CR_ADSTART);
//   i1 = ((float)ADC1->DR - 31715);
//   i2 = ((float)ADC2->DR - 31800) * 1.04f;
//   i3 = 0 - i1 - i2;

//   // Fetch the voltages.
//   v1 = TIM1->CCR1 + TIM1->CCR1 - TIM1->CCR2 - TIM1->CCR3;
//   v2 = TIM1->CCR2 + TIM1->CCR2 - TIM1->CCR1 - TIM1->CCR3;
//   v3 = TIM1->CCR3 + TIM1->CCR3 - TIM1->CCR1 - TIM1->CCR2;

//   // Calculate the instantaneous current and power.
//   // TODO optimize compiler, especially here.
//   current = sqrt(i1 * i1 + i2 * i2 + i3 * i3);
//   power = (v1 * i1 + v2 * i2 + v3 * i3) >> 14;

//   // Adjust voltage to fit calculates power to target power.
//   if (power > target_power) voltage -= 10;
//   if (power < target_power) voltage += 10;
//   if (voltage < 0) voltage = 0;
//   if (voltage > 4080) voltage = 4080;

//   // TODO: We can do better V/Hz control if we measure the
//   // bus voltage too.
//   // Work out V/Hz by multiplying the voltage by a Hz constant.
//   // Subtract some voltage based on current to account for resistive losses.

//   // A multiple is 25 here is theoretically correct for my motor at 23V DC supply.
//   int increment = voltage * 25 - 1000 * 25;
//   if(increment < 0) increment = 0;

//   // Increment the output phase.
//   sine_angle += increment;
//   if(sine_angle > 100663295) sine_angle -= 100663296;
//   if(sine_angle < 0) sine_angle += 100663296;
//   // Output SVM using PWM.
//   update_svm(sine_angle, voltage);
// }

int main() {
  ADC1->CR |= ADC_CR_ADSTART;
  while(1);
}
