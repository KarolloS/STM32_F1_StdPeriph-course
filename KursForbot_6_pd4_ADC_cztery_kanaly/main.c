#include <stdio.h>
#include "stm32f10x.h"

void SysTick_Handler(void);
void USART2_IRQHandler(void);
void send_char(char c);
void send_string(char s[]);
void delay_ms(uint32_t time);
void __io_putchar(char c);
void send_int(unsigned int d);
void send_float(float f,int po_przecinku);
uint32_t adc_read(int channel);

volatile uint32_t timer_ms;
volatile uint8_t odebrano_polecenie, RxBuf[100], TxBuf[100], RxIndex , TxIndex;


void SysTick_Handler(void)
{
	if (timer_ms)
	{
		timer_ms--;
	}
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
	TxBuf[1]=0; //znak 'null'

	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(2);
}

void send_string(char s[])
{
	uint8_t i = 0;

	for (i = 0; i<100; i++)
	{
		TxBuf[i]=s[i];
		if (s[i] == 0) break; //znak 'null'
	}


	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
	delay_ms(5);
}

void delay_ms(uint32_t time)
{
	timer_ms = time;
	while (timer_ms) {};
}

void __io_putchar(char c)
{
	//////// for printf function ////////
	if (c=='\n')
		send_char('\r');
	send_char(c);
}

void send_int(unsigned int d)
{
	int base = 10;
	int i = 0;
	int div = 1;
	while (d/div >= base)
		div *= base;

	while (div != 0)
	{
		int num = d/div;
		d = d%div;
		div /= base;
		if (num > 9)
		{
			TxBuf[i] = (num-10) + 'A';
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

uint32_t adc_read(int channel)
{
 ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_13Cycles5);
 ADC_SoftwareStartConvCmd(ADC1, ENABLE);

 while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) != SET);

 return ADC_GetConversionValue(ADC1);
}

int main(void)
{
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;
	ADC_InitTypeDef adc;
	NVIC_InitTypeDef nvic;

	/////////// CLOCKS ON ////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);

	////////// GPIO for UART configuration //////////
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	///////// GPIO for ADC configuration ///////////
	gpio.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_5|GPIO_Pin_6;
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpio);

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

	//////////////// ADC configuration ////////////
	ADC_StructInit(&adc);
	adc.ADC_ContinuousConvMode = DISABLE;
	adc.ADC_NbrOfChannel = 4;
	adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_Init(ADC1, &adc);
	ADC_Cmd(ADC1, ENABLE);

	//////////// ADC calibration ///////////////
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	//////////// SysTick ON /////////////////
	SysTick_Config(SystemCoreClock / 1000);


	while (1)
	{
		uint16_t adc;
		float v;

		adc = adc_read(ADC_Channel_0);
		v = (float)adc * 3.3f / 4096.0f;

		send_string("ADC 0 = ");
		send_int(adc);
		send_string(" (");
		send_float(v,2);
		send_string(" V)\t");

		adc = adc_read(ADC_Channel_1);
		v = (float)adc * 3.3f / 4096.0f;

		send_string("ADC 1 = ");
		send_int(adc);
		send_string(" (");
		send_float(v,2);
		send_string(" V)\t");

		adc = adc_read(ADC_Channel_5);
		v = (float)adc * 3.3f / 4096.0f;

		send_string("ADC 2 = ");
		send_int(adc);
		send_string(" (");
		send_float(v,2);
		send_string(" V)\t");

		adc = adc_read(ADC_Channel_6);
		v = (float)adc * 3.3f / 4096.0f;

		send_string("ADC 3 = ");
		send_int(adc);
		send_string(" (");
		send_float(v,2);
		send_string(" V)\r\n");
	}

}
