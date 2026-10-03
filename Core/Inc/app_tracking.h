#ifndef __APP_TRACKING_H
#define __APP_TRACKING_H

#include "main.h"

//循迹速度
#define TRACK_SPEED   500

//循迹决策周期
#define TRACK_PERIOD_MS  30

//任务入口
void TrackTask(void *argument);

#endif
