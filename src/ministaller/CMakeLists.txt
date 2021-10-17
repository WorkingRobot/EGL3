cmake_minimum_required (VERSION 3.18)

## Project Definition and Options ##

project(ministaller)

## Source Files ##

add_dir(ministaller .)
add_file(ministaller backend/Internal.cpp)
add_file(ministaller backend/Installer.cpp)
add_file(ministaller backend/Registry.cpp)
add_file(ministaller version.rc)
add_file(ministaller dialog.rc)
add_file(ministaller app.manifest)
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/decompress/zstd_decompress.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/decompress/zstd_decompress_block.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/decompress/huf_decompress.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/common/entropy_common.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/common/fse_decompress.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/common/error_private.c")
add_file(ministaller "${CMAKE_SOURCE_DIR}/zstd/lib/common/zstd_common.c")

function(ADD_DEFINE_FILE FILE VAL)
    set_property(
        SOURCE ${FILE}
        APPEND
        PROPERTY COMPILE_DEFINITIONS
        ${VAL}
    )
endfunction()

function(ADD_DEFINE_DIR DIR VAL)
    aux_source_directory(${DIR} DIR_SOURCES)
    foreach(FILE ${DIR_SOURCES})
        ADD_DEFINE_FILE(${FILE} ${VAL})
    endforeach()
endfunction()

add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/decompress" ZSTD_NO_UNUSED_FUNCTIONS)
add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/common" ZSTD_NO_UNUSED_FUNCTIONS)
add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/decompress" HUF_FORCE_DECOMPRESS_X1=1)
add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/common" HUF_FORCE_DECOMPRESS_X1=1)
add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/decompress" ZSTD_FORCE_DECOMPRESS_SHORT=1)
add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/common" ZSTD_FORCE_DECOMPRESS_SHORT=1)
#add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/decompress" ZSTD_NO_INLINE=1)
#add_define_dir("${CMAKE_SOURCE_DIR}/zstd/lib/common" ZSTD_NO_INLINE=1)

ADD_VERSION_DEFS(version.rc)


## Define executables ##

add_executable(ministaller WIN32 ${ministaller_SOURCES})


## Dependencies ##

# GUI #
target_link_libraries(ministaller ComCtl32.lib Shlwapi.lib)

# Http #
target_link_libraries(ministaller Winhttp.lib)

# Zstd #
target_include_directories(ministaller PRIVATE "${CMAKE_SOURCE_DIR}/zstd/lib")


## Properties ##

# C++20 #
set_property(TARGET ministaller PROPERTY CXX_STANDARD 23)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"))
   target_compile_options(ministaller PRIVATE /Zi)
   set_target_properties(ministaller PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME ministaller
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()

function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB /ENTRY:Main /STACK:0x100000,0x100000 /MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS- /Gs131072 /EHs-")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS- /Gs131072 /EHs-")
  string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  #set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /Ob0 /Od")
  #set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /Ob0 /Od")

  target_compile_definitions(ministaller PUBLIC _HAS_EXCEPTIONS=0)
endif()

dump_cmake_variables()