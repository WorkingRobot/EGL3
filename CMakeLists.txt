cmake_minimum_required (VERSION 3.8)

project(EGL3)

add_executable(EGL3
     "ntfs/mkntfs.c" "ntfs/attrdef.c" "ntfs/sd.c" "ntfs/utils.c" "ntfs/boot.c" "ntfs/security.c" "ntfs/getopt.c" "ntfs/attrib.c"  "ntfs/volume.c" "ntfs/logfile.c" "ntfs/bootsect.c" "ntfs/attrlist.c" "ntfs/lcnalloc.c" "ntfs/bitmap.c" "ntfs/device.c" "ntfs/dir.c" "ntfs/unistr.c" "ntfs/runlist.c" "ntfs/mst.c" "ntfs/cache.c" "ntfs/index.c" "ntfs/inode.c" "ntfs/logging.c" "ntfs/mft.c" "ntfs/win32_io.c" "ntfs/collate.c" "ntfs/winsd.c" "ntfs/misc.c"  "launcher/main.cpp"  "ntfs/win32_io_helper.cpp" "ntfs/support.h" "storage/install.h" "utils/stringex/user_functions.cpp" "utils/stringex/token_parser.cpp" "utils/stringex/expression_tree.cpp" "utils/stringex/stringex.h" "utils/stringex/stringex.cpp")

set_property(TARGET EGL3 PROPERTY CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(EGL3 PRIVATE /Zi)
    set_target_properties(EGL3 PROPERTIES
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
        COMPILE_PDB_NAME EGL3 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
endif()

target_include_directories(EGL3 PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\inc")
target_link_libraries(EGL3 PRIVATE "$ENV{ProgramFiles\(x86\)}\\WinSpd\\lib\\winspd-x64.lib" Rpcrt4)