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
    src/stm32f4xx_hal_conf.h
    src/defines.h
    src/enc28j60.h
    src/ip_arp_udp_tcp.h
    src/net.h
    src/dhcp.h
    src/dnslkup.h
    src/websrv_help_functions.h
    src/EtherShield.h
)

set(ETHERNET_LED_GPIO       "GPIOA"         CACHE INTERNAL "GPIO for ethernet LED")
set(ETHERNET_LED_PIN        "GPIO_PIN_2"    CACHE INTERNAL "PIN for ethernet LED")
set(ETHERNET_CS_GPIO        "GPIOA"         CACHE INTERNAL "GPIO for SPI chip select")
set(ETHERNET_CS_PIN         "GPIO_PIN_3"    CACHE INTERNAL "PIN for SPI chip select")
set(ETHERNET_CS_DELAY       "2"             CACHE INTERNAL "chip-select delay for ehternet")

set(UDP_client              "1"     CACHE INTERNAL "enables UDP transport protocol")
set(DNS_client              "1"     CACHE INTERNAL "enables DNS client")
set(DHCP_client             "1"     CACHE INTERNAL "enables DHCP client")
set(PING_client             "1"     CACHE INTERNAL "enables ICMP ping client")
set(PINGPATTERN             0x42    CACHE INTERNAL "ping pattern value")
set(TCP_client              "1"     CACHE INTERNAL "enables TCP transport protocol")
set(WWW_client              "1"     CACHE INTERNAL "enables HTTP")
set(FROMDECODE_websrv_help  "1"     CACHE INTERNAL "")
set(URLENCODE_websrv_help   "1"     CACHE INTERNAL "")

add_compile_definitions(
    ETHERNET_LED_GPIO=${ETHERNET_LED_GPIO}
    ETHERNET_LED_PIN=${ETHERNET_LED_PIN}
    ETHERNET_CS_GPIO=${ETHERNET_CS_GPIO}
    ETHERNET_CS_PIN=${ETHERNET_CS_PIN}
    ETHERNET_CS_DELAY=${ETHERNET_CS_PIN}

    UDP_client=${UDP_client}
    DNS_client=${DNS_client}
    DHCP_client=${DHCP_client}
    PING_client=${PING_client}
    PINGPATTERN=${PINGPATTERN}
    TCP_client=${TCP_client}
    WWW_client=${WWW_client}
    FROMDECODE_websrv_help=${FROMDECODE_websrv_help}
    URLENCODE_websrv_help=${URLENCODE_websrv_help}
)

include_directories(src)
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