cmake_minimum_required (VERSION 3.8)

## Project Definition and Options ##

project(EGL3_SRV)


## Source Files ##

add_dir_recursive(EGL3_SRV .)
add_dir_recursive(EGL3_SRV ../utils/mmio)
add_dir_recursive(EGL3_SRV ../storage/game)
add_dir_recursive(EGL3_SRV ../disk)
add_file(EGL3_SRV ../storage/models/MountedDisk.cpp)
add_file(EGL3_SRV ../utils/StackTrace.cpp)
add_file(EGL3_SRV ../utils/Random.cpp)
add_file(EGL3_SRV ../srv_main.cpp)


## Define EGL3 executable ##

add_executable(EGL3_SRV ${EGL3_SRV_SOURCES})

target_compile_definitions(EGL3_SRV PRIVATE
    SERVICE_NAME="EGL3 Launcher"
    SERVICE_DESC="Handles the mounting for .egia files on EGL3's behalf"
)


## Dependencies ##

# WinSpd #
target_include_directories(EGL3_SRV PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\inc")
target_link_libraries(EGL3_SRV "$ENV{ProgramFiles\(x86\)}\\WinSpd\\lib\\winspd-x64.lib")

# NtDll (Used for mmio) #
target_link_libraries(EGL3_SRV "${CMAKE_SOURCE_DIR}\\src\\utils\\mmio\\ntdll64.lib")


## Properties ##

# C++20 #
set_property(TARGET EGL3_SRV PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3_SRV PRIVATE /Zi)
   set_target_properties(EGL3_SRV PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3_SRV 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()
