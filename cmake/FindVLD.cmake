find_path(VLD_INCLUDE_DIR vld.h
    PATHS
    "C:/Program Files (x86)/Visual Leak Detector/include"
    "C:/Program Files/Visual Leak Detector/include"
)

find_library(VLD_LIBRARY vld
    PATHS
    "C:/Program Files (x86)/Visual Leak Detector/lib/Win64"
    "C:/Program Files/Visual Leak Detector/lib/Win64"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VLD DEFAULT_MSG VLD_INCLUDE_DIR VLD_LIBRARY)

if(VLD_FOUND)
    set(VLD_INCLUDE_DIRS ${VLD_INCLUDE_DIR})
    set(VLD_LIBRARIES ${VLD_LIBRARY})
endif()