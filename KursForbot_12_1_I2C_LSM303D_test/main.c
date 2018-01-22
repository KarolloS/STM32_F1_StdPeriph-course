#include <stdio.h>
#include "stm32f10x.h"

#define LSM303D_ADDR			0x3a

void SysTick_Handler(void);
void delay_ms(uint32_t time);
void USART2_IRQHandler(void);
void send_char(char c);
void send_string(char s[]);
void send_int(unsigned int d);
void send_float(float f, int po_przecinku);
void lsm_set_reg(uint8_t reg);
void lsm_write(uint8_t reg, const void* data, int size);
void lsm_read(uint8_t reg, void* data, int size);

volatile uint32_t timer_ms;
volatile uint8_t odebrano_polecenie, RxBuf[100], TxBuf[100], RxIndex, TxIndex;

void SysTick_Handler(void)
{
	if (timer_ms)
	{
		timer_ms--;
	}
}

void delay_ms(uint32_t time)
{
	timer_ms = time;
	while (timer_ms)
	{
	};
}

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

void lsm_set_reg(uint8_t reg)
{
	I2C_GenerateSTART(I2C1, ENABLE);
	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS);

	I2C_Send7bitAddress(I2C1, LSM303D_ADDR, I2C_Direction_Transmitter);
	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS);

	I2C_SendData(I2C1, 0x80 | reg);
	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING) != SUCCESS);
}

void lsm_write(uint8_t reg, const void* data, int size)
{
	int i;
	const uint8_t* buffer = (uint8_t*) data;

	lsm_set_reg(reg);
	for (i = 0; i < size; i++)
	{
		I2C_SendData(I2C1, buffer[i]);
		while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING) != SUCCESS);
	}

	I2C_GenerateSTOP(I2C1, ENABLE);
}

void lsm_read(uint8_t reg, void* data, int size)
{
	int i;
	uint8_t* buffer = (uint8_t*) data;

	lsm_set_reg(reg);

	I2C_GenerateSTART(I2C1, ENABLE);
	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS);

	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C_Send7bitAddress(I2C1, LSM303D_ADDR, I2C_Direction_Receiver);
	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != SUCCESS);

	for (i = 0; i < size - 1; i++)
	{
		while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS);
		buffer[i] = I2C_ReceiveData(I2C1);
	}

	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);

	while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) != SUCCESS);
	buffer[i] = I2C_ReceiveData(I2C1);
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


	while (1)
	{

	}

}
