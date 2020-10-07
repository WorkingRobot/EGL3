cmake_minimum_required (VERSION 3.8)

project(EGL3)

option(ENABLE_CONSOLE "Use console subsystem" OFF)

## Use /MT instead of /MD ##
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


## Dependencies ##

# Http #
find_package(cpr CONFIG REQUIRED)
link_libraries(cpr)

# JSON #
find_package(RapidJSON CONFIG REQUIRED)
include_directories(${RAPIDJSON_INCLUDE_DIRS})

# WinSpd #
include_directories("$ENV{ProgramFiles\(x86\)}\\WinSpd\\inc")
link_libraries("$ENV{ProgramFiles\(x86\)}\\WinSpd\\lib\\winspd-x64.lib")

# NtDll (Used for mmio) #
link_libraries("${CMAKE_SOURCE_DIR}\\src\\utils\\mmio\\ntdll64.lib")

# GUI #

# These aren't really needed for compilation and the libraries automatically
# include the dlls needed into the output anyway
find_library(CAIROMM_LIBRARY cairomm-1.0)
link_libraries(${CAIROMM_LIBRARY})
find_library(GDKMM_LIBRARY gdkmm)
link_libraries(${GDKMM_LIBRARY})
find_library(GIOMM_LIBRARY giomm)
link_libraries(${GIOMM_LIBRARY})
find_library(PANGOMM_LIBRARY pangomm)
link_libraries(${PANGOMM_LIBRARY})

find_library(ATKMM_LIBRARY atkmm)
link_libraries(${ATKMM_LIBRARY})
find_library(GLIB2_LIBRARY glib-2.0)
link_libraries(${GLIB2_LIBRARY})
find_library(GLIBMM_LIBRARY glibmm)
link_libraries(${GLIBMM_LIBRARY})
find_library(GTKMM_LIBRARY gtkmm)
link_libraries(${GTKMM_LIBRARY})
find_library(SIGC_LIBRARY sigc-2.0)
link_libraries(${SIGC_LIBRARY})

find_path(GTKMM_INCLUDE_DIRS gtkmm.h)
include_directories(${GTKMM_INCLUDE_DIRS})


## Source Files ##

function(ADD_DIR PROJECT TARGET_DIR)
    aux_source_directory(${TARGET_DIR} TARGET_DIR_SOURCES)
    list(APPEND ${PROJECT}_SOURCES ${TARGET_DIR_SOURCES})
    set(${PROJECT}_SOURCES ${${PROJECT}_SOURCES} PARENT_SCOPE)
endfunction()

function(ADD_FILE PROJECT TARGET_FILE)
    list(APPEND ${PROJECT}_SOURCES ${TARGET_FILE})
    set(${PROJECT}_SOURCES ${${PROJECT}_SOURCES} PARENT_SCOPE)
endfunction()

function(ADD_DIR_RECURSIVE PROJECT TARGET_DIR)
    file(GLOB_RECURSE CHILDREN ${TARGET_DIR}/*)
    foreach(CHILDPATH ${CHILDREN})
        get_filename_component(CHILD ${CHILDPATH} DIRECTORY)
        if(IS_DIRECTORY ${CHILD})
            list(APPEND DIRLIST ${CHILD})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES DIRLIST)

    foreach(DIRPATH ${DIRLIST})
        add_dir(${PROJECT} ${DIRPATH})
    endforeach()
    set(${PROJECT}_SOURCES ${${PROJECT}_SOURCES} PARENT_SCOPE)
endfunction()

add_dir_recursive(EGL3 src)
add_file(EGL3 src/main.rc)


## Subsystem Options ##

if (ENABLE_CONSOLE)
    set(SUBSYSTEM "")
    add_compile_definitions(USE_SUBSYSTEM_CONSOLE)
else()
    set(SUBSYSTEM "WIN32")
    add_compile_definitions(USE_SUBSYSTEM_WIN32)
endif()


## Define EGL3 executable ##

add_executable(EGL3 ${SUBSYSTEM} ${EGL3_SOURCES})


## Properties ##

# C++20 #
set_property(TARGET EGL3 PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3 PRIVATE /Zi)
   set_target_properties(EGL3 PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()
