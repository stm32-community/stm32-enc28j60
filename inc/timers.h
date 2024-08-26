/*
 * timers.h
 *
 *  Created on: May 30, 2024
 *      Author: dtneo
 */

#ifndef TIMERS_H
#define TIMERS_H

#include "main.h"

extern TIM_HandleTypeDef htim4;

void timerLog(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void generalTimersInit();


#endif // TIMERS_H
