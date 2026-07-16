#ifndef screen_H_
#define screen_H_

#include "ILI9341_GFX.h"

typedef enum {
    MAIN_MENU 	= 0,
    BRAIN_RING 	= 1,
	SIMPLE	 	= 2,
    ERUDIT		= 3,
    SETTINGS 	= 4
} game_screen;

void Touchscreen_handler();
void screen_menu();
void screen_Brain_Ring();
void show_coordinate_grid();
void screen_Simple();
void screen_Erudit_Quartet();
void screen_setting();
void tournament_table();
void button_event_handler();
void base_setting_color();
void reset_color();
void reset_falsstart(void);
void render_scores(uint16_t active_teams);
void sub_window_scope(void);

#endif
