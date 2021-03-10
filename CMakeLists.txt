cmake_minimum_required (VERSION 3.8)

## Project Definition and Options ##

project(EGL3)

option(ENABLE_CONSOLE "Use console subsystem" OFF)
option(ENABLE_TRACY "Use tracy" OFF)
option(ENABLE_INSTALLER "Build the installer installer (this must be separately due to different vcpkg triplets)" OFF)


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


## Subsystem Options ##

if (ENABLE_CONSOLE)
    set(SUBSYSTEM "")
    add_compile_definitions(USE_SUBSYSTEM_CONSOLE)
else()
    set(SUBSYSTEM "WIN32")
    add_compile_definitions(USE_SUBSYSTEM_WIN32)
endif()

if (ENABLE_INSTALLER)
    add_subdirectory(src/installer)
else()
    add_subdirectory(src)
    add_subdirectory(src/srv)
endif()