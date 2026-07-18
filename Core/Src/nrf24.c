/****************************
 * nrf24.c
 *  Created on: 18 мая 2026 г.
 *  Author: Dmitrii Safonov
 ****************************/
#include "main.h"
#include "nrf24.h"
#include "stdio.h"
#include "string.h"

#define TX_ADR_WIDTH 3
#define TX_PLOAD_WIDTH 5
#define NRF24_DELAY_US 1
#define NRF24_CE_DELAY_US 150

static const uint8_t tx_addr[3] =
{ 0xb3, 0xb4, 0x01 };
uint8_t RX_BUF[TX_PLOAD_WIDTH] =
{ 0 };

extern SPI_HandleTypeDef hspi3;
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim3;

extern char str1[150];
uint8_t ErrCnt_Fl = 0;

void Delay_us(uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim3, 0);
	while (__HAL_TIM_GET_COUNTER(&htim3) < us)
	{
	}
}

uint8_t NRF24_ReadReg(uint8_t addr)
{
	uint8_t dt, cmd;
	CS_ON;
	HAL_SPI_TransmitReceive(&hspi3, &addr, &dt, 1, 1000);
	if (addr != STATUS)
	{
		cmd = 0xFF;
		HAL_SPI_TransmitReceive(&hspi3, &cmd, &dt, 1, 1000);
	}
	CS_OFF;
	return dt;
}

void NRF24_WriteReg(uint8_t addr, uint8_t dt)
{
	addr |= W_REGISTER;
	CS_ON;
	HAL_SPI_Transmit(&hspi3, &addr, 1, 1000);
	HAL_SPI_Transmit(&hspi3, &dt, 1, 1000);
	CS_OFF;
}

void NRF24_ToggleFeatures(void)
{
	CS_ON;
	HAL_SPI_Transmit(&hspi3, (uint8_t[]
			)
			{ ACTIVATE }, 1, 1000);
	Delay_us(NRF24_DELAY_US);
	HAL_SPI_Transmit(&hspi3, (uint8_t[]
			)
			{ 0x73 }, 1, 1000);
	CS_OFF;
}

void NRF24_Read_Buf(uint8_t addr, uint8_t *pBuf, uint8_t bytes)
{
	CS_ON;
	HAL_SPI_Transmit(&hspi3, &addr, 1, 1000);
	HAL_SPI_Receive(&hspi3, pBuf, bytes, 1000);
	CS_OFF;
}

void NRF24_Write_Buf(uint8_t addr, const uint8_t *pBuf, uint8_t bytes)
{
	addr |= W_REGISTER;
	CS_ON;
	HAL_SPI_Transmit(&hspi3, &addr, 1, 1000);
	Delay_us(NRF24_DELAY_US);
	HAL_SPI_Transmit(&hspi3, pBuf, bytes, 1000);
	CS_OFF;
}

void NRF24_FlushRX(void)
{
	CS_ON;
	HAL_SPI_Transmit(&hspi3, (uint8_t[]
			)
			{ FLUSH_RX }, 1, 1000);
	CS_OFF;
}

void NRF24_FlushTX(void)
{
	CS_ON;
	HAL_SPI_Transmit(&hspi3, (uint8_t[]
			)
			{ FLUSH_TX }, 1, 1000);
	CS_OFF;
}

void NRF24L01_RX_Mode(void)
{
	uint8_t regval = NRF24_ReadReg(CONFIG);
	regval |= (1 << PWR_UP) | (1 << PRIM_RX);
	NRF24_WriteReg(CONFIG, regval);
	CE_SET;
	Delay_us(NRF24_CE_DELAY_US);
	NRF24_FlushRX();
	NRF24_FlushTX();
}

void NRF24L01_TX_Mode(uint8_t *pBuf)
{
	NRF24_Write_Buf(TX_ADDR, tx_addr, TX_ADR_WIDTH);
	CE_RESET;
	NRF24_FlushRX();
	NRF24_FlushTX();
}

void NRF24_Transmit(uint8_t addr, uint8_t *pBuf, uint8_t bytes)
{
	CE_RESET;
	CS_ON;
	HAL_SPI_Transmit(&hspi3, &addr, 1, 1000);
	Delay_us(NRF24_DELAY_US);
	HAL_SPI_Transmit(&hspi3, pBuf, bytes, 1000);
	CS_OFF;
	CE_SET;
}

uint8_t NRF24L01_Send(uint8_t *pBuf)
{
	uint8_t status, regval;
	NRF24L01_TX_Mode(pBuf);
	regval = NRF24_ReadReg(CONFIG);
	regval |= (1 << PWR_UP);
	regval &= ~(1 << PRIM_RX);
	NRF24_WriteReg(CONFIG, regval);
	Delay_us(NRF24_CE_DELAY_US);
	NRF24_Transmit(WR_TX_PLOAD, pBuf, TX_PLOAD_WIDTH);
	CE_SET;
	Delay_us(NRF24_DELAY_US);
	CE_RESET;
	while ((GPIO_PinState) IRQ == GPIO_PIN_SET)
	{
	}
	status = NRF24_ReadReg(STATUS);
	if (status & TX_DS)
	{
		LED_TGL;
		NRF24_WriteReg(STATUS, 0x20);
	}
	else if (status & MAX_RT)
	{
		NRF24_WriteReg(STATUS, 0x10);
		NRF24_FlushTX();
	}
	regval = NRF24_ReadReg(OBSERVE_TX);
	NRF24L01_RX_Mode();
	return regval;
}

uint8_t NRF24L01_Receive(void)
{
	uint8_t status = 0x01;
	while ((GPIO_PinState) IRQ == GPIO_PIN_SET)
	{
	}
	status = NRF24_ReadReg(STATUS);
	if (status & 0x40)
	{
		NRF24_Read_Buf(RD_RX_PLOAD, RX_BUF, TX_PLOAD_WIDTH);
		NRF24_WriteReg(STATUS, 0x40);
		return RX_BUF[0];
	}
	return 0x00;
}

void NRF24_ini(void)
{
	CE_RESET;
	Delay_us(5000);
	NRF24_WriteReg(CONFIG, 0x0a);
	Delay_us(5000);
	NRF24_WriteReg(EN_AA, 0x02);
	NRF24_WriteReg(EN_RXADDR, 0x02);
	NRF24_WriteReg(SETUP_AW, 0x01);
	NRF24_WriteReg(SETUP_RETR, 0x5F);
	NRF24_ToggleFeatures();
	NRF24_WriteReg(FEATURE, 0);
	NRF24_WriteReg(DYNPD, 0);
	NRF24_WriteReg(STATUS, 0x70);
	NRF24_WriteReg(RF_CH, 76);
	NRF24_WriteReg(RF_SETUP, 0x06);
	NRF24_Write_Buf(TX_ADDR, tx_addr, TX_ADR_WIDTH);
	NRF24_Write_Buf(RX_ADDR_P1, tx_addr, TX_ADR_WIDTH);
	NRF24_WriteReg(RX_PW_P1, TX_PLOAD_WIDTH);
	NRF24L01_RX_Mode();
	LED_OFF;
}

void NRF24_PrintReg(char *name, uint8_t reg)
{
	uint8_t value = NRF24_ReadReg(reg);
	sprintf(str1, "%s: 0x%02X\r\n", name, value);
	HAL_UART_Transmit(&huart1, (uint8_t*) str1, strlen(str1), 0x1000);
}

void Blink_LED(void)
{
	LED_ON;
	HAL_Delay(200);
	LED_OFF;
}
