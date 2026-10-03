#ifndef __APP_AVOID_H
#define __APP_AVOID_H

#include "main.h"

//可调参数
#define OBSTACLE_THRESHOLD_CM   30      /* 前方障碍判定距离(cm) */
#define SERVO_LEFT_ANGLE        45      /* 左扫角度 */
#define SERVO_CENTER_ANGLE      90      /* 正前方角度 */
#define SERVO_RIGHT_ANGLE       135     /* 右扫角度 */
#define TURN_TIME_MS            550     /* 转向幅度(ms) */
#define OBSTACLE_RECOVERY_MS    300     /* 转向后直行缓冲(ms) */
#define AVOID_SPEED             400     /* 避障行驶速度（不再共享手动速度）*/

//任务入口
void AvoidTask(void *argument);         

#endif
