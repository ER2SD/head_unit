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

extern game_screen screen;
extern uint16_t teams;
extern uint16_t pressed_btn_team;
extern int16_t scores[8];
extern uint16_t answer;
extern uint8_t question_id;
char scope_buf[16];

#define TEAM_LABEL_X      7
#define TEAM_SCORE_X      135
#define TEAM_START_Y      51
#define TEAM_ROW_STEP     20
#define MAX_TEAMS         8

static inline void int16_to_str(int16_t val, char *buf, uint8_t maxlen)
{
    if (maxlen == 0)
        return;

    memset(buf, 0, maxlen);

    int32_t v = (int32_t)val;   /* widen so that -INT16_MIN (-32768) is safe */
    char digits[6];
    uint8_t n = 0;
    uint8_t i = 0;

    if (v < 0) {
        v = -v;
        if (i + 1 < maxlen)
            buf[i++] = '-';
    }

    do {
        if (n < 5)
            digits[n++] = '0' + (v % 10);
        v /= 10;
    } while (v > 0);

    while (n > 0 && i < maxlen - 1)
        buf[i++] = digits[--n];
    buf[i] = '\0';
}


static inline void draw_team_name(uint8_t idx, uint16_t color)
{
	snprintf(scope_buf, sizeof(scope_buf), "Team %u", idx + 1);
	ILI9341_WriteString(TEAM_LABEL_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, color, MYFON);
}

