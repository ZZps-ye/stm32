#ifndef __APP_CAR_H
#define __APP_CAR_H

#include "main.h"
#include "cmsis_os2.h"

//小车工作模式，三者互斥
typedef enum
{
    CAR_MODE_MANUAL = 0,    /* 手动蓝牙遥控（默认）*/
    CAR_MODE_AVOID,         /* 超声波自动避障 */
    CAR_MODE_TRACK,         /* 四路循迹 */
} CarMode;

//电机动作类型
typedef enum
{
    CAR_ACT_STOP = 0,       /* 停止 */
    CAR_ACT_FORWARD,        /* 前进 */
    CAR_ACT_BACKWARD,       /* 后退 */
    CAR_ACT_LEFT,           /* 原地左转 */
    CAR_ACT_RIGHT,          /* 原地右转 */
    CAR_ACT_REFRESH,        /* 用消息里的新速度重发上一个动作（数字键调速立即生效）*/
} CarAction;

//动作队列消息
typedef struct
{
    CarAction action;       /* 动作类型 */
    uint16_t  speed;        /* 目标速度（0~999，对应 PWM 占空比千分比）*/
    uint16_t  duration_ms;  /* 动作持续时间；0 = 持续到下一条指令（手动模式），
                               >0 = 执行这么久后自动停车（避障转向用）*/
} CarActionMsg;

//全局RTOS对象
extern osMessageQueueId_t qCarAction;   /* 动作队列：bt/avoid/track -> motor */
extern osMessageQueueId_t qBt;          /* 蓝牙字节队列：UART 中断 -> btTask */
extern osMutexId_t        mCfg;         /* 配置互斥锁：保护 s_mode / s_speed */

//读写API
CarMode  Car_GetMode(void);
void     Car_SetMode(CarMode mode);     /* 切模式时会清空动作队列并停车 */
uint16_t Car_GetSpeed(void);
void     Car_SetSpeedVal(uint16_t speed);

//动作发送API
void Car_SendAction(CarAction action, uint16_t speed, uint16_t duration_ms);
void Car_SendStop(void);

#endif
