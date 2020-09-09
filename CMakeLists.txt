cmake_minimum_required (VERSION 3.8)

project(EGL3)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELEASE
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

function(ADD_DIR PROJECT TARGET_DIR)
    aux_source_directory(${TARGET_DIR} TARGET_DIR_SOURCES)
    list(APPEND ${PROJECT}_SOURCES ${TARGET_DIR_SOURCES})
    set(${PROJECT}_SOURCES ${${PROJECT}_SOURCES} PARENT_SCOPE)
endfunction()

add_dir(EGL3 launcher)
add_dir(EGL3 ntfs)
add_dir(EGL3 storage)
add_dir(EGL3 utils)
add_dir(EGL3 utils/base64)
add_dir(EGL3 utils/stringex)
add_dir(EGL3 web/epic)
add_dir(EGL3 web/epic/auth)

add_executable(EGL3 ${EGL3_SOURCES})

set_property(TARGET EGL3 PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3 PRIVATE /Zi)
    set_target_properties(EGL3 PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()

find_package(cpr CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

target_include_directories(EGL3 PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\inc" ${RAPIDJSON_INCLUDE_DIRS})
target_link_libraries(EGL3 PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\lib\\winspd-x64.lib" cpr)