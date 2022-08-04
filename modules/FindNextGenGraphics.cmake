message("<FindNextGenGraphics.cmake>")


set(NextGenGraphics_DIR "G:/Projects/NextGenGraphics")


set(NextGenGraphics_FOUND TRUE)
set(NextGenGraphics_INCLUDE_DIRS "${NextGenGraphics_DIR}/libs")
set(NextGenGraphics_LIBRARIES TRUE)

set(DATAPATH_PACKAGE_CACHE_DIRECTORY "G:/DatapathPackages/Windows-x64-Debug")


include_directories("${NextGenGraphics_INCLUDE_DIRS}")
include_directories("${DATAPATH_PACKAGE_CACHE_DIRECTORY}/eternal-src/include")
