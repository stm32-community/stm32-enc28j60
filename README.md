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
`ES_FullConnection();` in the file: `EtherShield.c`
Call this fonction just before the while and ping your device !

and in your while think to use this for treat everything from your network :
` paquetweb();`

Options for NSS Output Signal in defines.h that's a simple desactivation and activation on the file enc28j60.h.

## CMake

This project based on [stm32-cmake](https://github.com/ObKo/stm32-cmake), read it's readme and install, then set appropriate environment variables, see [`stm32-env-example.ps1`](stm32-env-example.ps1)

The project comes with a template of a cmake configuration, so that it builds as library that can be linked using CMake in other STM32 projects. Use variables defined in [`CMakeLists.txt`](CMakeLists.txt) to enable, disable and configure internet modules.

To make it work for your MCU family and model you must change `MCU_FAMILY` and `MCU_MODEL` CMake variables.

## Examples

* [dc-thermal-logger](https://github.com/mephi-ut/dc-thermal-logger/blob/master/collector/firmware/Src/main.c)
* [STM32_Devel](https://github.com/mephi-ut/STM32_Devel)
* [nucleo-f091rc-rs232-enc28j60](https://github.com/mephi-ut/nucleo-f091rc-rs232-enc28j60/blob/master/Src/main.c)

# License
Mainpage Arduino ENC28J60 EtherShield Library
## section Introduction
This library is derived from original code by Guido Socher and Pascal Stang, and hence licensed as GPL2. See http://www.gnu.org/licenses/gpl.html
It comprises a C++ class wrapper and a number of C files. It still follows pretty much the same structure as the original code that it was based on.
The Arduino EtherShield Library was initially created by Xing Yu of Nuelectronics, http://www.nuelectronics.com/estore/index.php?main_page=product_info&cPath=1&products_id=4
The library was heavily modified and improved by Andrew D. Lindsay (http://blog.thiseldo.co.uk) with extra code from the Tuxgraphics.org ethernet library (http://www.tuxgraphics.org/electronics/200905/embedded-tcp-ip-stack.shtml), which also originated from the Pascal Stang code.
Further additions include the DHCP implementation with some assistance from JCW at http://jeelabs.org which is now used in their own version of the library for their EtherCard at http://jeelabs.net/projects/hardware/wiki/Ether_Card.
The library is now being used successfully with the Nanode, as minimal Ethernet connected Arduino compatible board, details available from http://wiki.london.hackspace.org.uk/view/Project:Nanode

## section Download
Download the latest library and examples from https://github.com/thiseldo/EtherShield
To fully utilise the Nanode board, you will also require a library that can access the onboard MAC address chip.
One such library is available from https://github.com/thiseldo/NanodeMAC and is used in the examples provided with this library.

## section Instalation
The library .zip file downloaded from https://github.com/thiseldo/EtherShield should be renamed to EtherShield.zip or EtherShield.tar.gz depending on the archive file you're downloading.
The file should be extracted to the sketchbook/libraries/ folder so that there is a subdirectory called EtherSheild containing all the files from the archive.

## section License
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  A copy of the GNU Lesser General Public
  License is available from http://www.gnu.org/licenses/gpl.html; or write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA