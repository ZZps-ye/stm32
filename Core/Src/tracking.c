#include "tracking.h"

//读取四路传感器状态,1等于检测到黑线
static uint8_t Tracking_Read(void)
{
    uint8_t v = 0;
    if (HAL_GPIO_ReadPin(S1_PORT, S1_PIN) == GPIO_PIN_SET) v |= 0x08;
    if (HAL_GPIO_ReadPin(S2_PORT, S2_PIN) == GPIO_PIN_SET) v |= 0x04;
    if (HAL_GPIO_ReadPin(S3_PORT, S3_PIN) == GPIO_PIN_SET) v |= 0x02;
    if (HAL_GPIO_ReadPin(S4_PORT, S4_PIN) == GPIO_PIN_SET) v |= 0x01;
    return v;
}

//循迹传感器初始化
void Tracking_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = S1_PIN | S2_PIN | S3_PIN | S4_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

//循迹决策
CarAction Tracking_Decide(void)
{
	uint8_t v = Tracking_Read();
	
	if ((v & 0x06) == 0x06)
    {
        /* S2 与 S3（中间两个）同时压线：车在线中央 -> 直行
         * （全黑 0x0F 也满足本条件，走直行，行为正确）*/
        return CAR_ACT_FORWARD;
    }
    else if (v & 0x08)
    {
        /* S1 压线：线在 S1 那一侧，车偏另一侧 -> 右拐回正（幅度最大）*/
        return CAR_ACT_LEFT;
    }
    else if (v & 0x01)
    {
        /* S4 压线 -> 左拐回正（幅度最大）*/
        return CAR_ACT_RIGHT;
    }
    else if (v & 0x04)
    {
        /* S2 单独压线 -> 右拐（轻微）*/
        return CAR_ACT_RIGHT;
    }
    else if (v & 0x02)
    {
        /* S3 单独压线 -> 左拐（轻微）*/
        return CAR_ACT_LEFT;
    }
    else if (v == 0x0F)
    {
        /* 全黑：十字路口或终点，先直行
         *（实际到不了这里——全黑已在第一个分支按直行处理，保留作注释说明）*/
        return CAR_ACT_FORWARD;
    }
    else
    {
        /* 全白：丢线，停车等待（防止冲出去）*/
        return CAR_ACT_STOP;
    }
}
