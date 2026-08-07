cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME panther-racing-litel)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)
# ── FatFs ──────────────────────────────────────────────────────────────────
set(FATFS_DIR ${CMAKE_CURRENT_LIST_DIR}/Lib/FatFs/source)

add_library(fatfs STATIC
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffunicode.c
)

target_include_directories(fatfs PUBLIC ${FATFS_DIR})

# diskio.c needs HAL headers + the HAL's compile definitions.
# Don't link stm32cubemx here — it carries HAL sources via target_sources(),
# so linking would compile the whole HAL into this library and produce
# duplicate symbols at final link. Extract only the properties we need.
target_include_directories(fatfs PRIVATE
    $<TARGET_PROPERTY:stm32cubemx,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(fatfs PRIVATE
    $<TARGET_PROPERTY:stm32cubemx,INTERFACE_COMPILE_DEFINITIONS>
)

target_compile_options(fatfs PRIVATE -Wno-unused-parameter -Wno-sign-compare)

# Link directories setup
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    fatfs
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Application/Src/ring_buffer.c
    Application/Src/can_ingest.c
    Application/Src/sd_logger.c
    Application/Src/rf_telemetry.c
    Application/Src/usb_handler.c
    Application/Src/state_machine.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Application/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

target_include_directories(stm32cubemx INTERFACE ${FATFS_DIR})