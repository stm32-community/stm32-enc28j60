# stm32-enc28j60

STM32 ENC28J60 ethernet driver, ported for CMake using [stm32-cmake](https://github.com/ObKo/stm32-cmake) and [stm32-base](https://github.com/DVALight/stm32-base)

An implementation of ENC28J60 driver for STM32 MCU. Tested only on sending UDP packets and only on MCUs STM32F091xC, STM32F030x6, STM32F103xB and STM32F401VE.

You need to copy `stm32*_hal_conf.h` of your MCU to `inc` folder to make it work.

If you need to use another STM32 MCU then don't forget to edit file [`inc/stm32includes.h`](inc/stm32includes.h).

## How to use it

* Either just copy it into your project and set up your project to be built with this library.
* Or use the [stm32-cmake](https://github.com/ObKo/stm32-cmake) (see the next section).


### News on branch V3

Quick Start for Full Network Connection
To quickly set up a complete network connection (DHCP, DNS, Gateway, etc.), use the following function:
`void ES_FullConnection();` in the file: `EtherShield.c`
Call this fonction just before the while and ping your device !

and in your while think to use this for treat everything from your network :
`void paquetweb() {
	packetloop_icmp_tcp(packet, enc28j60PacketReceive(200, packet));
}`

## CMake

This project based on [stm32-cmake](https://github.com/ObKo/stm32-cmake), read it's readme and install, then set appropriate environment variables, see [`stm32-env-example.ps1`](stm32-env-example.ps1)

The project comes with a template of a cmake configuration, so that it builds as library that can be linked using CMake in other STM32 projects. Use variables defined in [`CMakeLists.txt`](CMakeLists.txt) to enable, disable and configure internet modules.

To make it work for your MCU family and model you must change `MCU_FAMILY` and `MCU_MODEL` CMake variables.

## Examples

* [dc-thermal-logger](https://github.com/mephi-ut/dc-thermal-logger/blob/master/collector/firmware/Src/main.c)
* [STM32_Devel](https://github.com/mephi-ut/STM32_Devel)
* [nucleo-f091rc-rs232-enc28j60](https://github.com/mephi-ut/nucleo-f091rc-rs232-enc28j60/blob/master/Src/main.c)
