# 01_systick

This demo implements interrupt of systick timer. Interrupt occurs every 1 millisecond.

![SystickFeature](./doc/systick.png "SystickFeature")  
Figure 1: See _systick_init function how it is implemented.

## Test
![SystickTest](./doc/DS1Z_QuickPrint1.png "SystickTest")  
Figure 2: Systick interrupt handler occurs every 1 ms and toggles led pins.