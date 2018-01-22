#include <string.h>
#include "lcd.h"
#include "font.h"
#include "delay.h"

#define PCD8544_FUNCTION_SET		0x20
#define PCD8544_DISP_CONTROL		0x08
#define PCD8544_DISP_NORMAL			0x0c
#define PCD8544_SET_Y				0x40
#define PCD8544_SET_X				0x80
#define PCD8544_H_TC				0x04
#define PCD8544_H_BIAS				0x10
#define PCD8544_H_VOP				0x80

#define LCD_BUFFER_SIZE			(LCD_WIDTH * LCD_HEIGHT / 8)

uint8_t lcd_buffer[LCD_BUFFER_SIZE];

//////////////////////////////////////////////////////////////////////////

static uint8_t spi_sendrecv(uint8_t byte)
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

static void lcd_cmd(uint8_t cmd)
{
	GPIO_ResetBits(GPIOC, LCD_CE | LCD_DC);
	spi_sendrecv(cmd);
	GPIO_SetBits(GPIOC, LCD_CE);
}

void lcd_setup(void)
{
	GPIO_ResetBits(GPIOC, LCD_RST);
	GPIO_SetBits(GPIOC, LCD_RST);

	lcd_cmd(PCD8544_FUNCTION_SET | 1);
	lcd_cmd(PCD8544_H_BIAS | 4);
	lcd_cmd(PCD8544_H_VOP | 0x3f);
	lcd_cmd(PCD8544_FUNCTION_SET);
	lcd_cmd(PCD8544_DISP_NORMAL);
}

void lcd_clear(void)
{
	memset(lcd_buffer, 0, LCD_BUFFER_SIZE);
}

void lcd_draw_bitmap(const uint8_t* data)
{
	memcpy(lcd_buffer, data, LCD_BUFFER_SIZE);
	delay_ms(10);
}

inline void lcd_draw_pixel(int x, int y)
{
	lcd_buffer[x + (y >> 3) * LCD_WIDTH] |= 1 << (y & 7);
}

void lcd_draw_text(int row, int col, const char* text)
{
	int i;
	uint8_t* pbuf = &lcd_buffer[row * 84 + col];
	while ((*text) && (pbuf < &lcd_buffer[LCD_BUFFER_SIZE - 6]))
	{
		int ch = *text++;
		const uint8_t* font = &font_ASCII[ch - ' '][0];
		for (i = 0; i < 5; i++)
		{
			*pbuf++ = *font++;
		}
		*pbuf++ = 0;
	}
	delay_ms(2);
}

void int_2_str(char s[], unsigned int d)
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
			s[i] = (num - 10) + 'A';
			i++;
		}
		else
		{
			s[i] = num + '0';
			i++;
		}
	}
	s[i] = 0;
}

void lcd_draw_int(int row, int col, unsigned int d)
{
	char s[60];
	int_2_str(s, d);
	lcd_draw_text(row, col, s);
}

void lcd_draw_line(int x1, int y1, int x2, int y2)
{
	int dx, dy, sx, sy;
	if (x2 >= x1)
	{
		dx = x2 - x1;
		sx = 1;
	}
	else
	{
		dx = x1 - x2;
		sx = -1;
	}
	if (y2 >= y1)
	{
		dy = y1 - y2;
		sy = 1;
	}
	else
	{
		dy = y2 - y1;
		sy = -1;
	}

	int dx2 = dx << 1;
	int dy2 = dy << 1;
	int err = dx2 + dy2;
	while (1)
	{
		lcd_draw_pixel(x1, y1);
		if (err >= dy)
		{
			if (x1 == x2)
				break;
			err += dy2;
			x1 += sx;
		}
		if (err <= dx)
		{
			if (y1 == y2)
				break;
			err += dx2;
			y1 += sy;
		}
	}
}

void lcd_copy(void)
{
	int i;
	GPIO_SetBits(GPIOC, LCD_DC);
	GPIO_ResetBits(GPIOC, LCD_CE);
	for (i = 0; i < LCD_BUFFER_SIZE; i++)
		spi_sendrecv(lcd_buffer[i]);
	GPIO_SetBits(GPIOC, LCD_CE);
}
