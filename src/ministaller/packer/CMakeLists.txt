cmake_minimum_required (VERSION 3.18)

## Project Definition and Options ##

project(minipacker)

## Source Files ##

add_dir(minipacker .)
add_file(minipacker ../backend/Internal.cpp)
add_file(minipacker ../backend/Packer.cpp)
add_dir(minipacker "${CMAKE_SOURCE_DIR}/zstd/lib/common")
add_dir(minipacker "${CMAKE_SOURCE_DIR}/zstd/lib/compress")
#add_dir(minipacker "${CMAKE_SOURCE_DIR}/zstd/lib/decompress")


## Define executables ##

add_executable(minipacker ${minipacker_SOURCES})


## Dependencies ##

# Msi #
target_link_libraries(minipacker Msi.lib)

# Zstd #
target_include_directories(minipacker PRIVATE "${CMAKE_SOURCE_DIR}/zstd/lib")


## Properties ##

# C++20 #
set_property(TARGET minipacker PROPERTY CXX_STANDARD 23)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"))
   target_compile_options(minipacker PRIVATE /Zi)
   set_target_properties(minipacker PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME minipacker
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB /ENTRY:Main /STACK:0x100000,0x100000")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS- /Gs16777216 /EHs-")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS- /Gs16777216 /EHs-")
  string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REPLACE "/RTC1" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  target_compile_definitions(minipacker PUBLIC _HAS_EXCEPTIONS=0)
endif()
