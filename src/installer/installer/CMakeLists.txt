cmake_minimum_required (VERSION 3.18)

## Project Definition and Options ##

project(installer)

## Source Files ##

add_dir(installer .)
add_dir_recursive(installer ../backend)
add_file(installer ../../utils/KnownFolders.cpp)
add_file(installer ../../utils/Version.cpp)
add_file(installer installerApp.rc)
add_file(installer app.manifest)

ADD_VERSION_DEFS(../../utils/Version.cpp)
ADD_VERSION_DEFS(installerApp.rc)

set_property(
        SOURCE ../backend/Installer.cpp
        APPEND
        PROPERTY COMPILE_DEFINITIONS
        INSTALLER_VERSION="${INSTALLER_VERSION}"
    )


## Define executables ##

add_executable(installer WIN32 ${installer_SOURCES})


## Dependencies ##

# Http #
target_link_libraries(installer Winhttp.lib)

# LZ4 #
find_package(lz4 REQUIRED)
target_link_libraries(installer lz4::lz4)


## Properties ##

target_link_options(installer PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")

# C++20 #
set_property(TARGET installer PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"))
   target_compile_options(installer PRIVATE /Zi)
   set_target_properties(installer PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME installer
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
else()
    set_property(TARGET installer PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()