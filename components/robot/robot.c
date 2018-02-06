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
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GPIO WORK
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
// MCPWM
#include "driver/mcpwm.h"
#include "driver/ledc.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define COLOR_FUNC_FADE 0
#define COLOR_FUNC_SOLID 1

#define MOTOR_MIN_SPEED 15
#define GPIO_MOTOR_PIN_SEL \
    ((1 << MOTOR_AIN1) | (1 << MOTOR_AIN2) | (1 << MOTOR_BIN1) | (1 << MOTOR_BIN2) | (1 << LED_RED) | (1 << LED_GRN) | (1 << LED_BLU))

typedef struct
{
    uint8_t channel;
    uint8_t gpio;
    uint16_t duty;
} ledc_info_t;

typedef void (*color_func)(void);

static int MODE_FW_UPDATE = 0;
static tx_func TX_FUNC;
static uint16_t COLOR_MOD[3] = {0, 0, 0};
static QueueHandle_t COLORS;
static QueueHandle_t COLORS_FUNC;
static color_func COLOR_EFFECTS[2];
static ledc_info_t ledc_ch[3] = {
    {.channel = LEDC_LS_CH0_CHANNEL,
     .gpio = LEDC_LS_CH0_GPIO,
     .duty = 0},
    {.channel = LEDC_LS_CH1_CHANNEL,
     .gpio = LEDC_LS_CH1_GPIO,
     .duty = 0},
    {.channel = LEDC_LS_CH2_CHANNEL,
     .gpio = LEDC_LS_CH2_GPIO,
     .duty = 5000}};

static uint16_t ledc_fade_timeout = 1000;
static uint16_t ledc_fade_delay_top = 1500;
static uint16_t ledc_fade_delay_bottom = 4500;

const static char *TAG = "ROBOT";

void robot_fwd()
{
    ESP_LOGI(TAG, "FORWARD");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void robot_speed(float left_motor, float right_motor)
{
    if (left_motor < MOTOR_MIN_SPEED)
    {
        mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
    }
    else
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, left_motor);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    }

    if (right_motor < MOTOR_MIN_SPEED)
    {
        mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
    }
    else
    {
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, right_motor);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
    }
}

void robot_bck()
{
    ESP_LOGI(TAG, "BACK");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN2, 1);
}

void robot_left()
{
    ESP_LOGI(TAG, "LEFT");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void robot_left_spin()
{
    ESP_LOGI(TAG, "LEFT");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 1);
}

void robot_right()
{
    ESP_LOGI(TAG, "RIGHT");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void robot_right_spin()
{
    ESP_LOGI(TAG, "RIGHT");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN2, 0);
}

void robot_stop()
{
    ESP_LOGI(TAG, "STOP");
    robot_speed(0.0, 0.0);
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}

void robot_connected(tx_func tx)
{
    if (tx)
    {
        TX_FUNC = tx;
    }

    robot_light_color(0, 255, 0);
    static char func = COLOR_FUNC_SOLID;
    xQueueOverwrite(COLORS_FUNC, &func);
}

void robot_disconnected()
{
    TX_FUNC = NULL;
    robot_light_color(0, 0, 255);
    static char func = COLOR_FUNC_FADE;
    xQueueOverwrite(COLORS_FUNC, &func);
}

void robot_light_blue(uint8_t on)
{
    COLOR_MOD[0] = 12;
    COLOR_MOD[1] = 12;
    COLOR_MOD[2] = on ? 5000 : 0;
    xQueueOverwrite(COLORS, &COLOR_MOD);
}

void robot_light_red(uint8_t on)
{
    COLOR_MOD[0] = on ? 5000 : 0;
    COLOR_MOD[1] = 12;
    COLOR_MOD[2] = 12;
    xQueueOverwrite(COLORS, &COLOR_MOD);
}

void robot_light_green(uint8_t on)
{
    COLOR_MOD[0] = 12;
    COLOR_MOD[1] = on ? 5000 : 0;
    COLOR_MOD[2] = 12;
    xQueueOverwrite(COLORS, &COLOR_MOD);
}

void robot_light_color(uint8_t red, uint8_t green, uint8_t blue)
{
    COLOR_MOD[0] = red * 19;
    COLOR_MOD[1] = green * 17;
    COLOR_MOD[2] = blue * 18;
    xQueueOverwrite(COLORS, &COLOR_MOD);
}

int robot_name(char *buf)
{
    size_t len;
    storage_len(PROPERTIES_STORAGE_KEY, "name", &len);
    if (len > 0)
    {
        storage_get(PROPERTIES_STORAGE_KEY, "name", buf, &len);
    }

    return len;
}

