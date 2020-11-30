cmake_minimum_required (VERSION 3.8)

project(EGL3)

option(ENABLE_CONSOLE "Use console subsystem" OFF)

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
install(TARGETS EGL3 DESTINATION bin)


## Dependencies ##

# Http #
find_package(cpr CONFIG REQUIRED)
target_link_libraries(EGL3 cpr)

# WebSockets (XMPP) #
find_library(WS_TLS_LIBRARY mbedtls)
get_filename_component(WS_TLS_LIBRARY ${WS_TLS_LIBRARY} DIRECTORY)
target_link_libraries(EGL3 ${WS_TLS_LIBRARY}/mbedtls.lib ${WS_TLS_LIBRARY}/mbedcrypto.lib ${WS_TLS_LIBRARY}/mbedx509.lib)
find_library(WS_LIBRARY ixwebsocket)
target_link_libraries(EGL3 ${WS_LIBRARY})

# XML (XMPP) #
find_path(RAPIDXML_INCLUDE_DIRS "rapidxml/rapidxml.hpp")
target_include_directories(EGL3 PRIVATE ${RAPIDXML_INCLUDE_DIRS})

# JSON #
find_package(RapidJSON CONFIG REQUIRED)
target_include_directories(EGL3 PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

# WinSpd #
target_include_directories(EGL3 PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\inc")
target_link_libraries(EGL3 "$ENV{ProgramFiles\(x86\)}\\WinSpd\\lib\\winspd-x64.lib")

# ZenReader #
add_subdirectory(ZenReader)
target_link_libraries(EGL3 ZenReader)
target_include_directories(EGL3 PRIVATE ${ZenReader_SOURCE_DIR}/src)

# NtDll (Used for mmio) #
target_link_libraries(EGL3 "${CMAKE_SOURCE_DIR}\\src\\utils\\mmio\\ntdll64.lib")

# GUI #

# These aren't really needed for compilation and the libraries automatically
# include the dlls needed into the output anyway
find_package(unofficial-cairo CONFIG REQUIRED)
target_link_libraries(EGL3 unofficial::cairo::cairo unofficial::cairo::cairo-gobject)
find_library(CAIROMM_LIBRARY cairomm-1.0)
target_link_libraries(EGL3 ${CAIROMM_LIBRARY})
find_library(GDKMM_LIBRARY gdkmm)
target_link_libraries(EGL3 ${GDKMM_LIBRARY})
find_library(GIOMM_LIBRARY giomm)
target_link_libraries(EGL3 ${GIOMM_LIBRARY})
find_library(PANGOMM_LIBRARY pangomm)
target_link_libraries(EGL3 ${PANGOMM_LIBRARY})

find_library(ATKMM_LIBRARY atkmm)
target_link_libraries(EGL3 ${ATKMM_LIBRARY})
find_library(GLIB2_LIBRARY glib-2.0)
target_link_libraries(EGL3 ${GLIB2_LIBRARY})
find_library(GLIBMM_LIBRARY glibmm)
target_link_libraries(EGL3 ${GLIBMM_LIBRARY})
find_library(GTKMM_LIBRARY gtkmm)
target_link_libraries(EGL3 ${GTKMM_LIBRARY})
find_library(SIGC_LIBRARY sigc-2.0)
target_link_libraries(EGL3 ${SIGC_LIBRARY})

find_path(GTKMM_INCLUDE_DIRS gtkmm.h)
target_include_directories(EGL3 PRIVATE ${GTKMM_INCLUDE_DIRS})

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
