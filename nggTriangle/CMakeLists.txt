#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2021, all rights reserved
  SPDX-License-Identifier: None
]]
#
# vulkan_composition Executable
#

add_executable(NggTriangle "")

target_compile_options( NggTriangle PRIVATE /wd4458)

target_sources(
   NggTriangle
   PRIVATE
      vulkan_glfw_wrapper.h
      vulkan_glfw_wrapper.cpp
      3rdPartyLibImp.cpp
      main.cpp)


#link_directories("${NEXTGENGRAPHICS_LIB}")
link_directories("${GLFW_PACKAGE}/out/build/x64-Debug/src")

target_link_libraries(
   NggTriangle
   PRIVATE
      ${vulkan_utils}
      ${datapath_lib}
      "${GLFW_PACKAGE}/out/build/x64-Debug/src/glfw3.lib" )