void robot_cmd(char *data, size_t length)
{
    if (MODE_FW_UPDATE)
    {
        return;
    }

    char cmd = data[0];
    char ctrl = data[1];
    char p0;
    uint16_t len = length < 120 ? length : 120;
    char str1[len], str2[len];
    int num1, num2, num3;

    switch (cmd)
    {
    case 'U':
        MODE_FW_UPDATE = 1;
        break;
    case 'N':
        if (len < 3)
        {
            break;
        }
        memcpy(str1, &data[1], len - 1);
        str1[len - 1] = '\0';
        printf("Setting Robot Name:%s\n", str1);
        storage_set(PROPERTIES_STORAGE_KEY, "name", str1);
        break;
    case 'W':
        printf("Setting Robot WIFI Settings\n");
        num1 = strchr(data, '|') - data;
        if (num1 < 2)
        {
            break;
        }
        memcpy(str1, &data[1], num1 - 1);
        memcpy(str2, &data[num1 + 1], len - num1);
        str1[num1 - 1] = '\0';
        str2[len - num1 - 1] = '\0';
        storage_set(PROPERTIES_STORAGE_KEY, "ssid", str1);
        storage_set(PROPERTIES_STORAGE_KEY, "pass", str2);
        break;
    case 'S':
        sscanf(data, "S%d|%d", &num1, &num2);
        num1 = num1 < 0 ? 0 : num1 > 100 ? 100 : num1;
        num2 = num2 < 0 ? 0 : num2 > 100 ? 100 : num2;
        robot_speed(num1, num2);
        break;
    case 'M':
        ESP_LOGI(TAG, "MOVE");
        switch (ctrl)
        {
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
        } /* switch */

        break;
    case 'C':
        sscanf(data, "C%d|%d|%d", &num1, &num2, &num3);
        num1 = num1 < 0 ? 0 : num1 > 255 ? 255 : num1;
        num2 = num2 < 0 ? 0 : num2 > 255 ? 255 : num2;
        num3 = num3 < 0 ? 0 : num3 > 255 ? 255 : num3;
        robot_light_color(num1, num2, num3);
        break;
    case 'L':
        if (strlen(data) > 2)
        {
            p0 = data[2];
        }
        else
        {
            p0 = '0';
        }

        switch (ctrl)
        {
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
    } /* switch */
} /* cmd */

static void chkQueue(int timeout)
{
    uint16_t clr[3];
    if (xQueueReceive(COLORS, &clr, timeout))
    {
        for (int i = 0; i < 3; i++)
        {
            if (clr[i] != 12)
            {
                ledc_ch[i].duty = clr[i];
            }
        }
    }
}

static void color_func_solid()
{
    chkQueue(200);
    for (int i = 0; i < 3; i++)
    {
        ledc_set_fade_with_time(LEDC_LS_MODE, ledc_ch[i].channel, ledc_ch[i].duty, 100);
        ledc_fade_start(LEDC_LS_MODE, ledc_ch[i].channel, LEDC_FADE_NO_WAIT);
    }
}

static void color_func_fade()
{
    chkQueue(0);
    for (uint8_t i = 0; i < 3; i++)
    {
        ledc_set_fade_with_time(LEDC_LS_MODE, ledc_ch[i].channel, ledc_ch[i].duty, ledc_fade_timeout);
        ledc_fade_start(LEDC_LS_MODE, ledc_ch[i].channel, LEDC_FADE_NO_WAIT);
    }

    if (ledc_fade_delay_top > 100)
    {
        vTaskDelay(ledc_fade_delay_top / portTICK_PERIOD_MS);
    }

    chkQueue(0);
    for (uint8_t i = 0; i < 3; i++)
    {
        ledc_set_fade_with_time(LEDC_LS_MODE, ledc_ch[i].channel, 0, ledc_fade_timeout);
        ledc_fade_start(LEDC_LS_MODE, ledc_ch[i].channel, LEDC_FADE_NO_WAIT);
    }

    if (ledc_fade_delay_bottom > 100)
    {
        vTaskDelay(ledc_fade_delay_bottom / portTICK_PERIOD_MS);
    }
}

static void ledc_task()
{
    //initialize fade service.
    ledc_fade_func_install(0);
    color_func func = color_func_fade;
    uint8_t effect;
    while (1)
    {
        if (xQueueReceive(COLORS_FUNC, &effect, 0))
        {
            func = COLOR_EFFECTS[effect];
        }

        func();
    }
}

static void monitor_task()
{
    const TickType_t xDelay = 20000 / portTICK_PERIOD_MS;
    while (1)
    {
        if (TX_FUNC != NULL)
        {
            TX_FUNC("ping", 4);
        }

        vTaskDelay(xDelay);
    }
}

void robot_enable()
{
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

    COLORS = xQueueCreate(1, sizeof(uint16_t[3]));
    COLORS_FUNC = xQueueCreate(1, 1);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000; // frequency = 500Hz,
    pwm_config.cmpr_a = 0;       // duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;       // duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    ledc_timer_config_t ledc_timer = {
        .bit_num = LEDC_TIMER_13_BIT, //set timer counter bit number
        .freq_hz = 5000,              //set frequency of pwm
        .speed_mode = LEDC_LS_MODE,   //timer mode,
        .timer_num = LEDC_LS_TIMER    //timer index
    };

    ledc_timer_config(&ledc_timer);

    for (uint8_t i = 0; i < 3; i++)
    {
        ledc_channel_config_t ledc_channel = {
            .channel = ledc_ch[i].channel,
            .duty = 0,
            .gpio_num = ledc_ch[i].gpio,
            .intr_type = LEDC_INTR_FADE_END,
            .speed_mode = LEDC_LS_MODE,
            .timer_sel = LEDC_LS_TIMER,
        };
        ledc_channel_config(&ledc_channel);
    }

    COLOR_EFFECTS[0] = color_func_fade;
    COLOR_EFFECTS[1] = color_func_solid;

    xTaskCreate(monitor_task, "ROBOT_MONITOR", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(ledc_task, "ROBOT_LED_FADE", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
}
