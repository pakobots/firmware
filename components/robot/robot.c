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

// GPIO BOARD SETUPS Defined in make menuconfig
#include "sdkconfig.h"
#include "robot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GPIO WORK
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
// MCPWM
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define MOTOR_MIN_SPEED 15
#define GPIO_MOTOR_PIN_SEL \
    ((1 << MOTOR_AIN1) | (1 << MOTOR_AIN2) | (1 << MOTOR_BIN1) | (1 << MOTOR_BIN2) \
    | (1 << LED_RED) | (1 << LED_GRN) | (1 << LED_BLU))

const static char * TAG = "ROBOT";

void
robot_fwd(){
    ESP_LOGI(TAG, "FORWARD");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void
robot_speed(float left_motor, float right_motor){
    if (left_motor < MOTOR_MIN_SPEED) {
        mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
    } else {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, left_motor);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    }

    if (right_motor < MOTOR_MIN_SPEED) {
        mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
    } else {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, right_motor);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
    }
}

void
robot_bck(){
    ESP_LOGI(TAG, "BACK");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN2, 1);
}

void
robot_left(){
    ESP_LOGI(TAG, "LEFT");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void
robot_left_spin(){
    ESP_LOGI(TAG, "LEFT");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 1);
}

void
robot_right(){
    ESP_LOGI(TAG, "RIGHT");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void
robot_right_spin(){
    ESP_LOGI(TAG, "RIGHT");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN2, 0);
}

void
robot_stop(){
    ESP_LOGI(TAG, "STOP");
    robot_speed(0.0, 0.0);
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void
robot_light_blue(int on){
    gpio_set_level(LED_BLU, on);
}

void
robot_light_red(int on){
    gpio_set_level(LED_RED, on);
}

void
robot_light_green(int on){
    gpio_set_level(LED_GRN, on);
}

void
robot_cmd(char * data){
    char cmd  = data[0];
    char ctrl = data[1];
    char p0;
    int left, right;

    switch (cmd) {
        case 'S':
            sscanf(data, "S%d|%d", &left, &right);
            left  = left < 0 ? 0 : left > 100 ? 100 : left;
            right = right < 0 ? 0 : right > 100 ? 100 : right;
            robot_speed(left, right);
            return;
        case 'M':
            ESP_LOGI(TAG, "MOVE");
            switch (ctrl) {
                case 'F':
                    robot_fwd();
                    break;
                case 'B':
                    robot_bck();
                    break;
                case 'L':
                    robot_left();
                    break;
                case 'R':
                    robot_right();
                    break;
                case 'S':
                    robot_stop();
                    break;
            }
            break;
        case 'L':
            if (strlen(data) > 2) {
                p0 = data[2];
            } else {
                p0 = '0';
            }
            switch (ctrl) {
                case 'R':
                    robot_light_red(p0 == '1' ? 1 : 0);
                    break;
                case 'G':
                    robot_light_green(p0 == '1' ? 1 : 0);
                    break;
                case 'B':
                    robot_light_blue(p0 == '1' ? 1 : 0);
                    break;
            }
            break;
    }
} /* cmd */

void
robot_enable(){
    gpio_config_t io_conf;

    // disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_MOTOR_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_direction(MOTOR_STBY, GPIO_MODE_DEF_OUTPUT);
    gpio_set_level(MOTOR_STBY, 1);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, MOTOR_PWM01);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, MOTOR_PWM02);

    mcpwm_config_t pwm_config;
    pwm_config.frequency    = 1000; // frequency = 500Hz,
    pwm_config.cmpr_a       = 0;    // duty cycle of PWMxA = 0
    pwm_config.cmpr_b       = 0;    // duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode    = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}
