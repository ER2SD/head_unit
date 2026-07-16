/*
 * screen.c
 *
 *  Created on: 6 апр. 2026 г.
 *      Author: dima
 */
#include "main.h"
#include "screen.h"
#include "ILI9341_GFX.h"
#include "xpt2046_touch.h"

#include "stdio.h"
#include "fonts.h"

extern uint16_t screen;
extern uint16_t teams;
extern uint16_t pressed_btn_team;
extern uint16_t scores[8];
extern uint16_t answer;
char scope_buf[16];

#define TEAM_LABEL_X      7
#define TEAM_SCORE_X      135
#define TEAM_START_Y      50
#define TEAM_ROW_STEP     20
#define MAX_TEAMS         8

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

static inline void draw_team_name(uint8_t idx, uint16_t color)
{
	snprintf(scope_buf, sizeof(scope_buf), "Team %u", idx + 1);
	ILI9341_WriteString(TEAM_LABEL_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, color, MYFON);
}

static inline void draw_team_score(uint8_t idx, uint16_t color)
{
	uint16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, color, MYFON);
}

static inline void draw_team_zero(uint16_t color)
{
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y, "0", Font_11x18, color, MYFON);
}

//============= координатная сетка =====================
void show_coordinate_grid()
{
	for (uint16_t z = 20; z < 240; z += 20)
		ILI9341_Draw_Horizontal_Line(0, z, 320, WHITE);
	for (uint16_t h = 20; h < 320; h += 20)
		ILI9341_Draw_Vertical_Line(h, 0, 240, WHITE);
}

//============= Турнирная таблица =====================
void tournament_table()
{
	for (uint16_t z = 50; z < 220; z += 20)
		ILI9341_Draw_Horizontal_Line(120, z, 80, RED);
	for (uint16_t h = 120; h < 200; h += 40)
		ILI9341_Draw_Vertical_Line(h, 50, 160, RED);
}

//==============установка цвета названия комманд для следующего вопроса===========
void reset_color(void)
{
	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t color = (i < teams) ? WHITE : BLACK;
		draw_team_name(i, color);
	}
}

//============= экран выбора игры =====================
void screen_menu()
{
	screen = 0;
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(128, 12, "CMJI", Font_16x26, NAVY, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 60, 100, 120, BLUE); // old 20, 60, 150, 120
	ILI9341_Draw_Filled_Rectangle_Coord(120, 60, 200, 120, BLUE);
	ILI9341_Draw_Filled_Rectangle_Coord(220, 60, 300, 120, BLUE); // old 170, 60, 300, 120
	ILI9341_Draw_Filled_Rectangle_Coord(20, 150, 300, 210, BLUE);
	ILI9341_WriteString(32, 82, "Brain", Font_11x18, ORANGE, BLUE);
	ILI9341_WriteString(128, 82, "Simple", Font_11x18, ORANGE, BLUE);
	ILI9341_WriteString(228, 82, "Erudit", Font_11x18, ORANGE, BLUE);
	ILI9341_WriteString(100, 167, "Settings", Font_16x26, ORANGE, BLUE);
}

//================== экран Brain Ring ==================
void screen_Brain_Ring(void)
{
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(5, 5, 315, 235, MYFON);
	ILI9341_WriteString(20, 10, "Brain Ring", Font_11x18, OLIVE, MYFON);
	ILI9341_WriteString(230, 10, "Timer", Font_7x10, BLUE, MYFON);
	ILI9341_WriteString(122, 36, "Score", Font_7x10, BLUE, MYFON);
	ILI9341_WriteString(175, 38, "FS", Font_7x10, RED, MYFON);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 220, 60, 240, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(130, 220, 190, 240, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(260, 220, 300, 240, WHITE);
	ILI9341_WriteString(30, 225, "yes", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(142, 225, "start", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(272, 225, "no", Font_7x10, BLUE, WHITE);
	tournament_table();

	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t color = (i < teams) ? WHITE : BLACK;
		draw_team_name(i, color);
		draw_team_zero(color);
	}

	//===========Блок кнопки "закрыть"=================================
	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);
}

//================== Simple ====================
void screen_Simple(void)
{
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(50, 12, "Simple", Font_16x26, BLACK, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);
}

//================== эрудит квартет ====================
void screen_Erudit_Quartet(void)
{
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(50, 12, "Erudit Quartet", Font_16x26, BLACK, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);
}

//============= экран настроек =====================
void screen_setting(void)
{
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(50, 15, "Brain &", Font_11x18, BLACK, RED);
	ILI9341_WriteString(50, 35, "Simple", Font_11x18, BLACK, RED);
	ILI9341_WriteString(200, 15, "Erudit", Font_11x18, BLACK, RED);
	ILI9341_WriteString(200, 35, "Quartet", Font_11x18, BLACK, RED);
	ILI9341_Draw_Vertical_Line(160, 0, 240, YELLOW);
	ILI9341_Draw_Horizontal_Line(10, 55, 300, WHITE);
	//===========Блок кнопки "закрыть"=================================
	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);
	//===========Блок настройки фальшстарт=================================
	ILI9341_WriteString(15, 60, "False start", Font_11x18, BLUE, RED);
	//===========Блок настроек колличества команд=================================
	ILI9341_Draw_Horizontal_Line(10, 100, 150, WHITE);
	ILI9341_WriteString(15, 103, "Number of", Font_11x18, BLUE, RED);
	ILI9341_WriteString(15, 118, "teams", Font_11x18, BLUE, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(156, 104, 122, 138, BLACK);
	ILI9341_Draw_Filled_Rectangle_Coord(30, 142, 68, 175, BLACK);
	ILI9341_Draw_Filled_Rectangle_Coord(90, 142, 128, 175, BLACK);
	ILI9341_WriteString(100, 145, "+", Font_16x26, WHITE, BLACK);
	ILI9341_WriteString(40, 145, "-", Font_16x26, WHITE, BLACK);
	//===========Блок кнопки "save"=================================
	ILI9341_Draw_Horizontal_Line(10, 180, 150, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(30, 190, 140, 220, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(32, 192, 138, 219, BLUE);
	ILI9341_WriteString(65, 197, "SAVE", Font_11x18, BLACK, BLUE);
}

//>>>>>>>>>>функция окрашивает в разные цвета команду которая нажала кнопку
void button_event_handler(void)
{
	if (pressed_btn_team < 1 || pressed_btn_team > MAX_TEAMS)
		return;
	uint8_t idx = pressed_btn_team - 1;

	uint16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, WHITE, MYFON);

	draw_team_name(idx, (answer == 0) ? BLACK : WHITE);

	if (answer == 1)
	{
		scores[idx]++;
	}
}

//>>>>>>>>>>функция окрашивает все надписи комманд в цвет по умолчанию
void base_setting_color(void)
{
	if (pressed_btn_team < 1 || pressed_btn_team > MAX_TEAMS)
		return;
	uint8_t idx = pressed_btn_team - 1;
	draw_team_name(idx, WHITE);
}

//>>>>>>>>>>фунция сброса фальшстарта
void reset_falsstart(void)
{
	for (uint8_t i = 0; i < MAX_TEAMS; i++)
		ILI9341_WriteString(175, TEAM_START_Y + 2 + i * TEAM_ROW_STEP, " ", Font_11x18, RED, MYFON);
}

void render_scores(uint16_t active_teams)
{
	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t color = (i < active_teams) ? WHITE : BLACK;
		draw_team_score(i, color);
	}
}

void sub_window_scope(void)
{
}
