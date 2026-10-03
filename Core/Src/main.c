/**
  ******************************************************************************
  * @file    main.c
  * @brief   蓝牙智能小车主程序（FreeRTOS 版）
  *          STM32F103C8T6 + TB6612 + HC-06 + HC-SR04 + SG90 + TCRT5000
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "app_motor.h"
/* USER CODE BEGIN Includes */
#include <string.h>
#include "tim.h"        /* htim2/htim3 句柄 + MX_TIMx_Init 声明（CubeMX 生成）*/
#include "usart.h"      /* huart1 句柄 + MX_USART1_UART_Init 声明（CubeMX 生成）*/
#include "ultrasonic.h"
#include "servo.h"
#include "tracking.h"
#include "app_bt.h"
#include "app_car.h"
/* USER CODE END Includes */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void MX_FREERTOS_Init(void);        /* (MISRA C 2004 rule 8.1) 实现在 freertos.c */

int main(void)
{
  /* MCU 配置 ------------------------------------------------------------*/
  HAL_Init();
  /* 注意：HAL 时基不需要手动初始化！CubeMX 把 Timebase Source 切到 TIM4 后，
   * HAL_Init() 内部会自动调用 stm32f1xx_hal_timebase_tim.c 里的 HAL_InitTick()，
   * 由它配置并启动 TIM4 的 1ms 中断——这就是 SysTick 被RTOS 征用后的替代时基。 */

  /* 配置系统时钟：HSE 8MHz * PLL x9 = 72MHz（失败自动回退 HSI）*/
  SystemClock_Config();

  /* 外设初始化（实现在 CubeMX 生成的 tim.c/usart.c，参数与裸机版一致）*/
  MX_GPIO_Init();                   /* GPIO：电机方向脚 + STBY + 超声波 */
  MX_TIM3_Init();                   /* TIM3：PWM 输出（电机调速）*/
  MX_TIM2_Init();                   /* TIM2：PWM 输出（SG90 舵机）*/
  MX_USART1_UART_Init();            /* USART1：HC-06 蓝牙串口 */

  /* USER CODE BEGIN 2 */
  Motor_Init();                     /* 电机初始化：STBY 使能 + 停止 */
  Tracking_Init();                  /* 循迹传感器引脚配置 */
  Servo_Init();                     /* 舵机初始化：启动 PWM 并回中 90° */
  Ultrasonic_Init();                /* 超声波初始化：使能 DWT 计数器 */

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);         /* 启动电机A PWM */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);         /* 启动电机B PWM */

  /* 注意：这里【不再】调用 HAL_UART_Receive_IT ——
   * 中断回调里要用 RTOS 队列 API，必须等内核启动后在 BtTask 里开启，
   * 否则若字节恰好在内核启动前到达会触发 FreeRTOS 断言（见 app_bt.c 注释）*/
  /* USER CODE END 2 */

  /* 【FreeRTOS 关键配置】初始化调度器并启动：
   * 主循环 while(1) 从此不复存在 —— CPU 的使用权归内核调度 */
  osKernelInitialize();             /* 内核初始化（创建 RTOS 对象前必须调用）*/
  MX_FREERTOS_Init();               /* 创建队列/互斥锁/任务（freertos.c）*/
  osKernelStart();                  /* 启动调度器，从此不再返回 */

  while (1)
  {
    /* 永远不会到这里；万一调度器异常退出则停机 */
  }
}

/**
  * @brief  系统时钟配置（与裸机版完全一致）
  * @note   HSE 8MHz x9 = 72MHz；失败回退 HSI 4MHz x16 = 64MHz
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK)
  {
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
      Error_Handler();
    }
    return;
  }

  /* 回退：HSE 失败（无晶振），用内部 HSI 4MHz x 16 = 64MHz */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  GPIO 初始化：电机方向引脚 + STBY + 超声波（与裸机版一致）
  * @note   TIM 复用引脚（PA0/PA6/PA7）不用这里配——
  *         HAL_TIM_MspPostInit()（stm32f1xx_hal_msp.c）在 MX_TIMx_Init 内部已完成。
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* 超声波 TRIG：PA1 推挽输出（初始低电平）*/
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* 超声波 ECHO：PA8 浮空输入（5V 容忍脚，可直连 HC-SR04 Echo）*/
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* 电机方向脚与 STBY 初始输出高电平 */
  HAL_GPIO_WritePin(GPIOB, AIN1_PIN | AIN2_PIN | BIN1_PIN | BIN2_PIN | STBY_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = AIN1_PIN | AIN2_PIN | BIN1_PIN | BIN2_PIN | STBY_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  错误处理函数
  * @note   保持可中断的死循环（不用 __disable_irq），方便接调试器查看任务状态。
  */
void Error_Handler(void)
{
  while (1)
  {
    /* 可在此加：串口打印错误码 / 关闭电机 PWM 保证安全 */
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
