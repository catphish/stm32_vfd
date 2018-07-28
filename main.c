#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include "uart.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)
#define PI 3.14159265f

// Performance metrics
int calculation_cycles;

// Debug data
int id_debug, iq_debug;
int vd_debug, vq_debug;

// Analog to digital converter DMA data
uint16_t adc_data[4];

// Motor parameters
const float loop_frequency = 9765.625f; // I use 8192 cycles of 80MHz MCU
const float loop_period = 0.0001024f; // 1/9765.625
const float time_constant = 0.06f; // Black magic and guesswork
const float motor_poles = 4.0f; // 4 Pole motor
const float encoder_resolution = 2400.0f; // Encoder produces 1200PPR, we use both lines
const float id_target = 3000; // Arbitrary choice of magnetising current
const float gain = 0.05f; // Gain for corrective voltage changes

// For flux estimator / FOC
float imag; // Stores current magnetizin current
float flux_angle; // The all important flux angle
uint32_t encoder_position_previous; // Does what it says it does
float vd, vq; // Persistent outputs voltages

// This runs at the PWM frequency: 9765.625Hz
// It runs 0.000048s after it's triggered (approx half the PWM period)
// This means data is collected during the first half of each
// pwm cycle ready for calculations in the second half.
void DMA1_Channel1_IRQHandler(void) {
  int start_time = TIM1 -> CNT;

  // Fetch the phase 1 and 2 currents
  int i1 = adc_data[0] - 32840; // Offset to 0A
  int i2 = adc_data[1] - 32840; // Offset to 0A

  // Clarke
  float ia = i1;
  float ib = 0.577350269f * i1 + 1.154700538f * i2;

  // Park
  float id = ia * cosf(flux_angle) + ib * sinf(flux_angle);
  float iq = ib * cosf(flux_angle) - ia * sinf(flux_angle);

  // Calculate encoder ticks change since the last iteration
  uint32_t encoder_position_current = TIM2->CNT; // Atomic as possible
  int encoder_position_change = encoder_position_current - encoder_position_previous; // Nasty casting to signed here
  encoder_position_previous = encoder_position_current;

  // Convert encoder pulse count to (electrical) radians/second
  float rotor_velocity = encoder_position_change * loop_frequency / encoder_resolution * PI * motor_poles;

  /////////////////////////
  // Flux angle estimator//
  /////////////////////////
  // This is based largely on the algorithm described here:
  // http://ww1.microchip.com/downloads/en/AppNotes/ACIM%20Vector%20Control%2000908a.pdf

  imag = imag + (loop_period / time_constant) * (id - imag);
  if(imag < 0) imag = -imag; // Magnetizing current is always positive, this avoids randomly switching pole

  if(imag != 0.0f) { // Avoid divide by zero
    float flux_slip_speed = (1.0f / time_constant) * (iq / imag);
    float flux_speed = rotor_velocity + flux_slip_speed;
    flux_angle = flux_angle + loop_period * flux_speed;
  }
  // Wrap the angle to maintain precision
  if(flux_angle > PI)  flux_angle -= PI*2.0f;
  if(flux_angle < -PI) flux_angle += PI*2.0f;

  // Calculate current errors
  float iq_target = (adc_data[3]-32768) / 8; // Torque current target from throttle +/-
  float id_error = id - id_target; // Torque current error
  float iq_error = iq - iq_target; // Flux current error
  vd -= id_error * gain; // Apply errors to ouutput voltages according to gain
  vq -= iq_error * gain; // Apply errors to ouutput voltages according to gain

  // Limits - This is very primative, we do not support field weakening yet
  // We limit to 4000 (98% of the 12 bit inverter)
  if(vd >  4000.0f) vd =  4000.0f; // Limit flux current directly
  if(vd < -4000.0f) vd = -4000.0f; // Limit flux current directly
  // If the total exceeds 4000, we weaken the torque
  if(sqrtf(vd*vd+vq*vq) > 4000.0f) {
    // Absolute value of torque to bring us up to 4000
    float vqtmp = sqrtf(16000000.0f - vd * vd);
    // Use the new value according to the original direction
    if(vq > 0) vq = vqtmp; else vq = -vqtmp;
  }

  // Inverse Park
  float va = vd * cosf(flux_angle) - vq * sinf(flux_angle);
  float vb = vd * sinf(flux_angle) + vq * cosf(flux_angle);

  // Instead of inverse clark, we convert to an angle and amplitude
  float vangle = atan2f(vb, va);
  int angle = ((vangle / PI) + 1.0f) * 4095;
  int voltage = sqrtf(va * va + vb * vb);

  // Update PWM outputs from SVM style lookup table
  TIM1->CCR3 = (voltage * table1[angle]) >> 15;
  TIM1->CCR2 = (voltage * table2[angle]) >> 15;
  TIM1->CCR1 = (voltage * table3[angle]) >> 15;

  // Debug output
  id_debug = id;
  iq_debug = iq;
  vd_debug = vd;
  vq_debug = vq;

  // Store the number of cycles it took to do the math
  calculation_cycles = start_time - TIM1 -> CNT;

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
    uart_write_int(id_debug);
    uart_write_char(',');
    uart_write_int(iq_debug);
    uart_write_char(',');
    uart_write_int(vd_debug);
    uart_write_char(',');
    uart_write_int(vq_debug);
    uart_write_char(',');
    uart_write_int(calculation_cycles);
    uart_write_nl();
    int n; for(n=0;n<1000;n++) nop();
  }
}
