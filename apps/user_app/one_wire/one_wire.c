
#include "system/includes.h"
#include "one_wire.h"
#include "led_strand_effect.h"
#include "led_strip_sys.h" // USER_MSG_MOTOR_SEND_DATA

// 单线通信配置
#define INS_LEN 7 // 指令长度
#define W_0_5MS 4 // 脉宽0.5ms
#define W_1MS   8
#define W_2MS   16

// 存放待发送的单线通信数据
volatile u16 one_wire_data = 0;
// 电机转速
u8 period[6] = {8, 11, 14, 17, 20, 35};

// 单线发送使能，控制中断服务函数的执行
static volatile u8 one_wire_send_en = 0;

/**
 * @brief  mcu通讯接口
 *  
 */
void one_wire_pin_init(void)
{
    gpio_set_die(MOTOR_DRIVER_PIN, 1);
    gpio_set_pull_up(MOTOR_DRIVER_PIN, 1);
    gpio_direction_output(MOTOR_DRIVER_PIN, 1);
}

void one_wire_send_enable(void)
{
    one_wire_send_en = 1;
}

void one_wire_send_disable(void)
{
    one_wire_send_en = 0;
}

u8 one_wire_send_is_finish(void)
{
    return one_wire_send_en;
}

/**
 * @brief 准备待发送的单线通信数据
 * 
 */
void one_wire_set_data(void)
{
    // 设置待发送的数据之前，需要禁止相关处理函数的发送
    one_wire_send_en = 0; 

    one_wire_data = 0;
    one_wire_data |= fc_effect.base_ins.mode; // bit0 ~ bit2 电机模式

    // 电机速度（控制电机周期：旋转一周的时间）
    one_wire_data |= (u16)fc_effect.base_ins.period << 3;

    if (fc_effect.base_ins.dir) {
        one_wire_data |= BIT(6);
    } 
}

void one_wire_send_data_enable(void)
{
    os_taskq_post("msg_task", 1, USER_MSG_MOTOR_SEND_DATA);
}

/**
 * @brief 发送单线通信数据
 *  
 */
AT_VOLATILE_RAM_CODE
// void __attribute__((weak)) one_wire_send_handle(void)
void one_wire_send_handle(void)
{
    static volatile u16 idle_cnt = 0;
    static volatile u8 step = 0;
    static volatile u8 send_cnt = 0;
    static volatile u8 _125ms_cnt = 0;
    static volatile u8 h_l = 0; //0:输出低电平，1：高电平

    static volatile u16 send_data = 0;
    if (one_wire_send_en == 0) {
        // 发送未使能，直接返回
        idle_cnt = 0;
        step = 0;
        send_cnt = 0;
        _125ms_cnt = 0;
        h_l = 0;
        gpio_direction_output(MOTOR_DRIVER_PIN, 1); // 发送空闲时，输出高电平
        return;
    }

    send_data = one_wire_data;

    switch (step) {
    case 0: //起始位
        /***********************************************************/
        //解决了app发送指令的，波形不正确的问题，但是问题愿意未清晰
        if (idle_cnt <= 40) {
            // 40 * 125us == 5ms
            idle_cnt++;
            return;
        }
        /**********************************************************/
        if (h_l == 0) {
            gpio_direction_output(MOTOR_DRIVER_PIN, 0);
            _125ms_cnt++;
            if (_125ms_cnt > (W_1MS)) {
                // 从1开始
                gpio_direction_output(MOTOR_DRIVER_PIN, 1);

                h_l = 1;
                _125ms_cnt = 0;
            }
        } else {
            _125ms_cnt++;

            if (_125ms_cnt == W_1MS) {
                gpio_direction_output(MOTOR_DRIVER_PIN, 0);
                h_l = 0;
                step = 1;
                _125ms_cnt = 0;
            }
        }
        break;

    case 1:
        if (h_l == 0) {
            _125ms_cnt++;

            if (_125ms_cnt == W_0_5MS) {

                gpio_direction_output(MOTOR_DRIVER_PIN, 1);
                h_l = 1;
                _125ms_cnt = 0;
            }
        } else {
            if ((send_data >> send_cnt) & 0x01) //1
            {
                // gpio_direction_output(IO_PORTA_00, 1);
                _125ms_cnt++;

                if (_125ms_cnt == W_1MS) {
                    gpio_direction_output(MOTOR_DRIVER_PIN, 0);

                    h_l = 0;
                    _125ms_cnt = 0;
                    // 完成1bit发送
                    send_cnt++;
                    if (send_cnt == INS_LEN) {
                        send_cnt = 0;
                        step = 2;
                    }
                }
            } else {
                // gpio_direction_output(IO_PORTA_00, 1);
                _125ms_cnt++;
                if (_125ms_cnt == W_0_5MS) {
                    gpio_direction_output(MOTOR_DRIVER_PIN, 0);
                    h_l = 0;
                    _125ms_cnt = 0;
                    // 完成1bit发送
                    send_cnt++;
                    if (send_cnt == INS_LEN) {
                        send_cnt = 0;
                        step = 2;
                    }
                }
            }
        }

        break;

    case 2:
        if (h_l == 0) {
            gpio_direction_output(MOTOR_DRIVER_PIN, 0);
            _125ms_cnt++;
            if (_125ms_cnt == W_2MS) {
                gpio_direction_output(MOTOR_DRIVER_PIN, 1);

                _125ms_cnt = 0;
                step = 0;
                send_cnt = 0;
                one_wire_send_en = 0; //等待下一次触发
            }
        }
        break;
    }
}

