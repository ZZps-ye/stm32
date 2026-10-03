#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

#define SERVO_ANGLE_MIN   0       /* 最小角度 */
#define SERVO_ANGLE_MAX   180     /* 最大角度 */

//函数声明
void Servo_Init(void);//启动PWM并回中90度

void Servo_SetAngle(uint8_t angle); //设置舵机角度0-180

#endif
