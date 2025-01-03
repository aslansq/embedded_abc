#define STM32F051x8
#include "stm32f0xx.h"
#include <assert.h>

#define CORE_CLK_FREQ (48*10e6)
#define SYSTICK_FREQ  (10e3)

void SystemInit(void) {
	// prefetch buffer enable
	FLASH->ACR |= FLASH_ACR_PRFTBE;

	uint32_t tmpreg = 0;
	// Wait HSI to be ready. we dont have any other clock source
	while((RCC->CR & RCC_CR_HSION) == 0)
		;
	while((RCC->CR & RCC_CR_HSIRDY) == 0)
		;
	// zero HSITRIM
	tmpreg = RCC->CR & (~RCC_CR_HSITRIM_Msk);
	// set to default value. ref Page 111 of RM0091
	tmpreg |= (16 << RCC_CR_HSITRIM_Pos);
	// now HSI SHOULD be 8MHz
	RCC->CR = tmpreg;

	// Hmm, it should be impossible that PLL is the source of system clock
	assert(!((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL));
	// Disable PLL. We could not modify PLL config otherwise
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
	// turns out you even need set flash latency
	FLASH->ACR |= FLASH_ACR_LATENCY;
	// wait for flash latency to be set or just wait forever I ll figure out a way to fix it
	while((FLASH->ACR & FLASH_ACR_LATENCY) == 0)
		;

	// The AHB and the APB domains maximum frequency is 48 MHz.
	tmpreg = RCC->CFGR & (~RCC_CFGR_PPRE);
	tmpreg |= RCC_CFGR_PPRE_DIV1;
	RCC->CFGR = tmpreg;
	tmpreg = RCC->CFGR & (~RCC_CFGR_HPRE);
	tmpreg |= RCC_CFGR_HPRE_DIV1;
	RCC->CFGR = tmpreg;

	// finally start using PLL as system clock
	tmpreg = RCC->CFGR & (~RCC_CFGR_SW);
	tmpreg |= RCC_CFGR_SW_PLL;
	RCC->CFGR = tmpreg;
}

void _systick_init(void) {
	// set reload value
	SysTick->LOAD = (CORE_CLK_FREQ/SYSTICK_FREQ) - 1;
	// set priority
	NVIC_SetPriority(SysTick_IRQn, 0); // lowest priority
	//enable systick
	//The RCC feeds the Cortex System Timer (SysTick) external clock with the AHB clock
	//(HCLK) divided by 8. The SysTick can work either with this clock or directly with the Cortex
	//clock (HCLK), configurable in the SysTick Control and Status Register.
	//see cortex m0 programming manual. SysTick control and status register (STK_CSR)
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

void _bsp_led_init(void) {
	// ld4 : PC8
	// ld3 : PC9
	// enable GPIOC
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	// set mode to output
	GPIOC->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
	GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
	// set output type to push-pull
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
	// set output speed to high
	GPIOC->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR8 | GPIO_OSPEEDR_OSPEEDR9;
	// set pull-up/pull-down to no pull-up no pull-down
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);
}

int main(void) {
	_systick_init();
	_bsp_led_init();
	while(1)
		;
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
	static uint32_t counter = 0;
	if(counter == 0) {
		GPIOC->ODR ^= GPIO_ODR_8;
		GPIOC->ODR = (GPIOC->ODR & GPIO_ODR_8) ? (GPIOC->ODR & ~GPIO_ODR_9) : (GPIOC->ODR | GPIO_ODR_9);
	}
	counter = (counter + 1) % 1000;
}
