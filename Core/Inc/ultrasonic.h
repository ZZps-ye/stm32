#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include "main.h"

//引脚定义
#define TRIG_PIN    GPIO_PIN_1
#define TRIG_PORT   GPIOA
#define ECHO_PIN    GPIO_PIN_8
#define ECHO_PORT   GPIOA

//函数声明
void     Ultrasonic_Init(void);//使能DWT内核计时器

uint16_t Ultrasonic_GetDistanceCm(void);//测距一次，返回距离，单位厘米

#endif
