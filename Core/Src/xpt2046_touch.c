/*
 * xpt2046_touch.c
 *
 *  Created on: 5 апр. 2020 г.
 *      Author: dima
 */

#include "main.h"
#include "xpt2046_touch.h"
#include "ILI9341_GFX.h"
#include "nrf24.h"
#include "stdio.h"

#include "button.h"
#include "screen.h"

#define TOUCH_SCALE_X 320
#define TOUCH_SCALE_Y 240

#define TOUCH_MIN_RAW_X 1500
#define TOUCH_MAX_RAW_X 30000
#define TOUCH_MIN_RAW_Y 2500
#define TOUCH_MAX_RAW_Y 30000

#define EDIT_ZONE_X_MIN 140
#define EDIT_ZONE_X_MAX 170
#define EDIT_ZONE_START_Y 50
#define EDIT_ZONE_ROW_STEP 20
#define MAX_TEAMS 8

#define BUTTON_PLUS_X 20
#define BUTTON_PLUS_Y 220
#define BUTTON_PLUS_W 40
#define BUTTON_PLUS_H 20

#define BUTTON_SAVE_X 130
#define BUTTON_SAVE_Y 220
#define BUTTON_SAVE_W 60
#define BUTTON_SAVE_H 20

#define BUTTON_MINUS_X 260
#define BUTTON_MINUS_Y 220
#define BUTTON_MINUS_W 40
#define BUTTON_MINUS_H 20

#define BUTTON_Y 225

extern TIM_HandleTypeDef htim2;

static const uint8_t cmd_read_x = 0x90;
static const uint8_t cmd_read_y = 0xD0;
static const uint8_t zeroes_tx[] =
{ 0x00, 0x00 };

extern uint16_t edit_score;
extern uint16_t scores[8];
extern uint16_t screen;
extern uint16_t x;
extern uint16_t y;
extern uint8_t flag_press;
extern uint16_t teams;
extern char scope_buf[6];

extern uint8_t timer_running;
extern volatile int g_timer_seconds;
extern volatile uint8_t reset_timer;

static inline void uint16_to_str(uint16_t val, char *buf, uint8_t maxlen)
{
	if (val == 0)
	{
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}
	uint8_t i = 0;
	while (val > 0 && i < maxlen - 1)
	{
		buf[i++] = '0' + (val % 10);
		val /= 10;
	}
	buf[i] = '\0';
	for (uint8_t j = 0; j < i / 2; j++)
	{
		char t = buf[j];
		buf[j] = buf[i - 1 - j];
		buf[i - 1 - j] = t;
	}
}

uint8_t ILI9341_TouchGetCoordinates(uint16_t *x, uint16_t *y)
{
	if (HAL_GPIO_ReadPin(GPIOB, T_IRQ_Pin) != GPIO_PIN_RESET)
		return 0;
	uint32_t avg_x = 0;
	uint32_t avg_y = 0;
	HAL_SPI_Transmit(TOUCH_SPI_PTR, (uint8_t*) &cmd_read_y, 1, 1000);
	uint8_t y_raw[2] =
	{ 0, };
	HAL_SPI_TransmitReceive(TOUCH_SPI_PTR, (uint8_t*) zeroes_tx, y_raw, 2, 1000);
	HAL_SPI_Transmit(TOUCH_SPI_PTR, (uint8_t*) &cmd_read_x, 1, 1000);
	uint8_t x_raw[2] =
	{ 0, };
	HAL_SPI_TransmitReceive(TOUCH_SPI_PTR, (uint8_t*) zeroes_tx, x_raw, 2, 1000);
	avg_x = (((uint16_t) x_raw[0]) << 8) | ((uint16_t) x_raw[1]);
	avg_y = (((uint16_t) y_raw[0]) << 8) | ((uint16_t) y_raw[1]);
	if (avg_x < TOUCH_MIN_RAW_X)
		avg_x = TOUCH_MIN_RAW_X;
	if (avg_x > TOUCH_MAX_RAW_X)
		avg_x = TOUCH_MAX_RAW_X;

	if (avg_y < TOUCH_MIN_RAW_Y)
		avg_y = TOUCH_MIN_RAW_Y;
	if (avg_y > TOUCH_MAX_RAW_Y)
		avg_y = TOUCH_MAX_RAW_Y;

	*x = (avg_x - TOUCH_MIN_RAW_X) * TOUCH_SCALE_X / (TOUCH_MAX_RAW_X - TOUCH_MIN_RAW_X);
	*y = (avg_y - TOUCH_MIN_RAW_Y) * TOUCH_SCALE_Y / (TOUCH_MAX_RAW_Y - TOUCH_MIN_RAW_Y);
	return 1;
}

