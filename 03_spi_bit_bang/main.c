#define STM32F051x8
#include "stm32f0xx.h"
#include <assert.h>

#define CORE_CLK_FREQ (48000000) // 48MHz
#define SYSTICK_FREQ (1000) // 1kHz
#define UINT24_MAX (0xFFFFFF)

// chip select
#define SPI_SET_CS()     ( GPIOC->ODR |=   GPIO_ODR_7  )
#define SPI_CLEAR_CS()   ( GPIOC->ODR &= (~GPIO_ODR_7) )
// clock
#define SPI_SET_CLK()    ( GPIOC->ODR |=   GPIO_ODR_8  )
#define SPI_CLEAR_CLK()  ( GPIOC->ODR &= (~GPIO_ODR_8) )
// mosi: master output slave input
#define SPI_SET_DATA()   ( GPIOC->ODR |=   GPIO_ODR_9  )
#define SPI_CLEAR_DATA() ( GPIOC->ODR &= (~GPIO_ODR_9) )
#define SPI_WAIT()       for(uint32_t spiWaitIdx = 0; spiWaitIdx < 100000;++spiWaitIdx)

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

// this may seems familiar because it is modified version _bsp_led_init.
// 2 led pins and additionally PC7 is used to bit bang SPI
void _spi_init(void) {
	// ld4 : PC8 blue
	// ld3 : PC9 yellow
	// So Cortex-M0 cares about power consumption due that clock are disabled by default.
	// We need to enable GPIOC clock. RM0091 page 122
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	// see all of definitions of gpio register at RM0091 page 158
	// set mode to output
	GPIOC->MODER &= ~(GPIO_MODER_MODER7 | GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
	GPIOC->MODER |= GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
	// set output type to push-pull
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_7 | GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
	// set output speed to high
	GPIOC->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR7 | GPIO_OSPEEDR_OSPEEDR8 | GPIO_OSPEEDR_OSPEEDR9;
	// set pull-up/pull-down to no pull-up no pull-down
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR7 | GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);
}

void _spi_send(uint8_t u8) {
	SPI_SET_CS();
	SPI_WAIT();
	uint8_t idx;
	for(idx = 0; idx < 8; ++idx) {
		SPI_CLEAR_CLK();
		if(u8 & 0x01) {
			SPI_SET_DATA();
		} else {
			SPI_CLEAR_DATA();
		}
		u8 = u8 >> 1;
		SPI_WAIT();
		SPI_SET_CLK();
		SPI_WAIT();
	}
	SPI_CLEAR_CS();
	SPI_WAIT();
}

int main(void) {
	_spi_init();
	while(1) {
		// welcome to embedded_abc project
		_spi_send('a');
		_spi_send('b');
		_spi_send('c');
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