static inline void draw_team_score(uint8_t idx, uint16_t color)
{
	int16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, color, MYFON);
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
void tournament_table() {
	for (uint16_t z = 50; z < 220; z += 20)
		ILI9341_Draw_Horizontal_Line(120, z, 80, RED);
	for (uint16_t h = 120; h < 201; h += 40)
		ILI9341_Draw_Vertical_Line(h, 50, 160, RED);
}
// simplified table
void tournament_table_simple() {
	for (uint16_t z = 50; z < 220; z += 20)
			ILI9341_Draw_Horizontal_Line(120, z, 80, RED);
	for (uint16_t h = 120; h < 201; h += 80)
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
	screen = MAIN_MENU;
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(128, 12, "CMJI", Font_16x26, NAVY, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 60, 100, 120, BLUE);
	ILI9341_Draw_Filled_Rectangle_Coord(120, 60, 200, 120, BLUE);
	ILI9341_Draw_Filled_Rectangle_Coord(220, 60, 300, 120, BLUE);
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
	ILI9341_WriteString(30, 225, "Yes", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(142, 225, "Start", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(272, 225, "No", Font_7x10, BLUE, WHITE);
	tournament_table();

	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t color = (i < teams) ? WHITE : BLACK;
		draw_team_name(i, color);
		draw_team_score(i, color);
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

	ILI9341_Draw_Filled_Rectangle_Coord(5, 5, 315, 235, MYFON);
	ILI9341_WriteString(20, 10, "Simple", Font_11x18, OLIVE, MYFON);
	ILI9341_WriteString(122, 36, "Score", Font_7x10, BLUE, MYFON);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 220, 60, 240, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(260, 220, 300, 240, WHITE);
	ILI9341_WriteString(30, 225, "Yes", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(272, 225, "No", Font_7x10, BLUE, WHITE);
	tournament_table();

	for (uint8_t i = 0; i < MAX_TEAMS; i++)
	{
		uint16_t color = (i < teams) ? WHITE : BLACK;
		draw_team_name(i, color);
		draw_team_score(i, color);
	}

	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);
}

//================== эрудит квартет ====================
void screen_Erudit_Quartet(void) {
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(5, 5, 315, 235, MYFON);
	ILI9341_WriteString(20, 10, "Erudit Quartet", Font_11x18, OLIVE, MYFON);
	ILI9341_WriteString(230, 10, "Timer", Font_7x10, BLUE, MYFON);
	ILI9341_WriteString(122, 36, "Score", Font_7x10, BLUE, MYFON);
	ILI9341_Draw_Filled_Rectangle_Coord(20, 220, 60, 240, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(130, 220, 190, 240, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(260, 220, 300, 240, WHITE);
	ILI9341_WriteString(30, 225, "Yes", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(142, 225, "Start", Font_7x10, BLUE, WHITE);
	ILI9341_WriteString(272, 225, "No", Font_7x10, BLUE, WHITE);
	tournament_table_simple();

	for (uint8_t i = 0; i < MAX_TEAMS; i++) {
		uint16_t color = (i < teams) ? WHITE : BLACK;
		draw_team_name(i, color);
		draw_team_score(i, color);
	}

	//===========Блок кнопки "закрыть"=================================
	ILI9341_Draw_Filled_Rectangle_Coord(299, 1, 319, 21, BLACK);
	ILI9341_Random_line(302, 4, 315, 16, WHITE);
	ILI9341_Random_line(302, 16, 315, 4, WHITE);

	// the part for the current question value
	ILI9341_WriteString(230, 60, "Points", Font_7x10, BLUE, MYFON);

	// the part for the question ID
	ILI9341_WriteString(230, 105, "Question", Font_7x10, BLUE, MYFON);
}

//============= экран настроек =====================
void screen_setting(void)
{
	ILI9341_Fill_Screen(YELLOW);
	ILI9341_Draw_Filled_Rectangle_Coord(10, 10, 310, 230, RED);
	ILI9341_WriteString(30, 15, "Brain Ring", Font_11x18, BLACK, RED);
	//ILI9341_WriteString(50, 35, "Simple", Font_11x18, BLACK, RED);
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
	ILI9341_Draw_Horizontal_Line(10, 100, 300, WHITE);
	ILI9341_WriteString(15, 103, "Number of", Font_11x18, BLUE, RED);
	ILI9341_WriteString(15, 118, "teams", Font_11x18, BLUE, RED);
	ILI9341_Draw_Filled_Rectangle_Coord(156, 104, 122, 138, BLACK);
	ILI9341_Draw_Filled_Rectangle_Coord(30, 142, 68, 175, BLACK);
	ILI9341_Draw_Filled_Rectangle_Coord(90, 142, 128, 175, BLACK);
	ILI9341_WriteString(100, 145, "+", Font_16x26, WHITE, BLACK);
	ILI9341_WriteString(40, 145, "-", Font_16x26, WHITE, BLACK);
	//===========Блок кнопки "save"=================================
	//ILI9341_Draw_Horizontal_Line(10, 180, 150, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(30, 190, 140, 220, WHITE);
	ILI9341_Draw_Filled_Rectangle_Coord(32, 192, 138, 219, BLUE);
	ILI9341_WriteString(65, 197, "SAVE", Font_11x18, BLACK, BLUE);

	// the lengh of the timer in the game
	ILI9341_WriteString(165, 60, "Timer timeout", Font_11x18, BLUE, RED);
	ILI9341_Draw_Rectangle(245, 104, 60, 34, BLACK);
	ILI9341_WriteString(165, 104, "seconds", Font_11x18, BLUE, RED);

	ILI9341_Draw_Rectangle(180, 142, 36, 34, BLACK);
	ILI9341_Draw_Rectangle(240, 142, 36, 34, BLACK);
	ILI9341_WriteString(190, 145, "-", Font_16x26, WHITE, BLACK);
	ILI9341_WriteString(250, 145, "+", Font_16x26, WHITE, BLACK);
}

//>>>>>>>>>>функция окрашивает в разные цвета команду которая нажала кнопку
void button_event_handler(void)
{
	if (pressed_btn_team < 1 || pressed_btn_team > MAX_TEAMS)
		return;

	uint8_t idx = pressed_btn_team - 1;

	if (answer == 1) {
		switch(screen) {
			case BRAIN_RING: {
				scores[idx]++;
				break;
			}

			case ERUDIT: {
				scores[idx] += question_id * 10;
				question_id ++;
				if(question_id > 5) {
					question_id = 1;
				}
				break;
			}

			default:
				break;
		}
	} else {
		if(answer == 0) {
			switch(screen) {
				case ERUDIT: {
					scores[idx] -= question_id * 10;
					if(question_id > 5) {
						question_id = 1;
					}
					break;
				}

				default:
					break;
			}
		}
	}

	int16_to_str(scores[idx], scope_buf, sizeof(scope_buf));
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y + idx * TEAM_ROW_STEP, "   ", Font_11x18, WHITE, MYFON);
	ILI9341_WriteString(TEAM_SCORE_X, TEAM_START_Y + idx * TEAM_ROW_STEP, scope_buf, Font_11x18, WHITE, MYFON);
	draw_team_name(idx, (answer == 0) ? BLACK : WHITE);
}

//>>>>>>>>>>функция окрашивает все надписи комманд в цвет по умолчанию
void base_setting_color(void) {
	if (pressed_btn_team < 1 || pressed_btn_team > MAX_TEAMS)
		return;
	uint8_t idx = pressed_btn_team - 1;
	draw_team_name(idx, WHITE);
}

//>>>>>>>>>>фунция сброса фальшстарта
void Reset_falstart_screen(void) {
	for (uint8_t i = 0; i < MAX_TEAMS; i++)
		ILI9341_WriteString(175, TEAM_START_Y + i * TEAM_ROW_STEP, " ", Font_11x18, RED, MYFON);
}

void render_scores(uint16_t active_teams) {
	for (uint8_t i = 0; i < MAX_TEAMS; i++) {
		uint16_t color = (i < active_teams) ? WHITE : BLACK;
		draw_team_score(i, color);
	}
}

void sub_window_scope(void)
{
}
