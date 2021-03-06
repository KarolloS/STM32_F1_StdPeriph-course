#include <stdio.h>
#include "stm32f10x.h"

void SysTick_Handler(void);
void USART2_IRQHandler(void);
void send_char(char c);
void send_string(char s[]);
void delay_ms(uint32_t time);
void __io_putchar(char c);
void send_int(unsigned int d);
void send_float(float f,int po_przecinku);


volatile uint32_t timer_ms;
volatile uint8_t odebrano_polecenie, RxBuf[100], TxBuf[100], RxIndex , TxIndex;


void SysTick_Handler(void)
{
	if (timer_ms)
	{
		timer_ms--;
	}
}

void USART2_IRQHandler(void)
{
	char c;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		RxBuf[RxIndex] = USART_ReceiveData(USART2);
		c = (char)RxBuf[RxIndex];
		send_char(c);
		RxIndex++;

		if(RxBuf[RxIndex-1] == 0x0D) //znak 'enter'
		{
			send_string("\r\n");
			odebrano_polecenie = 1;
			RxIndex = 0;
		}
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{
		USART_SendData(USART2, TxBuf[TxIndex++]);

		if(TxBuf[TxIndex-1] == 0) //znak 'null'
		{
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			TxIndex = 0;
		}
	}
}

void send_char(char c)
{
	TxBuf[0]=c;
	TxBuf[1]=0; //znak 'null'

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(2);
}

void send_string(char s[])
{
	uint8_t i = 0;

	for (i = 0; i<100; i++)
	{
		TxBuf[i]=s[i];
		if (s[i] == 0) break; //znak 'null'
	}


	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(5);
}

void delay_ms(uint32_t time)
{
	timer_ms = time;
	while (timer_ms) {};
}

void __io_putchar(char c)
{
	//////// for printf function ////////
	if (c=='\n')
		send_char('\r');
	send_char(c);
}

void send_int(unsigned int d)
{
	int base = 10;
	int i = 0;
	int div = 1;
	while (d/div >= base)
		div *= base;

	while (div != 0)
	{
		int num = d/div;
		d = d%div;
		div /= base;
		if (num > 9)
		{
			TxBuf[i] = (num-10) + 'A';
			i++;
		}
		else
		{
			TxBuf[i] = num + '0';
			i++;
		}
	}
	TxBuf[i] = 0;

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(3);
}

void send_float(float f, int po_przecinku)
{
	int d1, d2;
	char flag = 0;
	uint8_t i = 0;
	d1 = (int) f;
	send_int(d1);
	send_char('.');
	while (i < po_przecinku)
	{
		f = f * 10;
		d2 = (int) f;

		if(d2 == 0 && flag == 0)
		{
			send_int(0);
		}
		else
		{
			flag = 1;
		}

		d1 = d1 * 10;
		i++;
	}

	d2 = d2 - d1;
	send_int(d2);
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;
	TIM_TimeBaseInitTypeDef tim;
	TIM_OCInitTypeDef  channel;
	NVIC_InitTypeDef nvic;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	////////// GPIO for UART configuration //////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	///////////// GPIO configuration ////////////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &gpio);

	//////////////// Timer configuration ////////////
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 3600 - 1;
	tim.TIM_Period = 100 - 1;
	TIM_TimeBaseInit(TIM4, &tim);

	///////// Timer channel configuration ///////////
	TIM_OCStructInit(&channel);
	channel.TIM_OCMode = TIM_OCMode_PWM1;
	channel.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC1Init(TIM4, &channel);
	TIM_OC2Init(TIM4, &channel);
	TIM_OC3Init(TIM4, &channel);
	TIM_OC4Init(TIM4, &channel);

	TIM_Cmd(TIM4, ENABLE);

	///////////// UART configuration ////////////
	USART_StructInit(&uart);
	uart.USART_BaudRate = 115200;
	USART_Init(USART2, &uart);

	///// UART interrupts ON
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);

	///////// NVIC for UART configuration /////////
	nvic.NVIC_IRQChannel = USART2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	//////////// SysTick ON /////////////////
	SysTick_Config(SystemCoreClock / 1000);


	while (1)
	{
		int i;

		for(i=0;i<=100;i++)
		{
			TIM_SetCompare1(TIM4, i);
			delay_ms(10);
		}

		for(i=100;i>=0;i--)
		{
			TIM_SetCompare1(TIM4, i);
			delay_ms(10);
		}

		for(i=0;i<=100;i++)
		{
			TIM_SetCompare2(TIM4, i);
			delay_ms(10);
		}

		for(i=100;i>=0;i--)
		{
			TIM_SetCompare2(TIM4, i);
			delay_ms(10);
		}

		for(i=0;i<=100;i++)
		{
			TIM_SetCompare3(TIM4, i);
			delay_ms(10);
		}

		for(i=100;i>=0;i--)
		{
			TIM_SetCompare3(TIM4, i);
			delay_ms(10);
		}

		for(i=0;i<=100;i++)
		{
			TIM_SetCompare4(TIM4, i);
			delay_ms(10);
		}

		for(i=100;i>=0;i--)
		{
			TIM_SetCompare4(TIM4, i);
			delay_ms(10);
		}

	}

}