static void draw_edit_buttons(void)
{
	ILI9341_Draw_Filled_Rectangle_Coord(BUTTON_PLUS_X, BUTTON_PLUS_Y,
	BUTTON_PLUS_X + BUTTON_PLUS_W, BUTTON_PLUS_Y + BUTTON_PLUS_H, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(BUTTON_SAVE_X, BUTTON_SAVE_Y,
	BUTTON_SAVE_X + BUTTON_SAVE_W, BUTTON_SAVE_Y + BUTTON_SAVE_H, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(BUTTON_MINUS_X, BUTTON_MINUS_Y,
	BUTTON_MINUS_X + BUTTON_MINUS_W, BUTTON_MINUS_Y + BUTTON_MINUS_H, WHITE);
	ILI9341_WriteString(30, BUTTON_Y, "+", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(142, BUTTON_Y, "save", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(272, BUTTON_Y, "-", Font_7x10, BLUE, WHITE);
}

void enable_score_editing(void)
{
	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t y_top = EDIT_ZONE_START_Y + i * EDIT_ZONE_ROW_STEP;
		uint16_t y_bot = y_top + EDIT_ZONE_ROW_STEP;
		if (x > EDIT_ZONE_X_MIN && x < EDIT_ZONE_X_MAX && y > y_top && y < y_bot)
		{
			edit_score = i + 1;
			uint16_to_str(scores[i], scope_buf, sizeof(scope_buf));
			ILI9341_WriteString(135, y_top, scope_buf, Font_11x18, ORANGE, MYFON);
			draw_edit_buttons();
			return;
		}
	}
}

void score_editing_handler(void)
{
	if (edit_score < 1 || edit_score > MAX_TEAMS)
		return;
	uint8_t idx = edit_score - 1;
	uint16_t y_pos = EDIT_ZONE_START_Y + idx * EDIT_ZONE_ROW_STEP;

	if (Button_Read_left())
	{
		scores[idx]++;
		uint16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
		ILI9341_WriteString(135, y_pos, scope_buf, Font_11x18, ORANGE, MYFON);
	}

	if (Button_Read_right())
	{
		if (scores[idx] > 0)
			scores[idx]--;
		uint16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
		ILI9341_WriteString(135, y_pos, scope_buf, Font_11x18, ORANGE, MYFON);
	}

	if (Button_Read_center() && edit_score != 0)
	{
		draw_edit_buttons();
		ILI9341_WriteString(30, BUTTON_Y, "yes", Font_7x10, BLUE, WHITE);
		ILI9341_WriteString(142, BUTTON_Y, "start", Font_7x10, BLUE, WHITE);
		ILI9341_WriteString(272, BUTTON_Y, "no", Font_7x10, BLUE, WHITE);
		edit_score = 0;
		render_scores(teams);
	}
}

void hide_reset_timer_button(void)
{
	if (x > 220 && x < 280 && y > 80 && y < 120)
	{
		ILI9341_Draw_Filled_Rectangle_Coord(220, 80, 280, 120, MYFON);
		g_timer_seconds = 60;
		reset_timer = 1;
		timer_running = 0;
		Reset_falstart_state();
		Reset_falstart_screen();
		HAL_TIM_Base_Stop_IT(&htim2);
	}
}
