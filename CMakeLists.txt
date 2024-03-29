cmake_minimum_required (VERSION 3.18)
cmake_policy(SET CMP0091 NEW)

## Project Definition and Options ##

project(EGL3)

option(ENABLE_INSTALLER "Build the installer (this must be separate due to different vcpkg triplets)" OFF)
set(INSTALLER_VERSION "" CACHE STRING "Installer version to compile to")

## Config ##

set(CONFIG_VERSION_MAJOR 3)
set(CONFIG_VERSION_MINOR 0)
set(CONFIG_VERSION_PATCH 13)

# Get Git Info #
execute_process(COMMAND git -C ${CMAKE_SOURCE_DIR} log --pretty=format:'%h' -n 1
                OUTPUT_VARIABLE GIT_REV
                ERROR_QUIET)

if ("${GIT_REV}" STREQUAL "")
    # No git info
    set(GIT_REV "fffffff")
    set(GIT_DIFF "")
    set(GIT_BRANCH "orphaned")
else()
    execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} diff --quiet --exit-code
        RESULT_VARIABLE GIT_DIFF)
    if (GIT_DIFF EQUAL "0")
        set(GIT_DIFF "")
    else()
        set(GIT_DIFF "+")
    endif()
    execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH)

    string(STRIP "${GIT_REV}" GIT_REV)
    string(SUBSTRING "${GIT_REV}" 1 6 GIT_REV)
    string(STRIP "${GIT_DIFF}" GIT_DIFF)
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
endif()

set(CONFIG_VERSION_HASH ${GIT_REV}${GIT_DIFF})
set(CONFIG_VERSION_BRANCH ${GIT_BRANCH})
set(CONFIG_VERSION_SHORT ${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_PATCH})
set(CONFIG_VERSION_LONG ${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_PATCH}-${CONFIG_VERSION_HASH}++EGL3+${CONFIG_VERSION_BRANCH})

message("Compiling EGL3 for ${CONFIG_VERSION_LONG}")

function(ADD_VERSION_DEFS TARGET_FILE)
    set_property(
        SOURCE ${TARGET_FILE}
        APPEND
        PROPERTY COMPILE_DEFINITIONS
        CONFIG_VERSION_MAJOR=${CONFIG_VERSION_MAJOR}
        CONFIG_VERSION_MINOR=${CONFIG_VERSION_MINOR}
        CONFIG_VERSION_PATCH=${CONFIG_VERSION_PATCH}
        CONFIG_VERSION_HASH="${CONFIG_VERSION_HASH}"
        CONFIG_VERSION_BRANCH="${CONFIG_VERSION_BRANCH}"
        CONFIG_VERSION_SHORT="${CONFIG_VERSION_SHORT}"
        CONFIG_VERSION_LONG="${CONFIG_VERSION_LONG}"
    )
endfunction()


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

function(REMOVE_FILE PROJECT TARGET_FILE)
    list(REMOVE_ITEM ${PROJECT}_SOURCES ${TARGET_FILE})
    set(${PROJECT}_SOURCES ${${PROJECT}_SOURCES} PARENT_SCOPE)
endfunction()


## Add projects ##

if (ENABLE_INSTALLER)
    if (INSTALLER_VERSION STREQUAL "")
        set(INSTALLER_VERSION ${CONFIG_VERSION_LONG})
    endif()
    add_subdirectory(src/installer/installer)
    add_subdirectory(src/installer/packer)
else()
    add_subdirectory(libstrophe)
    add_subdirectory(WinToast)
    add_subdirectory(src)
    add_subdirectory(src/srv)
endif()


## Define install targets ##

if (NOT ENABLE_INSTALLER)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
    install(
        TARGETS EGL3 EGL3_SRV
        RUNTIME DESTINATION "."
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
    )

    install(CODE [[
        file(GLOB_RECURSE DLL_DEPS "${CMAKE_BINARY_DIR}/*.dll")
        foreach(_file ${DLL_DEPS})
            file(INSTALL
                DESTINATION ${CMAKE_INSTALL_PREFIX}
                TYPE SHARED_LIBRARY
                FILES "${_file}"
            )
        endforeach()
    ]])

    file(GLOB EXTRA_DEPS "${CMAKE_SOURCE_DIR}/extras/*")
    foreach(EXTRA_DEP ${EXTRA_DEPS})
        if(IS_DIRECTORY ${EXTRA_DEP})
            if (NOT ${EXTRA_DEP} MATCHES "extras/share$")
                install(
                    DIRECTORY ${EXTRA_DEP}
                    DESTINATION "."
                    CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                )
            endif()
        else()
            install(
                FILES ${EXTRA_DEP}
                DESTINATION "."
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            )
        endif()
    endforeach()
endif()