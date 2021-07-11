cmake_minimum_required (VERSION 3.8)

## Project Definition and Options ##

project(installer)

## Source Files ##

add_dir_recursive(installer gui)
add_file(installer backend/streams/LZ4DecompStream.cpp)
add_file(installer backend/streams/WebStream.cpp)
add_file(installer backend/Installer.cpp)
add_file(installer backend/VersionInfo.cpp)
add_file(installer backend/Unpacker.cpp)
add_file(installer gui/installerApp.rc)
add_file(installer gui/app.manifest)

add_dir_recursive(packer backend)
add_file(packer packer_main.cpp)

ADD_VERSION_DEFS(backend/streams/WebStream.cpp)
ADD_VERSION_DEFS(packer_main.cpp)
ADD_VERSION_DEFS(gui/installerApp.rc)


## Define executables ##

set(CMAKE_MFC_FLAG 1)
add_executable(installer WIN32 ${installer_SOURCES})

add_executable(packer ${packer_SOURCES})


## Dependencies ##

# Http #
target_link_libraries(installer Winhttp.lib)
target_link_libraries(packer Winhttp.lib)

# LZ4 #
find_package(lz4 REQUIRED)
target_link_libraries(installer lz4::lz4)
target_link_libraries(packer lz4::lz4)


## Properties ##

target_link_options(installer PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")

# C++20 #
set_property(TARGET installer PROPERTY CXX_STANDARD 20)
set_property(TARGET packer PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"))
   target_compile_options(installer PRIVATE /Zi)
   target_compile_options(packer PRIVATE /Zi)
   set_target_properties(installer PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME installer 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
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
    set_property(TARGET installer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
    set_property(TARGET packer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
else()
    set_property(TARGET installer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
    set_property(TARGET packer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()