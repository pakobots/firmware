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
