/**
 * @file timers.h
 * @brief Header file for timer-related functions and declarations for the STM32 platform.
 *
 * This file contains the declarations of functions for initializing, logging, and handling
 * timer interrupts, specifically for the TIM4 timer. It also includes the necessary external
 * references to the TIM handle and function prototypes.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 *
 * @created May 30, 2024
 * @author dtneo
 */

#ifndef TIMERS_H
#define TIMERS_H

#include "main.h"

extern TIM_HandleTypeDef htim4;

void timerLog(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void generalTimersInit();


#endif // TIMERS_H
