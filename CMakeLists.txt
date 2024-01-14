cmake_minimum_required(VERSION 3.16)

project(stm32-enc28j60 C ASM)
set(SOURCES
    src/enc28j60.c
    src/ip_arp_udp_tcp.c
    src/dhcp.c
    src/dnslkup.c
    src/websrv_help_functions.c
    src/EtherShield.c
    src/error_handler.c
)

set(HEADERS
    #inc/stm32f4xx_hal_conf.h
    inc/defines.h
    inc/enc28j60.h
    inc/ip_arp_udp_tcp.h
    inc/net.h
    inc/dhcp.h
    inc/dnslkup.h
    inc/websrv_help_functions.h
    inc/EtherShield.h
    inc/error_handler.h
)

set(ETHERNET_LED_GPIO       "GPIOA"         CACHE INTERNAL "GPIO for ethernet LED")
set(ETHERNET_LED_PIN        "GPIO_PIN_2"    CACHE INTERNAL "PIN for ethernet LED")
set(ETHERNET_CS_GPIO        "GPIOA"         CACHE INTERNAL "GPIO for SPI chip select")
set(ETHERNET_CS_PIN         "GPIO_PIN_3"    CACHE INTERNAL "PIN for SPI chip select")
set(ETHERNET_CS_DELAY       "1"             CACHE INTERNAL "chip-select delay for ehternet")

set(ETHERSHIELD_DEBUG       "1"             CACHE INTERNAL "enables debugging")

set(UDP_client              "1"                     CACHE INTERNAL "enables UDP transport protocol")
set(NTP_client              "1"                     CACHE INTERNAL "enables NTP client")
set(WOL_client              "1"                     CACHE INTERNAL "enables WOL client")
set(DNS_client              "1"                     CACHE INTERNAL "enables DNS client")
set(DHCP_client             "1"                     CACHE INTERNAL "enables DHCP client")
set(DHCP_HOSTNAME           "stm32-enc28j60"        CACHE INTERNAL "DHCP host name")
set(PING_client             "1"                     CACHE INTERNAL "enables ICMP ping client")
set(PINGPATTERN             0x42                    CACHE INTERNAL "ping pattern value")
set(TCP_client              "1"                     CACHE INTERNAL "enables TCP transport protocol")
set(WWW_client              "1"                     CACHE INTERNAL "enables HTTP")
set(WWW_USER_AGENT          "stm32-enc28j60/1.0"    CACHE INTERNAL "HTTP User-Agent")
set(FROMDECODE_websrv_help  "1"                     CACHE INTERNAL "")
set(URLENCODE_websrv_help   "1"                     CACHE INTERNAL "")

string(LENGTH ${DHCP_HOSTNAME} DHCP_HOSTNAME_LEN)
if(DHCP_HOSTNAME_LEN GREATER 32)
  message(SEND_ERROR "DHCP_HOSTNAME length > 32 limit")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

## STM32_TOOLCHAIN_PATH, STM32_TARGET_TRIPLET, STM32_CUBE_F4_PATH must be present in environment variables
## check https://github.com/DVALight/stm32-cmake for more info
#list(APPEND HAL_COMP_LIST STM32F4 CORTEX RCC GPIO SPI)
#list(APPEND CMSIS_COMP_LIST "STM32F4")

if(NOT STM32_CMAKE_PACKAGES)
    find_package(CMSIS COMPONENTS "${CMSIS_COMP_LIST}" REQUIRED)
    find_package(HAL COMPONENTS "${HAL_COMP_LIST}" REQUIRED)
    set(STM32_CMAKE_PACKAGES 1)
endif()

add_library(stm32-enc28j60 ${SOURCES} ${HEADERS})
target_include_directories(stm32-enc28j60 PUBLIC inc)
target_compile_definitions(stm32-enc28j60 PUBLIC
    ETHERNET_LED_GPIO=${ETHERNET_LED_GPIO}
    ETHERNET_LED_PIN=${ETHERNET_LED_PIN}
    ETHERNET_CS_GPIO=${ETHERNET_CS_GPIO}
    ETHERNET_CS_PIN=${ETHERNET_CS_PIN}
    ETHERNET_CS_DELAY=${ETHERNET_CS_DELAY}

    ETHERSHIELD_DEBUG=${ETHERSHIELD_DEBUG}

    UDP_client=${UDP_client}
    NTP_client=${NTP_client}
    WOL_client=${WOL_client}
    DNS_client=${DNS_client}
    DHCP_client=${DHCP_client}
    DHCP_HOSTNAME="${DHCP_HOSTNAME}"
    DHCP_HOSTNAME_LEN=${DHCP_HOSTNAME_LEN}
    PING_client=${PING_client}
    PINGPATTERN=${PINGPATTERN}
    TCP_client=${TCP_client}
    WWW_client=${WWW_client}
    WWW_USER_AGENT="${WWW_USER_AGENT}"
    FROMDECODE_websrv_help=${FROMDECODE_websrv_help}
    URLENCODE_websrv_help=${URLENCODE_websrv_help}
)

#target_link_libraries(stm32-enc28j60
#    HAL::STM32::F4::RCC
#    HAL::STM32::F4::GPIO
#    HAL::STM32::F4::CORTEX
#    CMSIS::STM32::F401VE
#    STM32::NoSys
#)

#add_custom_command(TARGET stm32-enc28j60 POST_BUILD
#    COMMAND ${CMAKE_OBJCOPY} -O ihex stm32-enc28j60.elf stm32-enc28j60.hex
#)

stm32_print_size_of_target(stm32-enc28j60)
