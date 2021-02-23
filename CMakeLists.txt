cmake_minimum_required (VERSION 3.8)

## Project Definition and Options ##

project(EGL3)

option(ENABLE_CONSOLE "Use console subsystem" OFF)
option(ENABLE_TRACY "Use tracy" OFF)


## Config ##

set(CONFIG_VERSION_MAJOR 3)
set(CONFIG_VERSION_MINOR 0)
set(CONFIG_VERSION_PATCH 1)

# Get Git Info #
execute_process(COMMAND git log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

if ("${GIT_REV}" STREQUAL "")
    # No git info
    set(GIT_REV "fffffff")
    set(GIT_DIFF "")
    set(GIT_BRANCH "orphaned")
else()
    execute_process(
        COMMAND bash -c "git diff --quiet --exit-code || echo +"
        OUTPUT_VARIABLE GIT_DIFF)
    execute_process(
        COMMAND git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif()

set(CONFIG_VERSION_HASH ${GIT_REV}${GIT_DIFF})
set(CONFIG_VERSION_BRANCH ${GIT_BRANCH})

add_compile_definitions(
    CONFIG_VERSION_MAJOR=${CONFIG_VERSION_MAJOR}
    CONFIG_VERSION_MINOR=${CONFIG_VERSION_MINOR}
    CONFIG_VERSION_PATCH=${CONFIG_VERSION_PATCH}
    CONFIG_VERSION_HASH="${CONFIG_VERSION_HASH}"
    CONFIG_VERSION_BRANCH="${CONFIG_VERSION_BRANCH}"
    CONFIG_VERSION_SHORT="${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_PATCH}"
    CONFIG_VERSION_LONG="${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_PATCH}-${CONFIG_VERSION_HASH}++EGL3+${CONFIG_VERSION_BRANCH}"
)

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

add_dir_recursive(EGL3 src/disk)
add_dir_recursive(EGL3 src/modules)
add_dir_recursive(EGL3 src/storage)
add_dir_recursive(EGL3 src/utils)
add_dir_recursive(EGL3 src/web)
add_dir_recursive(EGL3 src/widgets)
add_file(EGL3 src/main.cpp)
add_file(EGL3 src/main.rc)
add_file(EGL3 src/main.manifest)
if (ENABLE_TRACY)
    add_file(EGL3 tracy/TracyClient.cpp)
endif()

add_dir_recursive(EGL3_SRV src/srv)


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
add_executable(EGL3_SRV ${EGL3_SRV_SOURCES})
install(TARGETS EGL3 EGL3_SRV DESTINATION bin)


## Dependencies ##

# Tracy #
if (ENABLE_TRACY)
    target_include_directories(EGL3 PRIVATE ${CMAKE_SOURCE_DIR}/tracy)
    target_compile_definitions(EGL3 PRIVATE TRACY_ENABLE TRACY_NO_EXIT)
endif()

# Http #
find_package(cpr CONFIG REQUIRED)
target_link_libraries(EGL3 cpr)

# WebSockets (XMPP) #
find_library(WS_TLS_LIBRARY mbedtls REQUIRED)
get_filename_component(WS_TLS_LIBRARY ${WS_TLS_LIBRARY} DIRECTORY)
target_link_libraries(EGL3 ${WS_TLS_LIBRARY}/mbedtls.lib ${WS_TLS_LIBRARY}/mbedcrypto.lib ${WS_TLS_LIBRARY}/mbedx509.lib)
find_library(WS_LIBRARY ixwebsocket REQUIRED)
target_link_libraries(EGL3 ${WS_LIBRARY})

# XML (XMPP) #
# This fix is required for permissive compilers (pre MSVC 16.8, and all others) - https://stackoverflow.com/a/14114300
find_path(RAPIDXML_INCLUDE_DIRS "rapidxml/rapidxml.hpp" REQUIRED)
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
find_library(CAIROMM_LIBRARY cairomm-1.0 REQUIRED)
target_link_libraries(EGL3 ${CAIROMM_LIBRARY})
find_library(GDKMM_LIBRARY gdkmm REQUIRED)
target_link_libraries(EGL3 ${GDKMM_LIBRARY})
find_library(GIOMM_LIBRARY giomm REQUIRED)
target_link_libraries(EGL3 ${GIOMM_LIBRARY})
find_library(PANGOMM_LIBRARY pangomm REQUIRED)
target_link_libraries(EGL3 ${PANGOMM_LIBRARY})
find_library(FONTCONFIG_LIBRARY fontconfig REQUIRED)
target_link_libraries(EGL3 ${FONTCONFIG_LIBRARY})
find_library(PANGOFT_LIBRARY pangoft2-1.0 REQUIRED)
target_link_libraries(EGL3 ${PANGOFT_LIBRARY})
find_library(PANGOCAIRO_LIBRARY pangocairo-1.0 REQUIRED)
target_link_libraries(EGL3 ${PANGOCAIRO_LIBRARY})

find_library(ATKMM_LIBRARY atkmm REQUIRED)
target_link_libraries(EGL3 ${ATKMM_LIBRARY})
find_package(unofficial-glib CONFIG REQUIRED)
target_link_libraries(EGL3 unofficial::glib::gio unofficial::glib::glib unofficial::glib::gmodule unofficial::glib::gobject)
find_library(GLIBMM_LIBRARY glibmm-2.0 REQUIRED)
target_link_libraries(EGL3 ${GLIBMM_LIBRARY})
find_library(GDK_LIBRARY gdk-3.0 REQUIRED)
target_link_libraries(EGL3 ${GDK_LIBRARY})
find_library(GTKMM_LIBRARY gtkmm REQUIRED)
target_link_libraries(EGL3 ${GTKMM_LIBRARY})
find_library(SIGC_LIBRARY sigc-2.0 REQUIRED)
target_link_libraries(EGL3 ${SIGC_LIBRARY})

find_path(GTKMM_INCLUDE_DIRS gtkmm.h REQUIRED)
target_include_directories(EGL3 SYSTEM PRIVATE ${GTKMM_INCLUDE_DIRS})


## Properties ##

# C++20 #
set_property(TARGET EGL3 PROPERTY CXX_STANDARD 20)
set_property(TARGET EGL3_SRV PROPERTY CXX_STANDARD 20)

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3 PRIVATE /Zi)
   target_compile_options(EGL3_SRV PRIVATE /Zi)
   set_target_properties(EGL3 EGL3_SRV PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
   )
endif()

# gtkmm causes a ton of C4250 warnings (only on MSVC) #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(EGL3 PRIVATE /wd4250)
endif()

# gtkmm assumes that the compiler is msvc if it detects MSC #
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(EGL3 PRIVATE SIGC_GCC_TEMPLATE_SPECIALIZATION_OPERATOR_OVERLOAD)
endif()