// -------------------------------------------------------------------API
/**
 * @brief 设置电机的模式
 * 
 * @param m       // 000:回正
                 // 001:区域1摇摆
                // 010:区域2摇摆
                // 011:区域1和区域2摇摆
                // 100:360°正转
               // 101:音乐律动
 */

void one_wire_set_mode(u8 m)
{
    fc_effect.base_ins.mode = m;
    printf("base_ins.mode = %d", fc_effect.base_ins.mode);
}
/**
 * @brief 设置电机转速
 * 
 * @param p ：8s 13s 18s 21s 26s
 */
void one_wire_set_period(u8 p)
{
    fc_effect.base_ins.period = p;
    printf("base_ins.period = %d", fc_effect.base_ins.period);
}

/**
 * @brief 设置电机正反转
 * 
 */
void one_wire_set_dir(void)
{
    printf("base_ins.dir = %d", fc_effect.base_ins.dir);
    fc_effect.base_ins.dir = !fc_effect.base_ins.dir;
}
/**
 * @brief Get the stepmotor mode object
 * 获取电机当前模式
 * 
 * @return u8 
 */
u8 get_stepmotor_mode(void)
{
    printf(" base_ins.mode = %d", fc_effect.base_ins.mode);
    return fc_effect.base_ins.mode;
}

/*************************************音乐律动模式**************************************/

/**
 * @brief 效果：反转
 * 
 */
void stepmotor_direction(void)
{
    // one_wire_set_dir();
    fc_effect.base_ins.dir = 1;
}

/**
 * @brief 效果：调到最慢速度
 * 
 */
void stepmotor_music_minSpeed(void)
{

    fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 26;
}

/**
 * @brief 效果：调到最快速度
 * 
 */
void stepmotor_music_maxSpeed(void)
{
    fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 8;
}

void set_stepmotor_music_mode(void)
{

    switch (fc_effect.base_ins.music_mode) {
    case 0:

        stepmotor_direction();
        one_wire_set_data();
        one_wire_send_data_enable();
        break;
    case 1:

        stepmotor_music_minSpeed();
        one_wire_set_data();
        one_wire_send_data_enable();
        break;
    case 2:

        stepmotor_music_maxSpeed();
        one_wire_set_data();
        one_wire_send_data_enable();
        break;
    }
}

/**
 * @brief 设置是最慢
 * 
 */
void set_stepmotor_slow(void)
{
    if (fc_effect.base_ins.period != 26) {
        stepmotor_music_minSpeed();
        one_wire_set_data();
        one_wire_send_data_enable();
    }
}

/**
 * @brief 设置是最快
 * 
 */
void set_stepmotor_fast(void)
{
    if (fc_effect.base_ins.period != 8) {
        stepmotor_music_maxSpeed();
        one_wire_set_data();
        one_wire_send_data_enable();
    }
}

/**
 * @brief 声控步进电机
 * 
 */

u8 stepmotor_sound_cnt = 0;
void effect_stepmotor(void)
{

    if (fc_effect.base_ins.mode == 0x05) {

        if (get_sound_result()) {
            set_stepmotor_fast();

            stepmotor_sound_cnt = 0;
        }

        if (stepmotor_sound_cnt < 100) {
            stepmotor_sound_cnt++;
        }
        if (stepmotor_sound_cnt >= 100) {
            set_stepmotor_slow();
        }
    }
}

u8 counting_flag = 0; //1：计时中， 0：计时完成
u16 stop_cnt = 0;
u8 set_time = 0; //1：不允许修改时间  0：允许修改时间
u8 temp = 0;
u8 long_key;
void clean_stepmorot_flag(void)
{
    counting_flag = 0;
    stop_cnt = 0;
    set_time = 1;
    long_key = 0; //长按立即停的标志
}
//10计时
void stepmotor(void)
{

    if (set_time == 1) {

        set_time = 0;
        temp = fc_effect.base_ins.period;

        if (fc_effect.on_off_flag == DEVICE_OFF || long_key == 1)
            temp = 0;
    }
    if (counting_flag == 1) {

        if (stop_cnt == temp * 100) {
            one_wire_set_mode(6);
            one_wire_set_data();
            one_wire_send_data_enable();
            clean_stepmorot_flag();
        } else {
            stop_cnt++;
        }
    }
}
