
#ifndef __LED_STRIP_DRIVE_H__
#define __LED_STRIP_DRIVE_H__

#include "board_ac632n_demo_cfg.h"
#include "asm/ledc.h"
#include "asm/gpio.h"
#include "user_config.h"

#define MIC_PIN  IO_PORTB_06
#define LEDC_PIN IO_PORTB_07

#define FAN_DRIVER_PIN IO_PORTB_05

// R、G、B、W通道驱动引脚
// 如果不使用测试板
#if (0 == USER_TEST_USE_ANOTHER_BOARD)
#define LED_DRIVER_R_PIN IO_PORTB_07
#define LED_DRIVER_G_PIN IO_PORTA_00
#define LED_DRIVER_B_PIN IO_PORTA_01
#define LED_DRIVER_W_PIN IO_PORTA_02
#else
// 如果使用测试板
#define LED_DRIVER_R_PIN IO_PORTA_07
#define LED_DRIVER_G_PIN IO_PORTA_01
#define LED_DRIVER_B_PIN IO_PORTA_02
#define LED_DRIVER_W_PIN IO_PORTB_06
#endif

typedef enum
{
    OFF, // mic关闭
    ON,  // mic打开
} MIC_OFFON;

extern MIC_OFFON MIC_ENABLE;

void led_state_init(void);

void ledc_init(const struct ledc_platform_data *arg);

void mic_adc_init(void);

#endif
