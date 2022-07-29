#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2020, all rights reserved
  SPDX-License-Identifier: None

  CMakeList.txt : CMake project for the Datapath Next Generation Graphics project.
  
  See project setup documentation on Sharepoint for details of build environment dependencies.
  
  General Dependencies:
    > Doxygen - Use the installer from the doxygen website.
    > Ensure the Python 3.x environment is installed in your Visual Studio installation.
    > Sphinx
       \-> Use pip via Visual Studio Menus: Tools -> Python -> Python Environments
           Select the Python Environment (latest v3 Python), use the list view, change is from
           "Overview" to "Packages (PyPI)".  Type "sphinx" without the quotes in the edit bot
           and click on "Run Command: pip install sphinx".  Let it install.  Done.
    > Sphinx read the docs theme:
       \-> Same process as for sphinx installation except the package is called "sphinx_rtd_theme".
    > Breathe:
       \-> This is also a Python package, same process as Sphinx installation with the package name
            of "breathe"
    > Visual Studio should be configured with a minimum of:
       \-> Desktop Development with C++
       \-> Python Development
       \-> Individual Components:
            \-> GIT for Windows
            \-> C++ CMake tools for Windows

]]

#[[
  set(PROJECT_NAME MyProject)
  
  file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.h")
  file(GLOB SOURCES "*.cpp")
  
  add_library(${PROJECT_NAME} ${SOURCES} ${HEADER_FILES})
  
  target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/include" PUBLIC "${CMAKE_SOURCE_DIR}/extern/SDL2/include")
  
  target_link_libraries(${PROJECT_NAME} PRIVATE SDL2main SDL2-static)
  
  set_target_properties( ${PROJECT_NAME}
      PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
  )
]]

cmake_minimum_required(VERSION 3.15)

#___________________________________________________________________________________________________
project(VulkanSelfTraining
   VERSION 0.1
   DESCRIPTION "Vulkan self training for Next Generation Graphics Project"
   LANGUAGES CXX C)

#___________________________________________________________________________________________________
#option(BUILD_APPS "Builds the apps subfolder" ON)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
   #target_compile_options(
   #   ${_target}
   #   PRIVATE
   #      /W4
   #      /WX)
 else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("C:/VulkanSDK/1.3.216.0/Include")
include_directories("C:/GL/glfw/include")
include_directories("C:/GL/glm")

#___________________________________________________________________________________________________
add_subdirectory(1stTriangle)
add_subdirectory(nggTriangle)
