#include "app_motor.h"
#include "app_car.h"

//电机PWM通道
extern TIM_HandleTypeDef htim3;

static void MotorA_SetSpeed(uint16_t speed)
{
	if (speed > SPEED_MAX) speed = SPEED_MAX;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, speed);
}

static void MotorB_SetSpeed(uint16_t speed)
{
    if (speed > SPEED_MAX) speed = SPEED_MAX;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, speed);
}

//驱动层实现
void Motor_Init(void)
{
    HAL_GPIO_WritePin(STBY_PORT, STBY_PIN, GPIO_PIN_SET);   /* STBY=1，芯片使能 */
    MotorA_SetSpeed(0);
    MotorB_SetSpeed(0);
    Car_Stop();
}

void Car_Forward(uint16_t speed)
{
    MOTOR_A_FORWARD();
    MOTOR_B_FORWARD();
    MotorA_SetSpeed(speed);
    MotorB_SetSpeed(speed);
}

void Car_Backward(uint16_t speed)
{
    MOTOR_A_BACKWARD();
    MOTOR_B_BACKWARD();
    MotorA_SetSpeed(speed);
    MotorB_SetSpeed(speed);
}

void Car_Left(uint16_t speed)
{
    MOTOR_A_BACKWARD();
    MOTOR_B_FORWARD();
    MotorA_SetSpeed(speed);
    MotorB_SetSpeed(speed);
}

void Car_Right(uint16_t speed)
{
    MOTOR_A_FORWARD();
    MOTOR_B_BACKWARD();
    MotorA_SetSpeed(speed);
    MotorB_SetSpeed(speed);
}

void Car_Stop(void)
{
    MOTOR_A_STOP();
    MOTOR_B_STOP();
    MotorA_SetSpeed(0);
    MotorB_SetSpeed(0);
}

//任务层
// 记录上一个持续型动作，供 CAR_ACT_REFRESH（数字键调速）重放
// 注：重放时速度用消息里的新值，所以只需记动作、不需记旧速度
static CarAction s_last_action = CAR_ACT_STOP;

static void Motor_Apply(const CarActionMsg *msg)
{
	switch (msg->action)
	{
		    case CAR_ACT_STOP:
            Car_Stop();
            s_last_action = CAR_ACT_STOP;
            break;
			
			case CAR_ACT_FORWARD:
			case CAR_ACT_BACKWARD:
			case CAR_ACT_LEFT:
			case CAR_ACT_RIGHT:
			if (msg->action == CAR_ACT_FORWARD)  Car_Forward(msg->speed);
			else if (msg->action == CAR_ACT_BACKWARD) Car_Backward(msg->speed);
			else if (msg->action == CAR_ACT_LEFT)     Car_Left(msg->speed);
			else                                      Car_Right(msg->speed);
			/* 只有无时限的动作才记为"上一个动作"，定时动作（避障转向）不记 */
			if (msg->duration_ms == 0)
            {
                s_last_action = msg->action;
            }
            break;
			
			case CAR_ACT_REFRESH:
			if (s_last_action == CAR_ACT_FORWARD)  Car_Forward(msg->speed);
			if (s_last_action == CAR_ACT_BACKWARD) Car_Backward(msg->speed);
			if (s_last_action == CAR_ACT_LEFT)     Car_Left(msg->speed);
			if (s_last_action == CAR_ACT_RIGHT)    Car_Right(msg->speed);
            break;
			
			default:
            break;
	}
}

//电机任务
void MotorTask(void *argument)
{
		CarActionMsg msg;
		
		for (;;)
			{
		if (osMessageQueueGet(qCarAction, &msg, NULL, osWaitForever) != osOK)
				{
				continue;
				}
				
				Motor_Apply(&msg);
				
				/* 带时长的动作：到时间自动停车（避障的转向/缓冲用）*/
				        if (msg.duration_ms > 0 && msg.action != CAR_ACT_STOP)
					{
							osDelay(msg.duration_ms);
							Car_Stop();
					}
			}
}
