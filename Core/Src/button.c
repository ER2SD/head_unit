/*
 * button.c
 *
 *  Created on: 3 мая 2026 г.
 *      Author: dima
 */

#include "stm32l1xx_hal.h"
#include "main.h"
#include "button.h"
#include "nrf24.h"
#include "ILI9341_GFX.h"
#include "xpt2046_touch.h"
#include <stdio.h>
#include <string.h>

extern uint8_t rx_data;
extern uint8_t falstart_enabled;
extern uint16_t pressed_btn_team;
extern uint8_t timer_running;
extern uint8_t flag_press;
extern uint32_t time_press;
extern uint16_t x;
extern uint16_t y;
extern game_screen screen;
extern char buf[64];

extern uint16_t scores[8];							//количество очков команд [0-7]

extern volatile int g_timer_seconds;				//Начальное значение таймера
extern volatile uint8_t reset_timer;		//старт/стоп таймера
extern uint16_t teams;									//номер команды
extern uint16_t teams_fs_state[8];          // Teams false start state
extern uint16_t answer;											//положительный или отрицательный ответ

// macros to check the coordinates in range
#define IS_WITHIN(x, y, x1, y1, x2, y2) ((x) > (x1)) && ((x) < (x2)) && ((y) > (y1)) && ((y) < (y2))

static const char *const team_digits[] =
{ "2", "3", "4", "5", "6", "7", "8" };
static uint32_t last_ui_update = 0;

extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;

//=======Обработчик левой кнопки KEY1==================
uint8_t Button_Read_left(void)
{
	static uint32_t last_time = 0;
	static uint8_t last_state = 0;
	uint8_t current_state;
	current_state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN_LEFT);
	if (current_state != last_state)
	{
		if (HAL_GetTick() - last_time > 20) // 20 мс антидребезг
		{
			last_time = HAL_GetTick();
			last_state = current_state;
			if (current_state != GPIO_PIN_SET)
				return 1; // кнопка нажата
		}
	}
	return 0;
}
//=======Обработчик сентральной кнопки KEY2==================
uint8_t Button_Read_center(void)
{
	static uint32_t last_time = 0;
	static uint8_t last_state = 0;
	uint8_t current_state;
	current_state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN_CENTER);
	if (current_state != last_state)
	{
		if (HAL_GetTick() - last_time > 20) // 20 мс антидребезг
		{
			last_time = HAL_GetTick();
			last_state = current_state;
			if (current_state == GPIO_PIN_SET)
				return 1; // кнопка нажата
		}
	}
	return 0;
}
//=======Обработчик правой кнопки KEY3==================
uint8_t Button_Read_right(void)
{
	static uint32_t last_time = 0;
	static uint8_t last_state = 0;
	uint8_t current_state;
	current_state = HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN_RIGHT);
	if (current_state != last_state)
	{
		if (HAL_GetTick() - last_time > 20) // 20 мс антидребезг
		{
			last_time = HAL_GetTick();
			last_state = current_state;
			if (current_state == GPIO_PIN_SET)
				return 1; // кнопка нажата
		}
	}
	return 0;
}
//=======Обработчик кнопок передатчиков===============
static const char *const team_names[] =
{ "Team 1", "Team 2", "Team 3", "Team 4", "Team 5", "Team 6", "Team 7", "Team 8" };
static const uint16_t team_y_pos[] =
{ 50, 70, 90, 110, 130, 150, 170, 190 };

void Show_reset_timer_button(void)
{
	ILI9341_Draw_Filled_Rectangle_Coord(220, 80, 280, 120, WHITE);
	ILI9341_WriteString(233, 90, "RESET", Font_7x10, BLACK, WHITE);
	ILI9341_WriteString(232, 105, "TIMER", Font_7x10, BLACK, WHITE);
}

void Reset_falstart_state(void)
{
	for (uint8_t i = 0; i < 8; i++)
		teams_fs_state[i] = 0;
}

void NRF_Event_handler(void)
{
	uint8_t status;
	status = NRF24_ReadReg(STATUS) & 0x40;
	if (!status)
	{
		return;
	}
	Blink_LED();

	switch (screen)
	{
	case BRAIN_RING:
		rx_data = NRF24L01_Receive();
		if (pressed_btn_team || rx_data < 0x01 || rx_data > 0x08)
		{
			return;
		}
		uint8_t team_idx = rx_data - 1;
		uint8_t is_false_start = (falstart_enabled && timer_running == 0);

		if (is_false_start)
		{
			ILI9341_WriteString(7, team_y_pos[team_idx], team_names[team_idx], Font_11x18, BLACK, MYFON);
			ILI9341_WriteString(175, team_y_pos[team_idx] + 2, "!", Font_11x18, RED, MYFON);

			teams_fs_state[team_idx] = 1;
			Show_reset_timer_button();
		}
		else if (!teams_fs_state[team_idx])
		{
			pressed_btn_team = rx_data;
			ILI9341_WriteString(7, team_y_pos[team_idx], team_names[team_idx], Font_11x18, YELLOW, MYFON);
			HAL_TIM_Base_Stop_IT(&htim2);
		}
		break;
	case SIMPLE:
		break;
	case ERUDIT:
		break;
	default:
		return;
	}
}

