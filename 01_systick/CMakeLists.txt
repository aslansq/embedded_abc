cmake_minimum_required(VERSION 3.10)

# Set the project name
project(demo.elf C ASM)

set(CMAKE_AR           $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-ar${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_ASM_COMPILER $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_C_COMPILER   $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_LINKER       $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX})
set(CMAKE_OBJCOPY      $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_RANLIB       $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-ranlib${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_SIZE         $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-size${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")
set(CMAKE_STRIP        $ENV{GCC_ARM_NONE_EABI_BIN_PATH}/arm-none-eabi-strip${CMAKE_EXECUTABLE_SUFFIX} CACHE INTERNAL "")

set(CMAKE_C_FLAGS  "-ffreestanding \
                    -mcpu=cortex-m0 \
                    -mfloat-abi=soft \
                    -Wall -Wextra -Wshadow \
                    -ggdb \
                    -Og \
                    -ffunction-sections \
                    -fdata-sections \
                    -mthumb" CACHE INTERNAL "")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/STM32F051R8TX_FLASH.ld \
                            -Xlinker -Map=demo.map \
                            -Xlinker --gc-sections \
                            -Xlinker --print-memory-usage \
                            -Xlinker --static \
                            --specs=nano.specs --specs=nosys.specs" CACHE INTERNAL "")

set_property(SOURCE startup_stm32f051r8tx.s APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")

file(GLOB demo_src "${CMAKE_SOURCE_DIR}/*.c")
# Add the executable
add_executable(demo.elf "${demo_src}"
                    "${CMAKE_SOURCE_DIR}/startup_stm32f051r8tx.s")
# Include directories
target_include_directories(demo.elf PUBLIC "${CMAKE_SOURCE_DIR}/"
                                       "${CMAKE_SOURCE_DIR}/CMSIS/Include"
                                       "${CMAKE_SOURCE_DIR}/CMSIS/Device/ST/STM32F0xx/Include")