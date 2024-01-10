# stm32-enc28j60

STM32 ENC28J60 ethernet driver, ported for CMake using [stm32-base](https://github.com/DVALight/stm32-base) and [stm32-cmake](https://github.com/ObKo/stm32-cmake)

## CMake

Builds as library that can be linked using CMake in other STM32 projects.

Fully configurable, use variables defined in CMakeLists.txt to enable, disable and configure internet modules

## Examples

[dc-thermal-logger](https://github.com/mephi-ut/dc-thermal-logger/blob/master/collector/firmware/Src/main.c)

[STM32_Devel](https://github.com/mephi-ut/STM32_Devel)

[nucleo-f091rc-rs232-enc28j60](https://github.com/mephi-ut/nucleo-f091rc-rs232-enc28j60/blob/master/Src/main.c)
