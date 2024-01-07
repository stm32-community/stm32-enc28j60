cmake_minimum_required(VERSION 3.16)

# STM32_TOOLCHAIN_PATH, STM32_TARGET_TRIPLET, STM32_CUBE_F4_PATH must be present in environment variables
# check https://github.com/DVALight/stm32-cmake for more info

project(stm32-enc28j60 C ASM)
set(SOURCES
    src/enc28j60.c
    src/ip_arp_udp_tcp.c
    src/dhcp.c
    src/dnslkup.c
    src/websrv_help_functions.c
    src/EtherShield.c
)

set(HEADERS
    src/defines.h
    src/enc28j60.h
    src/ip_config.h
    src/ip_arp_udp_tcp.h
    src/net.h
    src/dhcp.h
    src/dnslkup.h
    src/websrv_help_functions.h
    src/EtherShield.h
)

include_directories(src src/core)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(HAL_COMP_LIST RCC GPIO CORTEX)
set(CMSIS_COMP_LIST "")

list(APPEND CMSIS_COMP_LIST STM32F4)
list(APPEND HAL_COMP_LIST STM32F4)

find_package(CMSIS COMPONENTS "${CMSIS_COMP_LIST}" REQUIRED)
find_package(HAL COMPONENTS "${HAL_COMP_LIST}" REQUIRED)

add_library(stm32-enc28j60 ${SOURCES} ${HEADERS})
target_link_libraries(stm32-enc28j60
    HAL::STM32::F4::RCC
    HAL::STM32::F4::GPIO
    HAL::STM32::F4::CORTEX
    CMSIS::STM32::F401VE
    STM32::NoSys
)

#add_custom_command(TARGET stm32-enc28j60 POST_BUILD
#    COMMAND ${CMAKE_OBJCOPY} -O ihex stm32-enc28j60.elf stm32-enc28j60.hex
#)

stm32_print_size_of_target(stm32-enc28j60)