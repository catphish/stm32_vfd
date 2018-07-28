#include <stdint.h>
#include <stm32l476xx.h>
#include "table.h"
#include "uart.h"
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)
#define PI 3.14159265f

// Performance metrics
int start_time, end_time;

// Debug
int id_debug, iq_debug;
int vd_debug, vq_debug;

// Analog to digital converter DMA data
uint16_t adc_data[4];

// Motor parameters
const float loop_period = 0.0001024f;
const float time_constant = 0.05f;
const float pole_pairs = 2.0f;

// For flux estimator / FOC
float imag;
float flux_angle;
uint32_t encoder_position_previous;
float vd, vq;

// This runs at the PWM frequency: 9765.625Hz
// It runs 0.000048s after it's triggered (approx half the PWM period)
// This means data is collected during the first half of each
// pwm cycle ready for calculations in the second half.
void DMA1_Channel1_IRQHandler(void) {
  start_time = TIM1 -> CNT;

  // Fetch the phase 1 and 2 currents
  int i1 = adc_data[0] - 32840;
  int i2 = adc_data[1] - 32840;

  float ia = i1;
  float ib = 0.577350269f * i1 + 1.154700538f * i2;

  float id = ia * cosf(flux_angle) + ib * sinf(flux_angle);
  float iq = ib * cosf(flux_angle) - ia * sinf(flux_angle);

  // Calculate encoder position change
  uint32_t encoder_position_current = TIM2->CNT;
  int encoder_position_change = encoder_position_current - encoder_position_previous;
  encoder_position_previous = encoder_position_current;

  // Convert encoder pulse count to (electrical) radians/second
  float rotor_velocity = encoder_position_change * 9765.625f / 2400.0f * PI * 2.0f * pole_pairs;

  // Flux angle estimator
  imag = imag + (loop_period / time_constant) * (id - imag);
  if(imag < 0) imag = -imag;
  if(imag != 0.0f) {
    float flux_slip_speed = (1.0f / time_constant) * (iq / imag);
    float flux_speed = rotor_velocity + flux_slip_speed;
    flux_angle = flux_angle + loop_period * flux_speed;
  }
  if(flux_angle > PI)  flux_angle -= PI*2.0f;
  if(flux_angle < -PI) flux_angle += PI*2.0f;

  // PID
  float id_target = 1000;
  float iq_target = (adc_data[3]-32768) / 4;
  float id_error = id - id_target;
  float iq_error = iq - iq_target;
  vd -= id_error * 0.1f;
  vq -= iq_error * 0.1f;

  // Limits
  if(vd >  4000) vd =  4000;
  if(vd < -4000) vd = -4000;
  if(vq >  4000) vq =  4000;
  if(vq < -4000) vq = -4000;

  // Voltage output
  float va = vd * cosf(flux_angle) - vq * sinf(flux_angle);
  float vb = vd * sinf(flux_angle) + vq * cosf(flux_angle);
  float vangle = atan2f(vb, va);
  float vamplitude = sqrtf(va * va + vb * vb);
  if(vamplitude > 4000) vamplitude = 4000;
  int voltage = vamplitude;
  int angle = ((vangle / PI) + 1.0f) * 67108863;

  // Debug output
  id_debug = id;
  iq_debug = iq;
  vd_debug = vd;
  vq_debug = vq;

  // Update PWM outputs from SVM style lookup table
  int phase = angle >> 14;
  TIM1->CCR3 = (voltage * table1[phase]) >> 15;
  TIM1->CCR2 = (voltage * table2[phase]) >> 15;
  TIM1->CCR1 = (voltage * table3[phase]) >> 15;

  end_time = TIM1 -> CNT;

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
    uart_write_byte(',');
    uart_write_int(iq_debug);
    uart_write_byte(',');
    uart_write_int(vd_debug);
    uart_write_byte(',');
    uart_write_int(vq_debug);
    uart_write_byte(',');
    uart_write_int(end_time - start_time);
    uart_write_nl();
    int n; for(n=0;n<1000;n++) nop();
  }
}
