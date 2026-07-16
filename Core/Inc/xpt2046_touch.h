/*
 * xpt2046_touch.h
 *
 *  Created on: 5 апр. 2020 г.
 *      Author: dima
 */

#ifndef XPT2046_TOUCH_H_
#define XPT2046_TOUCH_H_

extern SPI_HandleTypeDef hspi2;
#define TOUCH_SPI_PTR 		 &hspi2

#define TOUCH_CS_SELECT      HAL_GPIO_WritePin(GPIOB, T_CS_Pin, GPIO_PIN_RESET)
#define TOUCH_CS_UNSELECT    HAL_GPIO_WritePin(GPIOB, T_CS_Pin, GPIO_PIN_SET)

uint8_t ILI9341_TouchGetCoordinates(uint16_t *x, uint16_t *y);
void enable_score_editing(void);
void score_editing_handler(void);
void hide_reset_timer_button(void);

#endif
