/*
 * This file is part of Pako Bots division of Origami 3 Firmware.
 *
 *  Pako Bots division of Origami 3 Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots division of Origami 3 Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots division of Origami 3 Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ROBOT_H_
#define _ROBOT_H_

#include "sdkconfig.h"
#include "stdint.h"
#include <stdio.h>

// GPIO PINS
#define LED_RED         2
#define LED_GRN         4
#define LED_BLU         15

#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_GPIO       (LED_BLU)
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_LS_CH1_GPIO       (LED_GRN)
#define LEDC_LS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_LS_CH2_GPIO       (LED_RED)
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2

#define MOTOR_PWM01     13
#define MOTOR_PWM02     25
#define MOTOR_AIN1      14
#define MOTOR_AIN2      12
#define MOTOR_STBY      33
#define MOTOR_BIN1      27
#define MOTOR_BIN2      26

typedef void (*tx_func)(char *data, size_t length);

void
robot_enable(void);

void
robot_cmd(char * data, size_t length);

void
robot_fwd(void);

void
robot_speed(float duty_cycle_motor1, float duty_cycle_motor2);

void
robot_bck(void);

void
robot_left(void);
void
robot_left_spin(void);

void
robot_right(void);
void
robot_right_spin(void);

void
robot_stop(void);

void
robot_connected(tx_func tx);

void
robot_disconnected(void);

int
robot_name(char * buf);

void
robot_light_blue(uint8_t on);

void
robot_light_red(uint8_t on);

void
robot_light_green(uint8_t on);

void
robot_light_color(uint8_t red, uint8_t blue, uint8_t green);

#endif
