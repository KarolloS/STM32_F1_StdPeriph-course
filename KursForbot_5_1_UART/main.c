#include "stm32f10x.h"

extern volatile uint32_t timer_ms;

void delay_ms(int time)
{
	timer_ms = time;
	while (timer_ms) {};
}

void send_char(char c)
{
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, c);
}

void send_string(const char* s)
{
	while (*s)
		send_char(*s++);
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;

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

	gpio.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|
					GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);

	USART_StructInit(&uart);
	//uart.USART_BaudRate = 115200;
	uart.USART_BaudRate = 9600;
	USART_Init(USART2, &uart);

	USART_Cmd(USART2, ENABLE);

	SysTick_Config(SystemCoreClock / 1000);

	while (1)
	{
		if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE)) {
		    char c = USART_ReceiveData(USART2);
		    switch (c)
		    {
		        case '1':
		            send_string("Zapal diode 1\r\n");
		            GPIO_ResetBits(GPIOC, 1023);
		            GPIO_SetBits(GPIOC, 1);
		            break;
		        case '2':
		            send_string("zapal diode 2\r\n");
		            GPIO_ResetBits(GPIOC, 1023);
		            GPIO_SetBits(GPIOC, 2);
		            break;
		        default:
		            send_string("nie zapalaj diody\r\n");
		            break;
		    }
		}

	}
}
