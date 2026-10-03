#include "ultrasonic.h"

//使能DWT CYCCNT计数器
void Ultrasonic_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* 使能跟踪模块 */
    DWT->CYCCNT = 0;                                   /* 计数器清零 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;               /* 使能周期计数 */
}

//延时函数
static void DelayUs(uint32_t us)
{
    uint32_t start  = DWT->CYCCNT;
    uint32_t target = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < target) { }
}

//触发一次HC-SR04并测距
uint16_t Ultrasonic_GetDistanceCm(void)
{
	uint32_t start, ticks;
    uint32_t t10ms = 10U   * (SystemCoreClock / 1000U);   /* 10ms 等回波超时 */
    uint32_t t40ms = 40U   * (SystemCoreClock / 1000U);   /* 40ms 量程上限 */
	
	/* 1. TRIG 拉高 >=10us 触发（实测 20us 更稳定）*/
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    DelayUs(20);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
	
	/* 2. 等待 ECHO 变高；10ms 无回波则超时返回 0 */
	start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET)
    {
        if ((DWT->CYCCNT - start) > t10ms) return 0;
    }
	
	/* 3. 计时 ECHO 高电平持续时间；40ms 超时（对应 4m 量程上限）*/
    start = DWT->CYCCNT;
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET)
    {
        if ((DWT->CYCCNT - start) > t40ms) return 0;
    }
    ticks = DWT->CYCCNT - start;
	
	/* 4. 换算：ticks -> us -> cm（距离 = t_us x 0.017）*/
    return (uint16_t)(ticks * 17U / ((SystemCoreClock / 1000000U) * 1000U));
}

