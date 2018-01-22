#include "stm32f10x.h"

void SysTick_Handler(void);
void USART2_IRQHandler(void);
void send_char(char c);
void send_string(char s[]);
void delay_ms(uint32_t time);


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
	TxBuf[1]=0x0; //znak 'null'

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

void send_string(char s[])
{
	uint8_t i = 0;

	for (i = 0; i<100; i++)
	{
		TxBuf[i]=s[i];
		if (s[i] == 0) break;
	}


	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

void delay_ms(uint32_t time)
{
	timer_ms = time;
	while (timer_ms) {};
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_StructInit(&gpio);

	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_5;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio);

	// przycisk
	//gpio.GPIO_Pin = GPIO_Pin_13;
	//gpio.GPIO_Mode = GPIO_Mode_IPU;
	//GPIO_Init(GPIOC, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|
					GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);

	nvic.NVIC_IRQChannel = USART2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	USART_StructInit(&uart);

	//uart.USART_BaudRate = 115200;
	uart.USART_BaudRate = 9600;
	USART_Init(USART2, &uart);

	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);

	SysTick_Config(SystemCoreClock / 1000);


	///////////////////////////////////////////////////////////////////

	uint8_t led;
	uint8_t polecenie;

	while (1)
	{
		send_string("test");
		if (odebrano_polecenie == 1)
		{
			if (RxBuf[0] == 'o' && RxBuf[1] == 'n' && RxBuf[2] == ' ')
			{
				polecenie = 1;
				led = (uint8_t) RxBuf[3];
				led = led - 49;
			}
			else if (RxBuf[0] == 'o' && RxBuf[1] == 'f' && RxBuf[2] == 'f' && RxBuf[3] == ' ')
			{
				polecenie = 2;
				led = (uint8_t) RxBuf[4];
				led = led - 49;
			}

			switch (polecenie)
			{
			case 1:
				GPIO_SetBits(GPIOC, 1 << led);
				break;
			case 2:
				GPIO_ResetBits(GPIOC, 1 << led);
				break;
			default:
				delay_ms(10);
				send_string("blad polecenia\r\n");
				break;
			}
			polecenie = 0;
			odebrano_polecenie = 0;
		}

	}
}
