#include <stdio.h>
#include "stm32f10x.h"
#include"delay.h"
#include"lsm303d.h"

void USART2_IRQHandler(void);
void __io_putchar(char c);
void send_char(char c);
void send_string(char s[]);
void send_enter(void);
void send_int(unsigned int d);
void send_sint(int d);
void send_float(float f, int po_przecinku);

volatile uint8_t odebrano_polecenie, RxBuf[100], TxBuf[100], RxIndex, TxIndex;

void USART2_IRQHandler(void)
{
	char c;

	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		RxBuf[RxIndex] = USART_ReceiveData(USART2);
		c = (char) RxBuf[RxIndex];
		send_char(c);
		RxIndex++;

		if (RxBuf[RxIndex - 1] == 0x0D) //znak 'enter'
		{
			send_string("\r\n");
			odebrano_polecenie = 1;
			RxIndex = 0;
		}
	}

	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
	{
		USART_SendData(USART2, TxBuf[TxIndex++]);

		if (TxBuf[TxIndex - 1] == 0) //znak 'null'
		{
			USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
			TxIndex = 0;
		}
	}
}

void __io_putchar(char c)
{
	if (c=='\n')
		send_char('\r');
	send_char(c);
}

void send_char(char c)
{
	TxBuf[0] = c;
	TxBuf[1] = 0; //znak 'null'

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(2);
}

void send_string(char s[])
{
	uint8_t i = 0;

	for (i = 0; i < 100; i++)
	{
		TxBuf[i] = s[i];
		if (s[i] == 0)
			break; //znak 'null'
	}

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(5);
}

void send_enter(void)
{
	send_string("\r\n");
}

void send_int(unsigned int d)
{
	int base = 10;
	int i = 0;
	int div = 1;
	while (d / div >= base)
		div *= base;

	while (div != 0)
	{
		int num = d / div;
		d = d % div;
		div /= base;
		if (num > 9)
		{
			TxBuf[i] = (num - 10) + 'A';
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

void send_sint(int d)
{
	if((d & (1 << 31)) == (1 << 31))
	{
		d=~d;
		d=d+1;
		send_char('-');
		send_int(d);
	}
	else
	{
		send_int(d);
	}

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
	I2C_InitTypeDef i2c;
	NVIC_InitTypeDef nvic;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	///////// GPIO for UART configuration /////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	///////// GPIO for I2C configuration //////////
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // SCL, SDA
	gpio.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

	///////////// I2C configuration ////////////
	I2C_StructInit(&i2c);
	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_ClockSpeed = 100000;
	I2C_Init(I2C1, &i2c);
	I2C_Cmd(I2C1, ENABLE);

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

	////////////////////////////////////////////////////////////////////////////////

	send_string("Wyszukiwanie akcelerometru...\r\n");

	uint8_t who_am_i = 0;
	lsm_read(0x0f, &who_am_i, sizeof(who_am_i));

	if (who_am_i == 0x49)
	{
		send_string("Znaleziono akcelerometr LSM303D\r\n");
	}
	else
	{
		send_string("Niepoprawna odpowiedz ukladu LSM303D\r\n");
	}

	lsm_write_reg(LSM303D_CTRL5, 0x80|0x10); // TEMP_EN | M_ODR2 (50Hz)
	delay_ms(100);


	while (1)
	{
		int16_t  temp = lsm_read_value(LSM303D_TEMP_OUT);

		send_string("Temp = ");
		send_sint(temp);
		send_enter();

		delay_ms(500);
	}

}
