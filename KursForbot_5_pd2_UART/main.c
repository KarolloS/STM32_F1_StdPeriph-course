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
	uart.USART_BaudRate = 115200;
	USART_Init(USART2, &uart);

	USART_Cmd(USART2, ENABLE);

	SysTick_Config(SystemCoreClock / 1000);

	char tekst[80] = "";
	uint32_t polecenie = 0;
	uint32_t i = 0;
	uint32_t led = 0;


	while (1)
	{
		if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
		{
		    char c = USART_ReceiveData(USART2);
		    if ( c == 13)
		    {
		    	tekst[i]='\r';
		    	tekst[i+1]='\n';
		    	tekst[i+2]='\0';
		    	led = (uint32_t)tekst[i-1];
		    	led = led - 49;
		    	send_string("\r\n");
		    	i=0;

		    	if (tekst[0] == 'o' && tekst[1] == 'n')
		    	{
		    		polecenie = 1;
		    	}
		    	else if (tekst[0] == 'o' && tekst[1] == 'f')
		    	{
		    		polecenie = 2;
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
		    			            send_string("BLAD!!\r\n");
		    			            break;
		    			    }
		    	polecenie = 0;
		    }
		    else
		    {
		    	tekst[i]=c;
		    	send_char(c);
			    i++;
		    }
		}
	}
}
