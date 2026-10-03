#include "app_bt.h"
#include "app_car.h"
#include "cmsis_os2.h"

extern UART_HandleTypeDef huart1;

static uint8_t bt_rx_byte;

//蓝牙任务
void BtTask(void *argument)
{
	uint8_t ch;
	HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1);
	for (;;)
	{
		//阻塞等字节
		if (osMessageQueueGet(qBt, &ch, NULL, osWaitForever) != osOK)
		{
			continue;
		}
		switch (ch)
		{
			//模式切换
			case 'A': case 'a':  Car_SetMode(CAR_MODE_AVOID);  break;
			case 'T': case 't':  Car_SetMode(CAR_MODE_TRACK);  break;
            case 'M': case 'm':  Car_SetMode(CAR_MODE_MANUAL); break;
			
			//调速
			case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
			{
				uint16_t s = (uint16_t)(ch - '0') * 100;
                Car_SetSpeedVal(s);
                Car_SendAction(CAR_ACT_REFRESH, s, 0);
                break;
			}
			
			//方向模式,手动模式
			case 'F': case 'f':  Car_SendAction(CAR_ACT_FORWARD,  Car_GetSpeed(), 0); break;
            case 'B': case 'b':  Car_SendAction(CAR_ACT_BACKWARD, Car_GetSpeed(), 0); break;
            case 'L': case 'l':  Car_SendAction(CAR_ACT_LEFT,     Car_GetSpeed(), 0); break;
            case 'R': case 'r':  Car_SendAction(CAR_ACT_RIGHT,    Car_GetSpeed(), 0); break;
            case 'S': case 's':  Car_SendStop();                                      break;
			default:
                break;
		}
	}
}

//串口接收完成回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t ch = bt_rx_byte;
        (void)osMessageQueuePut(qBt, &ch, 0, 0);        /* 投队列，队列满则丢弃 */
        HAL_UART_Receive_IT(&huart1, &bt_rx_byte, 1);   /* 重开接收 */
    }
}
