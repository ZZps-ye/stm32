#include "app_tracking.h"
#include "app_car.h"
#include "tracking.h"
#include "cmsis_os2.h"

//循迹任务
void TrackTask(void *argument)
{
	CarAction act;
	for (;;)
	{
		//轮询检查当前模式
		if (Car_GetMode() != CAR_MODE_TRACK)
        {
            osDelay(50);
            continue;
        }
		
		//读取四路传感器并且决策
		act = Tracking_Decide();
		
		//下发动作
		Car_SendAction(act, TRACK_SPEED, 0);
		
		osDelay(TRACK_PERIOD_MS);
	}
}
