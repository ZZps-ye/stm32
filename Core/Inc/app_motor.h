#ifndef __APP_MOTOR_H
#define __APP_MOTOR_H

#include "main.h"

//引脚定义
#define AIN1_PIN    GPIO_PIN_0
#define AIN2_PIN    GPIO_PIN_1
#define AIN1_PORT   GPIOB
#define AIN2_PORT   GPIOB

#define BIN1_PIN    GPIO_PIN_10
#define BIN2_PIN    GPIO_PIN_11
#define BIN1_PORT   GPIOB
#define BIN2_PORT   GPIOB

#define STBY_PIN    GPIO_PIN_12
#define STBY_PORT   GPIOB
//速度上限
#define SPEED_MAX   999

//方向控制宏定义
#define MOTOR_A_FORWARD()  do{ HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_SET);   \
                               HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_RESET);}while(0)//左前进
#define MOTOR_A_BACKWARD() do{ HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_RESET); \
                               HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_SET);}while(0)//左后退
#define MOTOR_A_STOP()     do{ HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_RESET); \
                               HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_RESET);}while(0)//左停止
#define MOTOR_B_FORWARD()  do{ HAL_GPIO_WritePin(BIN1_PORT, BIN1_PIN, GPIO_PIN_SET);   \
                               HAL_GPIO_WritePin(BIN2_PORT, BIN2_PIN, GPIO_PIN_RESET);}while(0)//右前进
#define MOTOR_B_BACKWARD() do{ HAL_GPIO_WritePin(BIN1_PORT, BIN1_PIN, GPIO_PIN_RESET); \
                               HAL_GPIO_WritePin(BIN2_PORT, BIN2_PIN, GPIO_PIN_SET);}while(0)//右后退
#define MOTOR_B_STOP()     do{ HAL_GPIO_WritePin(BIN1_PORT, BIN1_PIN, GPIO_PIN_RESET); \
                               HAL_GPIO_WritePin(BIN2_PORT, BIN2_PIN, GPIO_PIN_RESET);}while(0)//右停止

//函数声明
void Motor_Init(void);			//STBY使能加停止
void Car_Forward(uint16_t speed);
void Car_Backward(uint16_t speed);
void Car_Left(uint16_t speed);
void Car_Right(uint16_t speed);
void Car_Stop(void);
//freertos.c任务入口
void MotorTask(void *argument);
#endif
