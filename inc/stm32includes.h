#ifndef __STM32INCLUDES_H
#define __STM32INCLUDES_H

#ifdef STM32F091xC
//#	include "stm32f091xc.h"
#	include "stm32f0xx.h"
#	include "stm32f0xx_hal_def.h"
#	include "stm32f0xx_hal_spi.h"
#endif
#ifdef STM32F103xB
//#	include "stm32f103xb.h"
#	include "stm32f1xx.h"
#	include "stm32f1xx_hal_def.h"
#	include "stm32f1xx_hal_spi.h"
#endif

#endif
