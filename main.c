#include <stdint.h>
#include <stm32l476xx.h>
#include <math.h>

#define nop()  __asm__ __volatile__ ("nop" ::)
#define PI 3.14159265
#define PI31 1.04719755
#define PI32 2.0943951
#define PI34 4.1887902

uint16_t adc_data[4];

int angle = 0;

float i1t, i2t, i3t;
float i1, i2, i3;
float v1, v2, v3;
float ia, ib, va, vb;
float vd, vq, id, iq;
float ccr1, ccr2, ccr3;
float fa, fb;
float flux, torque;
float iangle;

void uart_write_string(char* str);
void uart_write_int(int32_t i);
void uart_write_nl();

void set_inverter_state(float angle) {

}

// void output_svm() {
//   float bottom_line;
//   // Offset and scale the sine waves to maximize voltage
//   if(v1 <= v2) {
//     if (v1 <= v3) {
//       bottom_line = v1;
//     } else {
//       bottom_line = v3;
//     }
//   } else {
//     if (v3 <= v2) {
//       bottom_line = v3;
//     } else {
//       bottom_line = v2;
//     }
//   }

//   ccr1 = (v1 - bottom_line) * 4096;
//   ccr2 = (v2 - bottom_line) * 4096;
//   ccr3 = (v3 - bottom_line) * 4096;

//   TIM1->CCR1 = ccr1;
//   TIM1->CCR2 = ccr2;
//   TIM1->CCR3 = ccr3;
// }

// This runs at 80000000/8192 = 9765.625Hz
void DMA1_Channel1_IRQHandler(void) {

  // fa += va - ia;
  // fb += vb - ib;
  // fa *= 0.998;
  // fb *= 0.998;

  // flux = sqrtf(fa*fa+fb*fb);
  // torque = fa*ib-fb*ia;

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

  angle += 20;
  if(angle > 62833) angle -= 62833;

    i1 = (adc_data[0] - 2048 - 4);
    i2 = (adc_data[1] - 2048);
    i3 = 0 - i1 - i2;
    // va = v1;
    // vb = 0.577350269 * v1 + 1.154700538 * v2;
    //ia = i1;
    //ib = 0.577350269 * i1 + 1.154700538 * i2;
    //iangle = atan2f(ib, ia);

    i1t = 200 * sinf((float)angle/10000);
    i2t = 200 * sinf((float)angle/10000 + PI32);
    i3t = 200 * sinf((float)angle/10000 + PI34);

    if(i1 < i1t - 10) {
      GPIOA->BSRR = (1<<8);
      GPIOB->BSRR = (1<<29);
    } else if (i1 > i1t + 10) {
      GPIOA->BSRR = (1<<24);
      GPIOB->BSRR = (1<<13);
    } else {
      GPIOA->BSRR = (1<<24);
      GPIOB->BSRR = (1<<29);
    }

    if(i2 < i2t - 10) {
      GPIOA->BSRR = (1<<9);
      GPIOB->BSRR = (1<<30);
    } else if (i2 > i2t + 10) {
      GPIOA->BSRR = (1<<25);
      GPIOB->BSRR = (1<<14);
    } else {
      GPIOA->BSRR = (1<<25);
      GPIOB->BSRR = (1<<30);
    }

    if(i3 < i3t - 10) {
      GPIOA->BSRR = (1<<10);
      GPIOB->BSRR = (1<<31);
    } else if (i3 > i3t + 10) {
      GPIOA->BSRR = (1<<26);
      GPIOB->BSRR = (1<<15);
    } else {
      GPIOA->BSRR = (1<<26);
      GPIOB->BSRR = (1<<31);
    }

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
    uart_write_int(i1*1000);
    uart_write_string(",");
    uart_write_int(i2*1000);
    uart_write_string(",");
    uart_write_int(i3*1000);
    uart_write_string(",");
    uart_write_int(i1t*1000);
    uart_write_string(",");
    uart_write_int(i2t*1000);
    uart_write_string(",");
    uart_write_int(i3t*1000);
    uart_write_nl();
    int n; for(n=0;n<10000;n++) nop();
  }
}
