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
	TIM_TimeBaseInitTypeDef tim;
	TIM_OCInitTypeDef  channel;
	ADC_InitTypeDef adc;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);

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

	///////// GPIO for Timer configuration /////////
	gpio.GPIO_Pin = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &gpio);

	///////// GPIO for ADC configuration ///////////
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpio);

	//////////// SPI configuration ///////////////
	SPI_StructInit(&spi);
	spi.SPI_Mode = SPI_Mode_Master;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	SPI_Init(SPI1, &spi);

	SPI_Cmd(SPI1, ENABLE);

	//////////// Timer configuration /////////////
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 3600 - 1;
	tim.TIM_Period = 100 - 1;
	TIM_TimeBaseInit(TIM4, &tim);

	///////// Timer channel configuration ///////////
	TIM_OCStructInit(&channel);
	channel.TIM_OCMode = TIM_OCMode_PWM1;
	channel.TIM_OutputState = TIM_OutputState_Enable;
	channel.TIM_Pulse = 0;
	TIM_OC1Init(TIM4, &channel);
	TIM_Cmd(TIM4, ENABLE);

	//////////////// ADC configuration ////////////
	ADC_StructInit(&adc);
	adc.ADC_ContinuousConvMode = ENABLE;
	adc.ADC_NbrOfChannel = 1;
	adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_Init(ADC1, &adc);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_71Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	//////////// ADC calibration ///////////////
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

	//////////////// SysTick ON /////////////////
	SysTick_Config(SystemCoreClock / 1000);



	////////////////////////////////////////////////////////////////////////////////////////

	lcd_setup();
	TIM_SetCompare1(TIM4, 100);

	lcd_draw_bitmap(forbot_logo);
	lcd_copy();

	delay_ms(1000);


	while (1)
	{
		lcd_clear();

		float adc = ADC_GetConversionValue(ADC1);
		float adc_procent = adc * 100 / 4096.0;
		int adc_pasek = (int) (adc * 70 / 4096.0);
		float adcf = adc * 3.3 / 4096.0;


		lcd_draw_line(7, 20, LCD_WIDTH - 7, 20);
		lcd_draw_line(7, 26, LCD_WIDTH - 7, 26);
		lcd_draw_line(7, 20, 7, 26);
		lcd_draw_line(LCD_WIDTH - 7, 20, LCD_WIDTH - 7, 26);

		lcd_draw_line(7, 21, 7+adc_pasek, 21);
		lcd_draw_line(7, 22, 7+adc_pasek, 22);
		lcd_draw_line(7, 23, 7+adc_pasek, 23);
		lcd_draw_line(7, 24, 7+adc_pasek, 24);
		lcd_draw_line(7, 25, 7+adc_pasek, 25);

		lcd_draw_int(1, 33, (int)adc_procent);
		lcd_draw_text(1, 48, "%");

		lcd_draw_text(4, 2, "Napiecie");
		lcd_draw_float(4, 53, adcf, 2);
		lcd_draw_text(4, 77, "V");

		lcd_copy();
		delay_ms(200);
	}

}
