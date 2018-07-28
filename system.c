#include <stm32l476xx.h>

#define nop()  __asm__ __volatile__ ("nop" ::)

void SystemInitError(uint8_t error_source) {
  while(1);
}

extern volatile uint32_t adc_data[];

void SystemInit() {

  // Enable FPU
  SCB->CPACR |= 0xf00000;

  RCC->CR |= (1<<8);
  /* Wait until HSI ready */
  while ((RCC->CR & RCC_CR_HSIRDY) == 0);

  /* Disable main PLL */
  RCC->CR &= ~(RCC_CR_PLLON);
  /* Wait until PLL ready (disabled) */
  while ((RCC->CR & RCC_CR_PLLRDY) != 0);

  /* Configure Main PLL */
  RCC->PLLCFGR = (1<<24)|(10<<8)|2;

  /* PLL On */
  RCC->CR |= RCC_CR_PLLON;
  /* Wait until PLL is locked */
  while ((RCC->CR & RCC_CR_PLLRDY) == 0);

  /*
   * FLASH configuration block
   * enable instruction cache
   * enable prefetch
   * set latency to 2WS (3 CPU cycles)
   */
  FLASH->ACR |= FLASH_ACR_ICEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_2WS;

  /* Set clock source to PLL */
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  /* Check clock source */
  while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL);

  // Enable GPIOA clock
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  // Enable GPIOB clock
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  // Enable GPIOC clock
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

  // A2 -> USART2_TX
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2_Msk;
  GPIOA->AFR[0] |= GPIO_AFRL_AFSEL2_0 | GPIO_AFRL_AFSEL2_1 | GPIO_AFRL_AFSEL2_2;
  // A0/A1 -> TIM2
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL0_Msk;
  GPIOA->AFR[0] |= GPIO_AFRL_AFSEL0_0;
  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL1_Msk;
  GPIOA->AFR[0] |= GPIO_AFRL_AFSEL1_0;
  // A8,A9,A10 -> TIM1
  GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL8_Msk|GPIO_AFRH_AFSEL9_Msk|GPIO_AFRH_AFSEL10_Msk);
  GPIOA->AFR[1] |= GPIO_AFRH_AFSEL8_0 | GPIO_AFRH_AFSEL9_0 | GPIO_AFRH_AFSEL10_0;
  // B13,B14,B15 -> TIM1
  GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10_Msk|GPIO_AFRH_AFSEL11_Msk|GPIO_AFRH_AFSEL13_Msk|GPIO_AFRH_AFSEL14_Msk|GPIO_AFRH_AFSEL15_Msk);
  GPIOB->AFR[1] |= GPIO_AFRH_AFSEL10_0 | GPIO_AFRH_AFSEL11_0 | GPIO_AFRH_AFSEL13_0 | GPIO_AFRH_AFSEL14_0 | GPIO_AFRH_AFSEL15_0;

  // PORTA Modes
  GPIOA->MODER = 0xABEAFFEA;
  // PORTB Modes
  GPIOB->MODER = 0xABAFFFFF;
  // PORTC Modes
  GPIOC->MODER = 0xFFFFFFFF;
  GPIOC->ASCR = 0xF; // ADC

  // Enable USART2 clock
  RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
  // Disable USART2
  USART2->CR1 = 0;
  // Set data rate
  USART2->BRR = 694; //115200
  //USART2->BRR = 80; //1M
  // Enable USART2
  USART2->CR1 |= USART_CR1_UE;
  // Enable transmit
  USART2->CR1 |= USART_CR1_TE;

  // DMA for ADC1
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // DMA1
  // Wait a bit
  nop(); nop(); nop(); nop(); nop(); nop();
  DMA1_Channel1->CPAR = (uint32_t)&(ADC123_COMMON->CDR);
  DMA1_Channel1->CMAR = (uint32_t)&adc_data;
  DMA1_Channel1->CNDTR = 2;
  DMA1_Channel1->CCR = DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC |
                       DMA_CCR_CIRC | DMA_CCR_TCIE | DMA_CCR_EN;

  // ADC123
  RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
  // Connect to system clock
  RCC->CCIPR |= RCC_CCIPR_ADCSEL_0 | RCC_CCIPR_ADCSEL_1;
  // Divide clock (/8)
  ADC123_COMMON->CCR |= ADC_CCR_PRESC_2;
  // Dual mode
  ADC123_COMMON->CCR |= ADC_CCR_DUAL_1 | ADC_CCR_DUAL_2;
  // MDMA
  ADC123_COMMON->CCR |= ADC_CCR_MDMA_1;

  // ADC1+2+3
  // Disable DEEPPWD, enable ADVREGEN
  ADC1->CR = ADC_CR_ADVREGEN;
  ADC2->CR = ADC_CR_ADVREGEN;

  // Wait a bit
  int n; for(n=0;n<100000;n++) nop();

  // Configure ADC1/2 IN5 for differential input
  //ADC1->DIFSEL |= (1<<5);
  //ADC2->DIFSEL |= (1<<5);

  // Calibrate
  ADC1->CR |= ADC_CR_ADCAL;
  ADC2->CR |= ADC_CR_ADCAL;
  while(ADC1->CR & ADC_CR_ADCAL);
  while(ADC2->CR & ADC_CR_ADCAL);
  // Wait a bit
  for(n=0;n<100000;n++) nop();

  // Enable procedure
  ADC1->ISR |= ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  ADC2->ISR |= ADC_ISR_ADRDY;
  ADC2->CR |= ADC_CR_ADEN;
  while(!(ADC1->ISR & ADC_ISR_ADRDY));
  while(!(ADC2->ISR & ADC_ISR_ADRDY));

  // Sequence
  ADC1->SQR1 = (1<<6) | (3<<12) | 1;
  ADC2->SQR1 = (2<<6) | (4<<12) | 1;

  // Oversampling (16x)
  ADC1->CFGR2 = (3<<2) | 1;
  ADC2->CFGR2 = (3<<2) | 1;

  // TIM1
  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
  TIM1->CR1 &= ~1;
  TIM1->ARR = 8192;
  TIM1->CCR1 = 0;
  TIM1->CCR2 = 0;
  TIM1->CCR3 = 0;
  TIM1->CCMR1 = (6<<12)|(6<<4);
  TIM1->CCMR2 = (6<<4);
  TIM1->CCER  = (1<<0)|(1<<4)|(1<<8);  // Positive
  TIM1->CCER |= (1<<2)|(1<<6)|(1<<10); // Negative
  // Interrupt on overflow
  TIM1->DIER = TIM_DIER_UIE;
  // Dead time is defines as a multiple of the clock interval 12.5ns
  // 24 x 15.s = 300ns
  //TIM1->BDTR = (1<<15) | 24;
  TIM1->BDTR = (1<<15) | 5;
  TIM1->CR1 |= 1;

  // TIM2
  GPIOA->PUPDR = 1 | 1<<2;
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
  TIM2->SMCR = 3; // Encoder mode 3
  TIM2->CCMR1 = 1 | (1<<8) | (3<<4) | (3<<12);
  TIM2->CCER  = TIM_CCER_CC1E | TIM_CCER_CC2E;
  TIM2->CR1 |= 1;

  // Global interrupt config
  NVIC->ISER[0] = (1 << TIM1_UP_TIM16_IRQn) | (1 << DMA1_Channel1_IRQn);
}
