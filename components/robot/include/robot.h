/*
 * This file is part of Pako Bots Firmware.
 *
 *  Pako Bots Firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Pako Bots Firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Pako Bots Firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ROBOT_H_
#define _ROBOT_H_

#include "sdkconfig.h"

void robot_enable(void);
void robot_cmd(char *);
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
robot_light_blue(int);

void
robot_light_red(int);

void
robot_light_green(int);

#endif
