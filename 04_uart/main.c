#define STM32F051x8
#include "stm32f0xx.h"
#include <assert.h>

#define CORE_CLK_FREQ (48000000u) // 48MHz
#define USART1_BAUD   (9600u)

// See 00_core_clk for more details of this implementation
// SystemInit is called before main
// called from startup_stm32f051r8tx.s
void SystemInit(void) {
	// todo: explain why we need to do this
	// prefetch buffer enable
	FLASH->ACR |= FLASH_ACR_PRFTBE;

	uint32_t tmpreg = 0;
	// Wait HSI to be ready. we dont have any other clock source
	while((RCC->CR & RCC_CR_HSION) == 0)
		;
	while((RCC->CR & RCC_CR_HSIRDY) == 0)
		;
	// rm00091 page 111
	// zero HSITRIM
	tmpreg = RCC->CR & (~RCC_CR_HSITRIM_Msk);
	// set to default value.
	tmpreg |= (16 << RCC_CR_HSITRIM_Pos);
	// now HSI SHOULD be 8MHz
	RCC->CR = tmpreg;

	// Hmm, it should be impossible that PLL is the source of system clock
	assert(!((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL));
	// Disable PLL. We could not modify PLL config otherwise. Example:
	// rm00091 page 113
	// PLLMUL[3:0]: PLL multiplication factor
	// These bits are written by software to define the PLL multiplication factor. These bits can be
	// written only when PLL is disabled.
	RCC->CR &= (~RCC_CR_PLLON);

	// Wait for PLL to be disabled
	while((RCC->CR & RCC_CR_PLLRDY) != 0)
		;

	// reset prediv bits
	tmpreg = RCC->CFGR2 & (~RCC_CFGR2_PREDIV);
	tmpreg |= RCC_CFGR2_PREDIV_DIV2;
	// from architecture it looks like we dont need it but RCC_CFGR2_PLLSRC documentation is weird
	// lets set it to HSI/2 to be safe
	RCC->CFGR2 = tmpreg;

	// reset pll src. source is is HSI/2
	tmpreg = RCC->CFGR & (~RCC_CFGR_PLLSRC);
	// hsi div2 is the source of pll
	tmpreg |= RCC_CFGR_PLLSRC_HSI_DIV2;
	RCC->CFGR = tmpreg;
	
	// reset pll mul bits
	tmpreg = RCC->CFGR & (~RCC_CFGR_PLLMUL);
	// pll mul is 12
	tmpreg |= RCC_CFGR_PLLMUL12;
	RCC->CFGR = tmpreg;

	// sys clk source is 48MHz
	RCC->CR |= RCC_CR_PLLON;
	// wait for pll to be enabled
	while((RCC->CR & RCC_CR_PLLRDY) == 0)
		;
	// todo: explain why we need to do this
	// turns out you even need set flash latency
	FLASH->ACR |= FLASH_ACR_LATENCY;
	// wait for flash latency to be set or just wait forever I ll figure out a way to fix it
	while((FLASH->ACR & FLASH_ACR_LATENCY) == 0)
		;

	// The AHB and the APB domains maximum frequency is 48 MHz.
	// All buses are clocked at maximum frequency.
	tmpreg = RCC->CFGR & (~RCC_CFGR_PPRE);
	// APB prescaler is 1
	tmpreg |= RCC_CFGR_PPRE_DIV1;
	RCC->CFGR = tmpreg;
	tmpreg = RCC->CFGR & (~RCC_CFGR_HPRE);
	// AHB prescaler is 1
	tmpreg |= RCC_CFGR_HPRE_DIV1;
	RCC->CFGR = tmpreg;

	// finally start using PLL as system clock
	tmpreg = RCC->CFGR & (~RCC_CFGR_SW);
	tmpreg |= RCC_CFGR_SW_PLL;
	RCC->CFGR = tmpreg;
}

static void _usart1_io_init(void) {
	// pa9 USART1_TX
	// enabled bus
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	// see all of definitions of gpio register at RM0091 page 158
	// set mode to output
	GPIOA->MODER &= (~GPIO_MODER_MODER9);
	// alternate function mode
	GPIOA->MODER |= GPIO_MODER_MODER9_1;
	// set output type to push-pull
	GPIOA->OTYPER &= (~GPIO_OTYPER_OT_9);
	// set output speed to high
	GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR9;
	// set pull-up/pull-down to no pull-up no pull-down
	GPIOA->PUPDR &= (~GPIO_PUPDR_PUPDR9);
	// ref stm32f051r8.pdf page 37
	//| Pin name  | AF0        | AF1       | AF2      | AF3        |
	//|-----------|------------|-----------|----------|------------|
	//| PA9       | TIM15_BKIN | USART1_TX | TIM1_CH2 | TSC_G4_IO1 |
	// RM00091 page 163
	GPIOA->AFR[1] &= (~GPIO_AFRH_AFSEL9);
	GPIOA->AFR[1] |= ((uint32_t)0b01 << GPIO_AFRH_AFSEL9_Pos);
}

static void _usart1_periph_init(void) {
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	// RM0091 page 744
	// disable usart1
	USART1->CR1 &= (~USART_CR1_UE);
	// word length 8
	USART1->CR1 &= (~(USART_CR1_M | ((uint32_t)1<<28)));
	// over sampling 16, this is important when calculation baudrate value
	USART1->CR1 &= (~USART_CR1_OVER8);
	// even parity
	USART1->CR1 &= (~USART_CR1_PS);
	// enable parity
	USART1->CR1 |= USART_CR1_PCE;
	// 1 stop bit
	USART1->CR2 &= (~USART_CR2_STOP);
	// RM0091 page 716 to see how baudrate generation register value calculated.
	USART1->BRR = CORE_CLK_FREQ / USART1_BAUD;
	USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static inline void _usart1_tx(uint8_t u8) {
	// wait until transmit data register empty
	while((USART1->ISR & USART_ISR_TXE) == 0)
		;
	USART1->TDR = u8;
}

int main(void) {
	_usart1_io_init();
	_usart1_periph_init();
	uint32_t delay = 10000;
	while(1) {
		// welcome to embedded_abc project
		_usart1_tx('a');
		// this delay is important when you dont have quality tools
		// I spent 2 hours why it is not working because dumb ass picoscope can not keep up
		// with data and serial decoding does not work
		for(uint32_t idx = 0; idx < delay; idx++);
			;
		_usart1_tx('b');
		for(uint32_t idx = 0; idx < delay; idx++);
			;
		_usart1_tx('c');
		for(uint32_t idx = 0; idx < delay; idx++);
			;
	}
}

void NMI_Handler(void) {
	while(1)
		;
}

void HardFault_Handler(void) {
	while(1)
		;
}

void SVC_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
}
