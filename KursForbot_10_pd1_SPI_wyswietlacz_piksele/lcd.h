#ifndef __LCD__
#define __LCD__

#include <stdint.h>
#include "stm32f10x.h"

#define LCD_WIDTH				84
#define LCD_HEIGHT				48

#define LCD_DC			GPIO_Pin_1
#define LCD_CE			GPIO_Pin_2
#define LCD_RST			GPIO_Pin_3

void lcd_setup(void);

void lcd_clear(void);
void lcd_draw_bitmap(const uint8_t* data);
void lcd_draw_pixel(int x, int y);
void lcd_draw_text(int row, int col, const char* text);
void int_2_str(char s[], unsigned int d);
void lcd_draw_int(int row, int col, unsigned int d);
void lcd_draw_line(int x1, int y1, int x2, int y2);

void lcd_copy(void);

#endif // __LCD__
