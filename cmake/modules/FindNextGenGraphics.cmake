message("<FindNextGenGraphics.cmake>")


set(NextGenGraphics_DIR "G:/Projects/NextGenGraphics")
set(DATAPATH_PACKAGE_CACHE_DIRECTORY "G:/DatapathPackages/Windows-x64-Debug")


set(NextGenGraphics_FOUND TRUE)
set(NextGenGraphics_INCLUDE_DIRS "${NextGenGraphics_DIR}/libs")


include_directories("${NextGenGraphics_INCLUDE_DIRS}")
include_directories("${DATAPATH_PACKAGE_CACHE_DIRECTORY}/eternal-src/include")

set(NextGenGraphics_LIBRARIES "${NextGenGraphics_DIR}/out/build/x64-Debug/libs")
find_library(vulkan_utils 
   NAMES vulkan_utils 
   PATHS 
      ${NextGenGraphics_LIBRARIES}/vulkan_utils)

find_library(datapath_lib 
   NAMES datapath_lib
   PATHS 
      ${NextGenGraphics_LIBRARIES}/datapath )

message("NextGenGraphics_INCLUDE_DIRS ${NextGenGraphics_INCLUDE_DIRS}")
message("NextGenGraphics_LIBRARIES ${NextGenGraphics_LIBRARIES}")
message("vulkan_utils ${vulkan_utils}")
message("datapath_lib ${datapath_lib}")