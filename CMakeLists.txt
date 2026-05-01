cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_PROJECT_NAME weather-station)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Include Ninja build system bootstrapping
include("CMake/dep-tools-ninja.cmake")

# Include toolchain download/bootstrapping
include("CMake/dep-tools-toolchain.cmake")

# Include toolchain file (must be before project() so compilers are set)
include("CMake/dep-arm-none-eabi-gcc.cmake")

# Optional bootstrap for clang-tidy/clang-format binaries.
include("CMake/dep-tools-clang.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM, C and CPP languages
enable_language(C ASM CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC
    USE_HAL_DRIVER
    STM32F746xx
    $<$<CONFIG:Debug>:DEBUG>
)

############################ INCLUDE DEPENDENCIES #############################
# Include STM32F7xx HAL drivers
include("CMake/dep-drivers-stm32.cmake")

# Include middleware dependencies (FreeRTOS, lwIP)
include("CMake/dep-middleware.cmake")

################################ TARGET DIRS ##################################
set(APP_PATH ${CMAKE_SOURCE_DIR}/Core/App)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/Core/App
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Inc
    ${CMAKE_SOURCE_DIR}/Core/Terminal
    ${CMAKE_SOURCE_DIR}/Config
    ${CMAKE_SOURCE_DIR}/SeggerRTT
    ${APP_PATH}/Tasks
    ${APP_PATH}/InterTaskCom
    ${APP_PATH}/Network
    ${APP_PATH}/Network/LwIP/BSP
    ${APP_PATH}/Network/LwIP
    ${APP_PATH}/Network/LwIP/arch
    ${APP_PATH}/Network/HttpClient
    ${APP_PATH}/Sensors
    ${APP_PATH}/Sensors/ds18b20
)

set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/Core/Startup/startup_stm32f746xx.s
    ${CMAKE_SOURCE_DIR}/Core/main.cpp
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Src/hardware_config.c
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Src/system_stm32f7xx.c
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Src/stm32f7xx_it.c
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Src/hal_tick_tim.c
    ${CMAKE_SOURCE_DIR}/Core/Hardware/Src/syscalls.c
    ${CMAKE_SOURCE_DIR}/Core/Terminal/terminal.c
    ${CMAKE_SOURCE_DIR}/SeggerRTT/SEGGER_RTT.c
    ${CMAKE_SOURCE_DIR}/SeggerRTT/SEGGER_RTT_printf.c
)

set(NETWORK_SOURCES
    ${APP_PATH}/Network/LwIP/BSP/lan8742.c
    ${APP_PATH}/Network/LwIP/sys_arch.c
    ${APP_PATH}/Network/LwIP/BSP/eth_phy_io.c
    ${APP_PATH}/Network/LwIP/ethernetif.c
    ${APP_PATH}/Network/network_task.cpp
)

set(SENSORS_SOURCES
    ${APP_PATH}/Sensors/ds18b20/ds18b20.cpp
    ${APP_PATH}/Sensors/ds18b20/ds18b20_it.cpp
    ${APP_PATH}/Sensors/sensors_task.cpp
)

set(APP_SOURCES
    ${APP_PATH}/app_init.cpp
    ${APP_PATH}/InterTaskCom/itc_manager.cpp
    ${SENSORS_SOURCES}
    ${NETWORK_SOURCES}
)

############################# TARGET ALL SOURCES ##############################
target_sources(${PROJECT_NAME} PRIVATE
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES}
    ${LWIP_SRCS}
    ${APP_SOURCES}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.bin
)

target_link_options(${PROJECT_NAME} PRIVATE
    --specs=nosys.specs
)

# Optional: run clang-format as part of the build
if(TARGET format)
    add_dependencies(${CMAKE_PROJECT_NAME} format)
endif()

if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()
