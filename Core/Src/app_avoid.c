#include "app_avoid.h"
#include "app_car.h"
#include "app_motor.h"
#include "servo.h"
#include "ultrasonic.h"
#include "cmsis_os2.h"

//测距返回0无回波的话视为无限远
#define SAFE_DIST(d)   ((d) == 0 ? 0xFFFF : (uint16_t)(d))

//测前方距离，3次去抖，取中值
static uint16_t MeasureFront(void)
{
    uint16_t v[3];
    uint8_t  n = 0, i, j;
    uint16_t tmp;

    v[n++] = Ultrasonic_GetDistanceCm();
    osDelay(50);
    { uint16_t d = Ultrasonic_GetDistanceCm(); if (d != 0) v[n++] = d; }
    osDelay(50);
    { uint16_t d = Ultrasonic_GetDistanceCm(); if (d != 0) v[n++] = d; }

    if (n == 0) return 0;
    if (n == 1) return v[0];
    if (n == 2) return (uint16_t)(((uint32_t)v[0] + v[1]) / 2);

    for (i = 0; i < 2; i++)
        for (j = i + 1; j < 3; j++)
            if (v[j] < v[i]) { tmp = v[i]; v[i] = v[j]; v[j] = tmp; }
    return v[1];
}

//测侧方距离
static uint16_t MeasureSide(void)
{
    uint16_t d1 = Ultrasonic_GetDistanceCm();
    osDelay(50);
    uint16_t d2 = Ultrasonic_GetDistanceCm();

    if (d1 == 0) return d2;
    if (d2 == 0) return d1;
    return (uint16_t)(((uint32_t)d1 + d2) / 2);
}

//执行一次完整的避障决策
static void Avoid_Step(void)
{
	uint16_t dist_front, dist_left, dist_right;

    /* ---------- 1. 舵机回正，测前方 ---------- */
    Servo_SetAngle(SERVO_CENTER_ANGLE);
    osDelay(150);
    dist_front = MeasureFront();
	
	//前方安全
	if (dist_front == 0 || dist_front > OBSTACLE_THRESHOLD_CM)
    {
        Car_SendAction(CAR_ACT_FORWARD, AVOID_SPEED, 0);
        return;
    }
	
	//疑似前方障碍，检测一下
	Car_SendStop();
    osDelay(100);
    {
        uint16_t d2 = MeasureFront();
        if (d2 == 0 || d2 > OBSTACLE_THRESHOLD_CM)
        {
            Car_SendAction(CAR_ACT_FORWARD, AVOID_SPEED, 0);
            return;//跳出Avoid_Step函数
        }
        if (d2 < dist_front) dist_front = d2;
    }
	
	//确认障碍，停车，左右扫测
	Car_SendStop();

    Servo_SetAngle(SERVO_LEFT_ANGLE);
    osDelay(200);
    dist_left = MeasureSide();

    Servo_SetAngle(SERVO_RIGHT_ANGLE);
    osDelay(200);
    dist_right = MeasureSide();

    Servo_SetAngle(SERVO_CENTER_ANGLE);
    osDelay(150);
	
	//决策：走空旷的一测，两侧都堵住则掉头
	if (SAFE_DIST(dist_left) > OBSTACLE_THRESHOLD_CM &&
        SAFE_DIST(dist_right) > OBSTACLE_THRESHOLD_CM)
	{
	if (SAFE_DIST(dist_left) >= SAFE_DIST(dist_right))
            Car_SendAction(CAR_ACT_LEFT,  AVOID_SPEED, TURN_TIME_MS);
        else
            Car_SendAction(CAR_ACT_RIGHT, AVOID_SPEED, TURN_TIME_MS);
	}
	else if (SAFE_DIST(dist_left) > OBSTACLE_THRESHOLD_CM)
    {
        Car_SendAction(CAR_ACT_LEFT,  AVOID_SPEED, TURN_TIME_MS);
    }
    else if (SAFE_DIST(dist_right) > OBSTACLE_THRESHOLD_CM)
    {
        Car_SendAction(CAR_ACT_RIGHT, AVOID_SPEED, TURN_TIME_MS);
    }
	else
    {
        Car_SendAction(CAR_ACT_LEFT,  AVOID_SPEED, TURN_TIME_MS * 2);   /* 掉头 180° */
    }
	
	//转向已自动停：直行缓冲，远离障碍防原地打转
	Car_SendAction(CAR_ACT_FORWARD, AVOID_SPEED, OBSTACLE_RECOVERY_MS);
}

//避障任务，优先级BelowNormal
void AvoidTask(void *argument)
{
	for (;;)
	{
		//轮询模式
		if (Car_GetMode() != CAR_MODE_AVOID)
        {
            osDelay(100);
            continue;
        }
		Avoid_Step();
		//决策间隔
		osDelay(50);
	}
}