void Touchscreen_handler(void)
{
	if (HAL_GPIO_ReadPin(GPIOB, T_IRQ_Pin) == GPIO_PIN_RESET && flag_press) //если нажат тачскрин
	{
		x = 0;
		y = 0;
		if (ILI9341_TouchGetCoordinates(&x, &y))
		{
			flag_press = 0;
			//>>>>>>>>>>вывод координат в UART для отладки
			snprintf(buf, 64, "X = %d, Y = %d\n", x, y);
			HAL_UART_Transmit(&huart1, (uint8_t*) buf, strlen(buf), 100);
			buf[strlen(buf) - 1] = '\0';
			//ILI9341_WriteString(10, 120, buf, Font_11x18, WHITE, MYFON);//вывод координат на экран для отладки
		}

		switch (screen)
		{
		case MAIN_MENU:
            if (IS_WITHIN(x, y, 20, 60, 100, 120)) // Brain Ring selected
			{
				//>>>>>>>>>>Сбрасываем очки у всех команд
				for (uint8_t i = 0; i < 8; i++)
					scores[i] = 0;

				//>>>>>>>>>>обработка состояния таймера
				g_timer_seconds = 60; 								//Устанавливаем начальное время
				reset_timer = 1;							//Сброс таймера
				timer_running = 0;						//флаг остановки таймера
				HAL_TIM_Base_Stop_IT(&htim2);	//Останавливаем таймер
				screen_Brain_Ring();					//переходим в экран игры "Брэйринг"
				screen = BRAIN_RING;					//устанавливаем переменную для страктуры "switch(screen)"
				break;
			}
            else if (IS_WITHIN(x, y, 120, 60, 200, 120)) // Simple selected
			{
				screen_Simple();
				screen = SIMPLE;
				break;
			}
            else if (IS_WITHIN(x, y, 220, 60, 300, 120)) // Erudit Quartet selected
			{
				screen_Erudit_Quartet();
				screen = ERUDIT;
				break;
			}
            else if (IS_WITHIN(x, y, 20, 150, 300, 210)) // если нажали Settings
			{
				screen_setting();
				//>>>>>>>>>>Блок сохранения количесва команд в игре
				if (teams >= 2 && teams <= 8)
				{
					ILI9341_WriteString(130, 110, team_digits[teams - 2], Font_16x26, WHITE, BLACK);
					HAL_Delay(500);
				}
				//>>>>>>>>>>Настройки фальшстарта
				if (falstart_enabled)
				{
					ILI9341_WriteString(40, 80, "ON", Font_11x18, GREEN, RED);
					ILI9341_WriteString(100, 80, "OFF", Font_11x18, DARKGREY, RED);
				}
				else
				{
					ILI9341_WriteString(40, 80, "ON", Font_11x18, DARKGREY, RED);
					ILI9341_WriteString(100, 80, "OFF", Font_11x18, GREEN, RED);
				}
				screen = SETTINGS;
				break;
			}
		case BRAIN_RING:
			enable_score_editing(); //редактирование результата
			if (falstart_enabled)
			{
				hide_reset_timer_button(); //выключение кнопки "reset timer"
			}
            if (IS_WITHIN(x, y, 300, 0, 320, 20)) //если нажали крестик
			{
				screen_menu();
				screen = MAIN_MENU;
			}
			break;
		case SIMPLE:
            if (IS_WITHIN(x, y, 300, 0, 320, 20)) //если нажали крестик
			{
				screen_menu();
				screen = MAIN_MENU;
			}
			break;
		case ERUDIT:
			if (x > 300 && x < 320 && y > 0 && y < 20) //если нажали крестик
			{
				screen_menu();
				screen = MAIN_MENU;
			}
			break;			
		case SETTINGS: 	//=============="setting"=================
			if (IS_WITHIN(x, y, 300, 0, 320, 20)) // если нажали крестик
			{
				screen_menu();
				screen = MAIN_MENU;
			}
            else if (IS_WITHIN(x, y, 40, 80, 60, 100)) //Фальшстарт ON
			{
				falstart_enabled = 1;
				ILI9341_WriteString(40, 80, "ON", Font_11x18, GREEN, RED);
				ILI9341_WriteString(100, 80, "OFF", Font_11x18, DARKGREY, RED);
			}
            else if (IS_WITHIN(x, y, 100, 80, 130, 100)) //Фальшстарт OFF
			{
				falstart_enabled = 0;
				ILI9341_WriteString(40, 80, "ON", Font_11x18, DARKGREY, RED);
				ILI9341_WriteString(100, 80, "OFF", Font_11x18, GREEN, RED);
			}
            else if (IS_WITHIN(x, y, 30, 142, 68, 175)) //Если добавляем комманды "-"
			{
				teams--;
				if (teams >= 2 && teams <= 8)
				{
					ILI9341_WriteString(130, 110, team_digits[teams - 2], Font_16x26, WHITE, BLACK);
					HAL_Delay(500);
				}
			}
            else if (IS_WITHIN(x, y, 90, 142, 128, 175)) //Если удаляем комманды "+"
			{
				teams++;
				if (teams >= 2 && teams <= 8)
				{
					ILI9341_WriteString(130, 110, team_digits[teams - 2], Font_16x26, WHITE, BLACK);
					HAL_Delay(500);
				}
			}
            else if (IS_WITHIN(x, y, 30, 190, 140, 220)) //обработчик кнопки "save"
			{
				screen_menu();
				screen = MAIN_MENU;
				break;
			}
			//-----------------------------------------------------------------------------------------------------------------------------------

		default:
			//Тут пишем код который не попал ни под один "case"
			return;
		}

	}
	if (!flag_press && (HAL_GetTick() - time_press) > 200) // задержка до следующего нажатия
	{
		flag_press = 1;
	}
}

