/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *
  *
  * 系统架构（任务间只通过队列和互斥锁通信，不共享裸变量）：
  *   [UART中断] --字节--> qBt --> [btTask] --动作--> qCarAction --+
  *                                                    +--> [motorTask] 唯一操作电机
  *   [trackTask] --30ms高频动作--> qCarAction --------+
  *   [avoidTask] --带时长转向动作-> qCarAction -------+
  *   模式/速度等共享配置由 mCfg 互斥锁保护（实现在 app_car.c）
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_car.h"
#include "app_motor.h"
#include "app_bt.h"
#include "app_avoid.h"
#include "app_tracking.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* ====== 全局 RTOS 对象句柄（app_car.h 里 extern 给各任务用）====== */
osMessageQueueId_t qCarAction;  /* 动作队列：bt/avoid/track -> motor，8 条 CarActionMsg */
osMessageQueueId_t qBt;         /* 蓝牙字节队列：UART 中断 -> btTask，16 字节 */
osMutexId_t        mCfg;        /* 配置互斥锁：保护当前模式/速度（app_car.c 内部）*/
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* ====== 创建互斥锁：保护 app_car.c 里的 s_mode / s_speed ======
   * 用互斥锁而不是二值信号量做锁，因为互斥锁带优先级继承，
   * 能缓解"低优先级持锁、高优先级等待、中优先级插队"的优先级反转 */
  mCfg = osMutexNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* ====== 创建两个消息队列 ======
   * 参数：(队列长度, 每条消息字节数, 属性)
   * 消息队列 put/get 都是值的拷贝——天然避免"发指针指向已销毁局部变量"的经典错误。
   * qCarAction 8 条：循迹 30ms 一条是最大生产者，motorTask 消费远快于生产；
   * qBt 16 字节：蓝牙是流式数据，16 字节缓冲足够覆盖任务调度的短暂延迟。 */
  qCarAction = osMessageQueueNew(8, sizeof(CarActionMsg), NULL);
  qBt        = osMessageQueueNew(16, sizeof(uint8_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* ====== 创建 4 个业务任务 ======
   * osThreadNew(任务函数, 参数, 属性)；属性传 NULL = 默认 Normal 优先级 + configMINIMAL_STACK_SIZE。
   * 注意 CMSIS-RTOS2 的 stack_size 单位是【字节】。
   * 栈给小了会溢出踩内存（症状是随机 hardfault），先给够再靠
   * uxTaskGetStackHighWaterMark() 观察余量往回收。
   * 优先级设计：蓝牙/循迹/电机 Normal（人机响应+控制实时性），
   *             避障 BelowNormal（一次决策几百 ms，让出优先级）。 */
  osThreadNew(MotorTask, NULL, NULL);   /* 电机任务：唯一操作电机的人 */

  {
    osThreadAttr_t attr = {0};
    attr.name       = "btTask";
    attr.priority   = (osPriority_t) osPriorityNormal;
    attr.stack_size = 512;
    osThreadNew(BtTask, NULL, &attr);    /* 蓝牙任务：解析指令 */

    attr.name       = "trackTask";
    osThreadNew(TrackTask, NULL, &attr); /* 循迹任务：30ms 周期 */

    attr.name       = "avoidTask";
    attr.priority   = (osPriority_t) osPriorityBelowNormal;
    attr.stack_size = 1024;              /* 避障局部变量多，栈给大点 */
    osThreadNew(AvoidTask, NULL, &attr); /* 避障任务：决策+发动作 */
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* ====== 栈溢出钩子（强烈建议保留）======
 * 前提：FreeRTOSConfig.h 里 configCHECK_FOR_STACK_OVERFLOW 已置 2
 * （CubeMX 界面：FREERTOS -> Config Parameters -> 搜 STACK_OVERFLOW）。
 * 内核会在任务切换时用"哨兵字"检测栈底是否被踩穿，
 * 调 RTOS 项目时任务栈溢出是最常见的死法，钩子能第一时间抓到凶手。 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  /* 到这里说明 pcTaskName 对应的任务栈溢出了 */
  (void)xTask;
  (void)pcTaskName;
  __disable_irq();
  while (1) { }
}
/* USER CODE END Application */
