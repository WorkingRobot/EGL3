cmake_minimum_required (VERSION 3.8)

project(EGL3Port)

add_executable(EGL3Port
     "mkntfs.c" "attrdef.c" "sd.c" "utils.c" "boot.c" "security.c" "getopt.c" "attrib.c" "misc.c" "volume.c" "logfile.c" "bootsect.c" "attrlist.c" "lcnalloc.c" "bitmap.c" "compress.c" "device.c" "dir.c" "unistr.c" "runlist.c" "mst.c" "cache.c" "index.c" "inode.c" "logging.c" "mft.c" "reparse.c" "win32_io.c" "acls.c" "collate.c" "object_id.c")

set_property(TARGET EGL3Port PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3Port PRIVATE /Zi)
    set_target_properties(EGL3Port PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3Port 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()

target_link_libraries(EGL3Port ntdll)