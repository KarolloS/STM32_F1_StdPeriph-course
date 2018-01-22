#include <stdio.h>
#include <math.h>
#include "stm32f10x.h"

#define MCP_IODIR		0x00
#define MCP_IPOL		0x01
#define MCP_GPINTEN		0x02
#define MCP_DEFVAL		0x03
#define MCP_INTCON		0x04
#define MCP_IOCON		0x05
#define MCP_GPPU		0x06
#define MCP_INTF		0x07
#define MCP_INTCAP		0x08
#define MCP_GPIO		0x09
#define MCP_OLAT		0x0a

void SysTick_Handler(void);
void delay_ms(uint32_t time);
uint8_t spi_sendrecv(uint8_t byte);
void mcp_write_reg(uint8_t addr, uint8_t value);
void USART2_IRQHandler(void);
void send_char(char c);
void send_string(char s[]);

volatile uint32_t timer_ms;
volatile uint32_t timer_ms;
volatile uint8_t odebrano_polecenie, RxBuf[100], TxBuf[100], RxIndex , TxIndex;

//////////////////////////////////////////////////////////////////////

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
	while (timer_ms) {};
}

uint8_t spi_sendrecv(uint8_t byte)
{
	// poczekaj az bufor nadawczy bedzie wolny
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
		;
	SPI_I2S_SendData(SPI1, byte);

	// poczekaj na dane w buforze odbiorczym
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
		;
	return SPI_I2S_ReceiveData(SPI1);
}

void mcp_write_reg(uint8_t addr, uint8_t value)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	spi_sendrecv(0x40);
	spi_sendrecv(addr);
	spi_sendrecv(value);
	GPIO_SetBits(GPIOC, GPIO_Pin_0);
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

int main(void)
{
	GPIO_InitTypeDef gpio;
	SPI_InitTypeDef spi;
	USART_InitTypeDef uart;
	NVIC_InitTypeDef nvic;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	////////// GPIO for UART configuration //////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	////////// GPIO for SPI configuration //////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7; // SCK, MOSI
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_6; // MISO
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_0; // CS
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);
	GPIO_SetBits(GPIOC, GPIO_Pin_0);

	//////////// SPI configuration ///////////////
	SPI_StructInit(&spi);
	spi.SPI_Mode = SPI_Mode_Master;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	SPI_Init(SPI1, &spi);

	SPI_Cmd(SPI1, ENABLE);

	///////// NVIC for UART configuration /////////
	nvic.NVIC_IRQChannel = USART2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0x00;
	nvic.NVIC_IRQChannelSubPriority = 0x00;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	///////////// UART configuration ////////////
	USART_StructInit(&uart);
	uart.USART_BaudRate = 115200;
	USART_Init(USART2, &uart);

	///// UART interrupts ON
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART2, ENABLE);

	//////////// SysTick ON /////////////////
	SysTick_Config(SystemCoreClock / 1000);



	////////////////////////////////////////////////////////////////////////////////////////

	mcp_write_reg(MCP_IODIR, ~0x0F);

/*	while (1)
	{
		// zapal diode
		mcp_write_reg(MCP_OLAT, 0x0F);
		delay_ms(1000);

		// zgas diode
		mcp_write_reg(MCP_OLAT, 0x00);
		delay_ms(1000);
	}*/

	uint8_t led;
	uint8_t polecenie;
	uint8_t r = 0;

	while (1)
	{
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
				r |= ( 1 << led );
				mcp_write_reg(MCP_OLAT, r);
				break;
			case 2:
				r &= ~( 1 << led );
				mcp_write_reg(MCP_OLAT, r);
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
