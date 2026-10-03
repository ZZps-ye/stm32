#include "app_car.h"
#include "app_motor.h"

//受保护的配置数据
static CarMode  s_mode  = CAR_MODE_MANUAL; //默认是蓝牙模式
static uint16_t s_speed = 400;//初始速度

//配置读写
CarMode Car_GetMode(void)
{
    CarMode m;
    osMutexAcquire(mCfg, osWaitForever);//申请互斥锁
    m = s_mode;
    osMutexRelease(mCfg);//释放互斥锁
    return m;
}

void Car_SetMode(CarMode mode)
{
    osMutexAcquire(mCfg, osWaitForever);
    s_mode = mode;
    osMutexRelease(mCfg);

    /* 关键一步：切模式时清空动作队列。
     * 场景：避障任务刚发了一条"左转 550ms"还在队列里，
     * 此时用户切回手动 —— 不清队列的话这条旧动作会被 motorTask 执行，
     * 车在手动模式下莫名其妙转一下。清队列 = 状态机干净切换。 */
    osMessageQueueReset(qCarAction);
    Car_SendStop();     /* 任何模式切换都先停车，安全第一 */
}

uint16_t Car_GetSpeed(void)
{
    uint16_t s;
    osMutexAcquire(mCfg, osWaitForever);
    s = s_speed;
    osMutexRelease(mCfg);
    return s;
}

void Car_SetSpeedVal(uint16_t speed)
{
    osMutexAcquire(mCfg, osWaitForever);
    s_speed = speed;
    osMutexRelease(mCfg);
}

//动作发送
void Car_SendAction(CarAction action, uint16_t speed, uint16_t duration_ms)
{
    CarActionMsg msg;
    msg.action      = action;
    msg.speed       = speed;
    msg.duration_ms = duration_ms;
    (void)osMessageQueuePut(qCarAction, &msg, 0, 0);
}

void Car_SendStop(void)
{
    Car_SendAction(CAR_ACT_STOP, 0, 0);
}
