#include "stm32f10x.h"

extern volatile int timer_ms;

void delay_ms(int time)
{
	timer_ms = time;
	while (timer_ms) {};
}

int main(void)
{
	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_5;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio);

	SysTick_Config(SystemCoreClock / 1000);

	while (1) {
		GPIO_SetBits(GPIOA, GPIO_Pin_5); // zapalenie diody
		delay_ms(500);
		GPIO_ResetBits(GPIOA, GPIO_Pin_5); // zgaszenie diody
		delay_ms(1000);
	}
}
