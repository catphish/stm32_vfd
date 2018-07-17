#include <stdint.h>
#include <stm32l476xx.h>
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)
#define PI 3.14159265f

uint16_t adc_data[4];

int angle = 0;

int segment;
int a, b, c;
int v1, v2;
float i1, i2;
float ia, ib;
float va, vb;
float fa, fb;
float flux, torque;
float iangle;

void uart_write_string(char* str);
void uart_write_int(int32_t i);
void uart_write_nl();

void set_inverter_state(float angle) {

}

// This runs at 80000000/8192 = 9765.625Hz
void DMA1_Channel1_IRQHandler(void) {


  // // Increment angle manually at 10Hz
  // angle += (10*2*PI/9765.625);
  // if(angle >= PI) angle -= 2*PI;

  // vd = 1;
  // vq = 0;

  // // Inverse Park (vd, vq -> va, vb)
  // va = vd * cosf(angle) - vq * sinf(angle);
  // vb = vd * sinf(angle) + vq * cosf(angle);

  // // Inverse Clarke (va, vb -> v1, v3, v3)
  // v1 = va;
  // v2 = 0 - 0.5 * va + 0.866025404 * vb;
  // v3 = 0 - 0.5 * va - 0.866025404 * vb;

  // output_svm();

  i1 = (adc_data[0] - 2048 - 4) * 6.1f;
  i2 = (adc_data[1] - 2048)     * 6.1f;
  ia = i1;
  ib = 0.577350269f * i1 + 1.154700538f * i2;

  va = v1;
  vb = 0.577350269f * v1 + 1.154700538f * v2;

  fa += va - ia;
  fb += vb - ib;
  fa *= 0.999f;
  fb *= 0.999f;

  flux = sqrtf(fa*fa+fb*fb);
  torque = fa*ib-fb*ia;

  iangle = atan2f(ib, ia);
//iangle += 0.001;
//if(iangle > PI) iangle-=2*PI;
#define FLUX 200000
#define TORQUE 100000

  int vector;
  if(iangle > PI*5.0f/6.0f || iangle < PI*-5.0f/6.0f) {
    segment = 4;
  } else if(iangle < PI*-3.0f/6.0f) {
    segment = 5;
  } else if(iangle < PI*-1.0f/6.0f) {
    segment = 6;
  } else if(iangle > PI*3.0f/6.0f) {
    segment = 3;
  } else if(iangle > PI*1.0f/6.0f) {
    segment = 2;
  } else {
    segment = 1;
  }
  if(flux < FLUX) {
    if(torque < TORQUE) vector = segment + 1;
    else vector = segment - 1;
  } else {
    if(torque < TORQUE) vector = segment + 2;
    else vector = segment - 2;
  }
  if(vector < 1) vector += 6;
  if(vector > 6) vector -= 6;

  switch(vector) {
    case 1 : a=1;b=0;c=0; break;
    case 2 : a=1;b=1;c=0; break;
    case 3 : a=0;b=1;c=0; break;
    case 4 : a=0;b=1;c=1; break;
    case 5 : a=0;b=0;c=1; break;
    case 6 : a=1;b=0;c=1; break;
  }

  v1 = (a+a-b-c)*1000;
  v2 = (b+b-a-c)*1000;

  GPIOA->ODR = (a<<8)|(b<<9)|(c<<10);
  GPIOB->ODR = ((!a)<<13)|((!b)<<14)|((!c)<<15);

  DMA1->IFCR = 0xFFFFFFFF;

}

// void TIM1_UP_TIM16_IRQHandler(void) {
//   // Start ADC conversions after each PWM refresh
//   ADC1->CR |= ADC_CR_ADSTART;
//   TIM1->SR = ~TIM_SR_UIF;
// }

int main() {
  ADC1->CR |= ADC_CR_ADSTART;
  while(1) {
    // Debug output
    uart_write_int(flux);
    uart_write_string(",");
    uart_write_int(torque);
    uart_write_nl();
    int n; for(n=0;n<10000;n++) nop();
  }
}
