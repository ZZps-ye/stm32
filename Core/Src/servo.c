#include "servo.h"

extern TIM_HandleTypeDef htim2;//TIM2句柄

/* SG90 脉宽映射（PSC=71 时计数器为 1MHz，CCR 即微秒数）：
   0°   -> 0.5ms  -> CCR = 500
   90°  -> 1.5ms  -> CCR = 1500
   180° -> 2.5ms  -> CCR = 2500                                    */
#define SERVO_PULSE_MIN   500
#define SERVO_PULSE_MAX   2500

//舵机初始化，启动PWM,回中
void Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);   /* 启动 50Hz PWM 输出 */
    Servo_SetAngle(90);                          /* 回中 */
}

//设置舵机角度
void Servo_SetAngle(uint8_t angle)
{
    uint32_t pulse;

    if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;

    /* 角度 -> 脉宽 线性映射 */
    pulse = SERVO_PULSE_MIN + (uint32_t)((SERVO_PULSE_MAX - SERVO_PULSE_MIN) * angle / 180U);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}
