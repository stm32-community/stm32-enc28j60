# stm32-enc28j60

STM32 ENC28J60 ethernet driver, ported for CMake using [stm32-base](https://github.com/DVALight/stm32-base) and [stm32-cmake](https://github.com/ObKo/stm32-cmake)

An implementation of ENC28J60 driver for STM32 MCU. Tested only on sending UDP packets and only on MCUs STM32F091xC, STM32F030x6 and STM32F103xB. If you need to use another STM32 MCU then don't forget to edit file [`src/stm32includes.h`](src/stm32includes.h).

## How to use it

* This library was tested in two ways. Either just copy it into your project and set up your project to be built with this library. This is how I used it.
* Or use the cmake template (see the next section).

## CMake

The project comes with a template of a cmake configuration, so that it builds as library that can be linked using CMake in other STM32 projects. Use variables defined in [`CMakeLists.txt`](CMakeLists.txt) to enable, disable and configure internet modules.

But to make it work uncomment the `list` and `target_link_libraries` lines and adapt to your specific MCU. Also, you might need to copy a header the `stm32*_hal_conf.h` file of your MCU (see the commented line `inc/stm32f4xx_hal_conf.h` in [`CMakeLists.txt`](CMakeLists.txt)).

## Examples

* [dc-thermal-logger](https://github.com/mephi-ut/dc-thermal-logger/blob/master/collector/firmware/Src/main.c)
* [STM32_Devel](https://github.com/mephi-ut/STM32_Devel)
* [nucleo-f091rc-rs232-enc28j60](https://github.com/mephi-ut/nucleo-f091rc-rs232-enc28j60/blob/master/Src/main.c)
