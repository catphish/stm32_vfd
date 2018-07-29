#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include "uart.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)
#define PI 3.14159265f

// Globals for data that is either persistent or used for debuging
int i1, i2, i3;    // Phase currents
int total_current; // Total instantaneous current
int angle;         // The angle of the stator voltage
int voltage;       // The magnitude of the stator voltage
int encoder_position_previous; // The previous encoder value

// Analog to digital converter DMA data
uint16_t adc_data[4]; // [i1, i2, i3, throttle]

// Motor parameters
#define MOTOR_POLES 4
#define ENCODER_RESOLUTION 2400
#define MAGNETIZING_CURRENT 3000
#define MAX_TORQUE_CURRENT 7000
#define GAIN 2

// This runs at the PWM frequency: 9765.625Hz
// It runs 0.000048s after it's triggered (approx half the PWM period)
// This means data is collected during the first half of each
// pwm cycle ready for calculations in the second half.
void DMA1_Channel1_IRQHandler(void) {
  // Fetch the phase currents
  i1 = adc_data[0] - 32840; // Offset to 0A
  i2 = adc_data[1] - 32840; // Offset to 0A
  i3 = adc_data[2] - 32840; // Offset to 0A
  // Calculate the total current (FPU operation)
  total_current = sqrtf((float)i1*(float)i1 + (float)i2*(float)i2 + (float)i3*(float)i3);

  // Calculate encoder ticks change since the last iteration
  uint32_t encoder_position_current = TIM2->CNT; // Atomic as possible
  int encoder_position_change = encoder_position_current - encoder_position_previous; // Nasty casting to signed here
  encoder_position_previous = encoder_position_current;

  // 1 Revolution = 67108864 (2**26) increments
  // 1 RPM = 6872 increments per loop (2**26 / 9765.625Hz)
  // Encoder at 1 RPM produces 0.24576 pulses per loop (2400 / 9765.625Hz)

  // Determine slip from the throttle
  // By happy coincidence this gives +/- 32768 (+/- 5Hz)
  int slip = adc_data[3] - 32768;
  // Increment the angle by the requested slip
  angle += slip;
  // Increment the angle by the encoder measurement
  angle += encoder_position_change * 67108864 / ENCODER_RESOLUTION * MOTOR_POLES / 2;

  // Wrap angle
  angle &= 0x3FFFFFF;

  // Calculate the target current.
  // This starts at MAGNETIZING_CURRENT and increases wih slip
  // up to a maximum of MAGNETIZING_CURRENT + MAX_TORQUE_CURRENT.
  // We use floating point operations here to avoid overflows
  int target_current;
  if(slip >= 0) {
    target_current = (float)MAGNETIZING_CURRENT + (float)MAX_TORQUE_CURRENT * (float)slip / 32768.0f;
  } else {
    target_current = (float)MAGNETIZING_CURRENT + (float)MAX_TORQUE_CURRENT * -(float)slip / 32768.0f;
  }

  // Apply current error to voltage output
  if(total_current < target_current && voltage < 4000) voltage += GAIN;
  if(total_current > target_current && voltage > 0)    voltage -= GAIN;

  // Update PWM outputs from SVM style lookup table
  TIM1->CCR3 = (voltage * table1[angle >> 13]) >> 15;
  TIM1->CCR2 = (voltage * table2[angle >> 13]) >> 15;
  TIM1->CCR1 = (voltage * table3[angle >> 13]) >> 15;

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
    uart_write_int(i1);
    uart_write_char(',');
    uart_write_int(i2);
    uart_write_char(',');
    uart_write_int(i3);
    uart_write_char(',');
    uart_write_int(total_current);
    uart_write_char(',');
    uart_write_int(voltage);
    uart_write_nl();
    int n; for(n=0;n<1000;n++) nop();
  }
}
