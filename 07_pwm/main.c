#define STM32F051x8
#include "stm32f0xx.h"
#include <assert.h>

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

static void _pwm_io_init(void) {
	// ld4 : PC8 blue
	// ld3 : PC9 yellow
	// So Cortex-M0 cares about power consumption due that clock are disabled by default.
	// We need to enable GPIOC clock. RM0091 page 122
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	// see all of definitions of gpio register at RM0091 page 158
	// set mode to alternate(IMPORTANT)
	GPIOC->MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
	GPIOC->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1;
	// set output type to push-pull
	GPIOC->OTYPER &= ~(GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
	// set output speed to high
	GPIOC->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR8 | GPIO_OSPEEDR_OSPEEDR9;
	// set pull-up/pull-down to no pull-up no pull-down
	GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR8 | GPIO_PUPDR_PUPDR9);
}

// this functions will configure a 10kHz pwm
static void _pwm_periph_init(void) {
	// enable the clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	// no clock division
	TIM3->CR1 &= ~TIM_CR1_CKD;
	TIM3->PSC = 47;
	// auto reload register, it will count up to this value
	TIM3->ARR = 1000;

	// pwm mode 1 selected
	TIM3->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3PE;
	TIM3->CCMR2 |= TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4PE;

	// capture compare enable
	TIM3->CCER |= TIM_CCER_CC3E | TIM_CCER_CC4E;

	// arpe and ug necessary so config takes effect
	TIM3->CR1 |= TIM_CR1_ARPE;
	TIM3->EGR |= TIM_EGR_UG;
	// enable the timer
	TIM3->CR1 |= TIM_CR1_CEN;
}

static void _pwm_set_duty_cycle(uint8_t percentage) {
	uint16_t dutyVal = (uint16_t)((TIM3->ARR * percentage) / 100.0);
	TIM3->CCR3 = dutyVal;
	TIM3->CCR4 = dutyVal;
}

int main(void) {
	_pwm_io_init();
	_pwm_periph_init();

	uint8_t brightness = 0;
	uint8_t brightnessDir = 0;

	while(1) {
		// heartbeat
		if(brightnessDir == 0) { // decrease
			if(((int16_t)brightness - 1) < 0) {
				// change direction when you could not decrease anymore
				brightnessDir = 1;
			} else {
				brightness--;
			}
		} else { // increase
			if((brightness+1) > 100) {
				// change direction when you could not increase anymore
				brightnessDir = 0;
			} else {
				brightness++;
			}
		}
		for(int i = 0; i < 30000; ++i) {
			__asm__("nop");
		}
		_pwm_set_duty_cycle(brightness);
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
