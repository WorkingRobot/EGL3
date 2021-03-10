cmake_minimum_required (VERSION 3.8)

## Project Definition and Options ##

project(installer)


## Source Files ##

add_dir_recursive(installer backend)
add_dir_recursive(installer gui)
add_file(installer gui/installerApp.rc)
add_file(installer gui/app.manifest)


## Define installer executable ##

add_definitions(-D_AFXDLL)
set(CMAKE_MFC_FLAG 1)
add_executable(installer WIN32 ${installer_SOURCES})


## Dependencies ##

# OpenSSL #
find_package(OpenSSL REQUIRED)
target_link_libraries(installer Crypt32 OpenSSL::SSL OpenSSL::Crypto)

# Zlib #
find_package(ZLIB REQUIRED)
target_link_libraries(installer ZLIB::ZLIB)

# LZ4 #
find_package(lz4 REQUIRED)
target_link_libraries(installer lz4::lz4)


## Properties ##

target_link_options(installer PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")

# C++20 #
set_property(TARGET installer PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(installer PRIVATE /Zi)
   set_target_properties(installer PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME installer 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()