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
uint8_t mcp_read_reg(uint8_t addr);

volatile uint32_t timer_ms;

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

uint8_t mcp_read_reg(uint8_t addr)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	spi_sendrecv(0x41);
	spi_sendrecv(addr);
	uint8_t value = spi_sendrecv(0xff);
	GPIO_SetBits(GPIOC, GPIO_Pin_0);
	return value;
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	SPI_InitTypeDef spi;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

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

	//////////// SysTick ON /////////////////
	SysTick_Config(SystemCoreClock / 1000);



	////////////////////////////////////////////////////////////////////////////////////////

	mcp_write_reg(MCP_IODIR, ~0x0F);
	mcp_write_reg(MCP_GPPU, 0x10);
	uint8_t reg = 0;

	while (1)
	{
		if ((mcp_read_reg(MCP_GPIO) & 0x10) == 0)
		{
			mcp_write_reg(MCP_OLAT, reg);
			if (reg < 15)
			{
				reg++;
			}
			delay_ms(1000);
		}
		else
		{
			mcp_write_reg(MCP_OLAT, reg);
			if (reg > 0)
			{
				reg--;
			}
			delay_ms(1000);
		}
	}


}
