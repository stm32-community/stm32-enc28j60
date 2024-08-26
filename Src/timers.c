/*
 * timers.c
 *
 *  Created on: Aug 7, 2024
 *      Author: dtneo
 */

#include "timers.h"
#include <stdbool.h>
#include <stdio.h>
#include "LogManager.h"

// Counter for the number of timer interrupts
static uint32_t int_counterTIM4 = 0;

_Bool isInitializationDone = false;

const char* getTimerName(TIM_TypeDef *instance) {
    if (instance == TIM4) return "Timer 4";
    return "Unknown Timer";
}

void timerLog(TIM_HandleTypeDef *htim) {
	const char* timerName = getTimerName(htim->Instance);
    char logMessage[128];
    sprintf(logMessage, "%s, Counter=%lu, Period=%lu", timerName, (unsigned long)__HAL_TIM_GET_COUNTER(htim), (unsigned long)htim->Init.Period);
    udpLog2("Initialisation_Timers", logMessage);
}

void timerInit(TIM_HandleTypeDef *htim) {
	  HAL_TIM_Base_Start_IT(htim);
	  HAL_TIM_Base_Stop_IT(htim);
	  __HAL_TIM_SET_COUNTER(htim, 0);
	  timerLog(htim);
}

void generalTimersInit() {
	TIM4Flag = 0;

	timerInit(&htim4);

	isInitializationDone = true;

	// Timers initialisation is done, now active them needed permanently.
	HAL_TIM_Base_Start_IT(&htim4);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	if (!isInitializationDone) {
		return; // no - the initialization of timer is not done - we skip this function
	}

    if (htim->Instance == TIM4) {
        HAL_TIM_Base_Stop_IT(htim);
        int_counterTIM4++;
        if (int_counterTIM4 >= 10) //
        {
        	TIM4Flag = 1;
            int_counterTIM4 = 0;
        }
        HAL_TIM_Base_Start_IT(htim);
    }
}
