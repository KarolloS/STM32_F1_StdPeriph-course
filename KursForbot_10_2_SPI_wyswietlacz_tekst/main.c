#include <stdio.h>
#include <math.h>
#include "stm32f10x.h"
#include "delay.h"
#include "lcd.h"
#include "bitmap.h"

///////////////////////////////////////////////////////////////////////////////


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

	gpio.GPIO_Pin = LCD_DC|LCD_CE|LCD_RST;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &gpio);

	GPIO_SetBits(GPIOC, LCD_CE|LCD_RST);

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

	lcd_setup();

	lcd_draw_bitmap(forbot_logo);
	lcd_copy();

	delay_ms(1000);

//	lcd_clear();
//	lcd_draw_int(1, 10, 1994);
//	lcd_copy();

//	int i;
//	lcd_clear();
//	for (i = 0; i < 6; i++)
//	lcd_draw_text(i, 10, "Forbot !");
//	lcd_copy();

	int angle = 0;

	while (1)
	{
		lcd_clear();
		lcd_draw_line(0, 0, LCD_WIDTH - 1, 0);
		lcd_draw_line(LCD_WIDTH - 1, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
		lcd_draw_line(LCD_WIDTH - 1, LCD_HEIGHT - 1, 0, LCD_HEIGHT - 1);
		lcd_draw_line(0, LCD_HEIGHT - 1, 0, 0);

		angle += 5;
		int x = 41.0 * sin(angle * M_PI / 180.0f);
		int y = 23.0 * cos(angle * M_PI / 180.0f);
		lcd_draw_line(LCD_WIDTH / 2 + x, LCD_HEIGHT / 2 - y, LCD_WIDTH / 2 - x, LCD_HEIGHT / 2 + y);

		lcd_copy();

		delay_ms(60);
	}

}
