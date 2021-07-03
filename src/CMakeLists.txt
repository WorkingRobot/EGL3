cmake_minimum_required (VERSION 3.18)

## Project Definition and Options ##

project(EGL3App)


## Source Files ##

add_dir_recursive(EGL3 disk)
add_dir_recursive(EGL3 modules)
add_dir_recursive(EGL3 storage)
add_dir_recursive(EGL3 srv/pipe/base)
add_dir_recursive(EGL3 srv/pipe/client)
add_dir_recursive(EGL3 utils)
add_dir_recursive(EGL3 web)
add_dir_recursive(EGL3 widgets)
add_file(EGL3 main.cpp)
add_file(EGL3 main.rc)
add_file(EGL3 main.manifest)

ADD_VERSION_DEFS(utils/Version.cpp)
ADD_VERSION_DEFS(main.rc)


## Define EGL3 executable ##

add_executable(EGL3 WIN32 ${EGL3_SOURCES})


## Dependencies ##

# Http #
find_package(cpr CONFIG REQUIRED)
target_link_libraries(EGL3 cpr)

# WebSockets #
find_library(WS_TLS_LIBRARY mbedtls REQUIRED)
get_filename_component(WS_TLS_LIBRARY ${WS_TLS_LIBRARY} DIRECTORY)
target_link_libraries(EGL3 ${WS_TLS_LIBRARY}/mbedtls.lib ${WS_TLS_LIBRARY}/mbedcrypto.lib ${WS_TLS_LIBRARY}/mbedx509.lib)
find_library(WS_LIBRARY ixwebsocket REQUIRED)
target_link_libraries(EGL3 ${WS_LIBRARY})

# XMPP #
target_link_libraries(EGL3 libstrophe)

# JSON #
find_package(RapidJSON CONFIG REQUIRED)
target_include_directories(EGL3 PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

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

# Add .pdb for release builds #
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3 PRIVATE /Zi)
   set_target_properties(EGL3 PROPERTIES
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
