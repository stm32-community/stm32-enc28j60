#ifndef __STM32INCLUDES_H
#define __STM32INCLUDES_H

#if STM32F0
/*#	if STM32F091xC
#		include "stm32f091xc.h"
#	endif*/
#	include "stm32f0xx.h"
#	include "stm32f0xx_hal_def.h"
#	include "stm32f0xx_hal_spi.h"
#elif STM32F1
//#	include "stm32f103xb.h"
#	include "stm32f1xx.h"
#	include "stm32f1xx_hal_def.h"
#	include "stm32f1xx_hal_spi.h"
#elif STM32F4
# include "stm32f4xx.h"
# include "stm32f4xx_hal.h"
# include "stm32f4xx_hal_gpio.h"
# include "stm32f4xx_hal_def.h"
# include "stm32f4xx_hal_spi.h"
#endif

#include <stdint.h>

#endif
