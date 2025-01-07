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

static void _i2c2_io_init(void) {
	// AF1
	// PB10 I2C2_SCL
	// PB11 I2C2_SDA
	// clock enable
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	// alternate function
	GPIOB->MODER &= ~(GPIO_MODER_MODER10 | GPIO_MODER_MODER11);
	GPIOB->MODER |= GPIO_MODER_MODER10_1 | GPIO_MODER_MODER11_1;
	// open drain
	GPIOB->OTYPER |= GPIO_OTYPER_OT_10 | GPIO_OTYPER_OT_11;
	// high speed, who cares
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR10 | GPIO_OSPEEDR_OSPEEDR11;
	// pull up, we are going to internal pull up resistor for i2c
	GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR10 | GPIO_PUPDR_PUPDR11);
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR10_0 | GPIO_PUPDR_PUPDR11_0;
	// i2c alternate function
	GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL11);
	GPIOB->AFR[1] |= ((uint32_t)1<<GPIO_AFRH_AFSEL10_Pos) | ((uint32_t)1<<GPIO_AFRH_AFSEL11_Pos);
}

static void _i2c2_periph_init(void) {
	// enable clock
	RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
	// i2c register definitions starts from rm0091 684
	// it is good to disable periph before you are configuring it, if you can
	// because some config register may not let you otherwise
	// disable i2c
	I2C2->CR1 &= ~I2C_CR1_PE;
	// 7 bit addressing mode
	I2C2->CR2 &= ~I2C_CR2_ADD10;
	// number of bytes 2 to be able send b and c chars.
	I2C2->CR2 &= I2C_CR2_NBYTES;
	I2C2->CR2 |= ((uint32_t)2 << I2C_CR2_NBYTES_Pos);
	// standard 100kb/s. I used cubemx to generate this number
	// too much calculations in reference manual
	I2C2->TIMINGR = 0x10805D88;
	// enable back periph
	I2C2->CR1 |= I2C_CR1_PE;
}

static uint8_t _i2c2_get_clk_st() {
	return (GPIOB->IDR & GPIO_IDR_10) ? 1 : 0;
}

static void _i2c2_wait_tx_empty() {
	while((I2C2->ISR & I2C_ISR_TXE) == 0) {
		__asm__("nop");
	}
}

static void _dummy_delay(void) {
	// so I can modify with debugger if I want to
	static uint32_t delay = 10000;
	for(uint32_t idx = 0; idx < delay; idx++) {
		__asm__("nop");
	}
}

static void _fake_slave_init(void) {
	// pb12
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	// output
	GPIOB->MODER &= ~GPIO_MODER_MODER12;
	GPIOB->MODER |= GPIO_MODER_MODER12_0;
	// open drain
	GPIOB->OTYPER |= GPIO_OTYPER_OT_12;
	// high speed, who cares
	GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR12;
	// pull up
	GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR12;
	GPIOB->PUPDR |= GPIO_PUPDR_PUPDR12_0;
	// do not pull down the bus
	GPIOB->ODR |= GPIO_ODR_12;
}

// since I dont have real slave connected to it, I just pull down SDA line with another pin to fake ACK
static void _fake_slave(void) {
	uint8_t clkCount = 0;
	uint8_t clkSt = 1;
	uint8_t clkStOld = 1;

	while(1) {
		clkSt = _i2c2_get_clk_st();
		// count falling edge transitions
		if(clkSt == 0 && clkStOld == 1) {
			clkCount++;
		}
		// time to fake
		if(clkCount == 9) {
			// pull down SDA line
			GPIOB->ODR &= ~GPIO_ODR_12;
			while(_i2c2_get_clk_st() == 0);
			while(_i2c2_get_clk_st() == 1);
			// release SDA line
			GPIOB->ODR |= GPIO_ODR_12;
			// our faking is done here
			break;
		}
		clkStOld = clkSt;
	}
}

int main(void) {
	_fake_slave_init();
	_i2c2_io_init();
	_i2c2_periph_init();
	while(1) {
		// id of the slave set to a
		I2C2->CR2 &= ~I2C_CR2_SADD;
		I2C2->CR2 |= ((uint32_t)'a'<<1);
		// start sending
		I2C2->CR2 |= I2C_CR2_START;
		_fake_slave();

		// wait for to send b
		_i2c2_wait_tx_empty();
		I2C2->TXDR = 'b';
		_fake_slave();

		// wait for to send c
		_i2c2_wait_tx_empty();
		I2C2->TXDR = 'c';
		_fake_slave();

		// we are done here
		I2C2->CR2 |= I2C_CR2_STOP;
		_dummy_delay();
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
