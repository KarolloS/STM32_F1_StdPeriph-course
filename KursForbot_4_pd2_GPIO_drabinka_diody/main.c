#include "stm32f10x.h"

extern volatile uint32_t timer_ms;
uint32_t led = 0;
int i=0;

void delay_ms(int time)
{
	timer_ms = time;
	while (timer_ms) {};
}

void EXTI15_10_IRQHandler()
{
	if (EXTI_GetITStatus(EXTI_Line13)) {
		if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 0)	// jesli przycisk jest przycisniety
			{
				GPIO_ResetBits(GPIOC, 1 << led);
				GPIO_ResetBits(GPIOA, GPIO_Pin_5);

				if (i==0)
					{
						led++;
					}
				else if (i==1)
					{
						led--;
					}

				if (led == 9)
					{
						i=1;
					}
				else if (led == 0)
					{
						i=0;
					}

				GPIO_SetBits(GPIOC, 1 << led);
			}
		else
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_5);
			}

		EXTI_ClearITPendingBit(EXTI_Line13);
	}
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	EXTI_InitTypeDef exti;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = GPIO_Pin_5;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_13;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|
					GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);

	EXTI_StructInit(&exti);
	exti.EXTI_Line = EXTI_Line13;
	exti.EXTI_Mode = EXTI_Mode_Interrupt;
	exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	exti.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti);

	nvic.NVIC_IRQChannel = EXTI15_10_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	SysTick_Config(SystemCoreClock / 1000);

	GPIO_SetBits(GPIOC, 1 << led);
	GPIO_SetBits(GPIOA, GPIO_Pin_5);

	while (1)
		{
		}
}
