
#include "system/includes.h"
#include "syscfg_id.h"
#include "save_flash.h"
#include "rf433.h"
#include "user_config.h"

#define CFG_USER_LED_LEDGTH_DATA 3

u8 ble_state; //默认开启BLE模块
extern u32 RF433_CODE;
/*******************************************************************************************************
**函数名：上电读取FLASH里保存的指令数据
**输  出：
**输  入：读取CFG_USER_COMMAND_BUF_DATA里保存的最后一条接收到的指令，
**描  述：读取CFG_USER_LED_LEDGTH_DATA里保存的第一次上电标志，灯带长度，顺序是：：第1字节：第一次上电标志位，第2、3字节：灯带长度
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
void read_flash_device_status_init(void)
{
    int ret;
    save_flash_t save_flash3;

    memset((u8 *)&save_flash3, 0, sizeof(save_flash_t));

    ret = syscfg_read(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_flash3),
                      sizeof(save_flash_t));

    if (save_flash3.header != 0x55) //第一次上电
    {
#if USER_DEBUG_ENABLE
        printf("flash header error\n");
#endif
        fc_data_init();
        ble_state = 1; //默认开启BLE模块
    } else {
#if USER_DEBUG_ENABLE
        printf("flash header valid\n");
#endif
        memcpy((u8 *)(&fc_effect), (u8 *)(&save_flash3.fc_save),
               sizeof(fc_effect_t));
        ble_state = save_flash3.ble_state;
        RF433_CODE = save_flash3.rf433_code;
    }

#if USER_DEBUG_ENABLE
    // printf("fc_effect.base_ins.motor_on_off == %u\n",
    //        (u16)fc_effect.base_ins.motor_on_off);
    printf("fc_effect.base_ins.period == %u\n", (u16)fc_effect.base_ins.period);

    // printf("fc_effect.dream_scene.speed == %u\n", fc_effect.dream_scene.speed);
#endif
}

// 把用户数据写到区域3
void save_user_data_area3(void)
{
    save_flash_t save_data;
    save_data.header = 0x55;
    save_data.ble_state = ble_state;
    save_data.rf433_code = RF433_CODE;
    memcpy((u8 *)(&save_data.fc_save), (u8 *)(&fc_effect), sizeof(fc_effect_t));
    syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data),
                 sizeof(save_flash_t));

#if USER_DEBUG_ENABLE
    printf("user data save success\n");
#endif
}

void read_user_data_area3(void)
{
}
