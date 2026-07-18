/*
 * button.h
 *
 *  Created on: 3 мая 2026 г.
 *      Author: dima
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#define BUTTON_PIN_LEFT left_key_Pin
#define BUTTON_PIN_CENTER centr_key_Pin
#define BUTTON_PIN_RIGHT right_key_Pin
#define BUTTON_PORT GPIOB

uint8_t Button_Read_left(void);
uint8_t Button_Read_center(void);
uint8_t Button_Read_right(void);
void Show_reset_timer_button(void);
void Reset_falstart_state(void);
void NRF_Event_handler(void);
void Touchscreen_handler(void);

#endif /* INC_BUTTON_H_ */
