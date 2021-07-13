cmake_minimum_required (VERSION 3.18)

## Project Definition and Options ##

project(packer)

## Source Files ##

add_dir(packer .)
add_dir_recursive(packer ../backend)
add_file(packer ../../utils/KnownFolders.cpp)
add_file(packer ../../utils/Version.cpp)

ADD_VERSION_DEFS(../../utils/Version.cpp)

## Define executables ##

add_executable(packer ${packer_SOURCES})


## Dependencies ##

# Http #
target_link_libraries(packer Winhttp.lib)

# LZ4 #
find_package(lz4 REQUIRED)
target_link_libraries(packer lz4::lz4)

# JSON #
find_package(RapidJSON CONFIG REQUIRED)
target_include_directories(packer PRIVATE ${RAPIDJSON_INCLUDE_DIRS})


## Properties ##

# C++20 #
set_property(TARGET packer PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"))
   target_compile_options(packer PRIVATE /Zi)
   set_target_properties(packer PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME packer
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()



# Use static CRT (the code is memcpy-heavy, avoid calling those dlls if we have to)
set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  string(REPLACE "-MD" "-MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set_property(TARGET packer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
else()
    set_property(TARGET packer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()