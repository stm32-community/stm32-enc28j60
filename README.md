# TOOLS for STM32
## ENC28J60
ENC28J60 ethernet module for STM32 MCU

Search terms for `ENC28J60/HR911105A datasheet` and `enc28j60 schematic` for detail about hardware.

About Cmake, this code was ported using [stm32-cmake](https://github.com/ObKo/stm32-cmake) and [stm32-base](https://github.com/DVALight/stm32-base)

## UDP Command Hnadler
Domotics !

## RTC
Active the code on file [`inc/define.h`](inc/defines.h).
Add the code needed in main.c `between USER code begin` and `User code end` of the section RTC created by your STM32cubeIDE, you should find examples files in this repo.

## MCU tested (please share your results for update this list):
- STM32F091xC (v2 Tested only on sending UDP packets)
- STM32F030x6 (v2 Tested only on sending UDP packets)
- STM32F103xB (v2 Tested only on sending UDP packets)
- STM32F401VE (v2 Tested only on sending UDP packets)
- STM32F407 (v3 complete network interface)
- STM32F411 (will be test current 2024 for v3 complete network interface)

# News on branch V3

Quick Start for Full Network Connection
To quickly set up a complete network connection (DHCP, DNS, Gateway, etc.), use the following function:
`ES_FullConnection();` from the file: `EtherShield.c`
Call this fonction just before the while and ping your device !

and in your while think to use this for treat everything from your network :
` ES_ProcessWebPacket();`

A lot of code was clear, if any function deseaper please create a Issue


# How to use this project

Copy it into your project and set up your project to be built with this library.
copy all files /inc and /src respectibly to the folder.

Main.c and Main.h examples should be present. Shared your files and create folder for each MCU for help the community.

About CMake, see the section Cmake below.

## Expected code from STM32cubeIDE configuration

You have to configure via STM32cubeIDE your SPI, automaticaly this code should be create in your main.c

`void ENC28J60_hspi->Instance_Configuration(void)
{
    SPI_InitTypeDef   SPI_InitStructure;

    /* Enable the SPI periph */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_hspi->Instance, ENABLE);

    /* SPI configuration -------------------------------------------------------*/
    SPI_I2S_DeInit(hspi->Instance);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_Init(hspi->Instance, &SPI_InitStructure);

    /* Enable hspi->Instance  */
    SPI_Cmd(hspi->Instance, ENABLE);
}`

Suggestion of GPIO Configuration

`void ENC28J60_GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable SCK, MOSI and MISO GPIO clocks */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  /* Enable CS  GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_hspi->Instance);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_hspi->Instance);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_hspi->Instance);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN_3;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* SPI MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Configure GPIO PIN for Chip select */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Deselect : Chip Select high */
  GPIO_SetBits(GPIOA, GPIO_PIN_4);
}`


### FILE [`inc/define.h`](inc/defines.h) Hardware SECTION

If you need to use another STM32 MCU then don't forget to edit file  in Hardware section.


### FILE [`inc/define.h`](inc/defines.h)  Parameters SECTION

Chose what you want, depend on your needs.

### FILE [`inc/define.h`](inc/defines.h)  CS SECTION
Very important part if you choose CS_Only, you have to specify this correcty.

### About CMake
Use the [stm32-cmake](https://github.com/ObKo/stm32-cmake) (see the next section).

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