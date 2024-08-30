/**
 * @file timers.c
 * @brief Source file implementing timer-related functions for STM32, including initialization, logging, and interrupt handling.
 *
 * This file contains functions to initialize and manage timers on the STM32 platform. It includes
 * interrupt handling for the TIM4 timer and logging of timer states. The functions ensure that
 * timers are properly initialized, started, and managed during runtime.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
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
