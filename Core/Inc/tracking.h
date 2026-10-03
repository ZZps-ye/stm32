#ifndef __TRACKING_H
#define __TRACKING_H

#include "main.h"
#include "app_car.h"

//引脚定义
#define S1_PIN    GPIO_PIN_2      /* 左1 */
#define S1_PORT   GPIOA
#define S2_PIN    GPIO_PIN_3      /* 左2 */
#define S2_PORT   GPIOA
#define S3_PIN    GPIO_PIN_4      /* 右2 */
#define S3_PORT   GPIOA
#define S4_PIN    GPIO_PIN_5      /* 右1 */
#define S4_PORT   GPIOA

//函数声明
void       Tracking_Init(void);//配置四路传感器引脚为输入

CarAction  Tracking_Decide(void);//读传感器并决策，返回应执行的动作

#endif
