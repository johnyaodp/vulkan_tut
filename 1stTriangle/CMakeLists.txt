#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2021, all rights reserved
  SPDX-License-Identifier: None
]]
#
# vulkan_composition Executable
#
message(STATUS "Hello")

add_executable(Triangle "")

target_sources(
   Triangle
   PRIVATE
      HelloTriangle.h
      HelloTriangle.cpp
      main.cpp)
      
target_link_libraries(
   Triangle
   PRIVATE
      "C:/GL/glfw/out/build/x64-Debug/src/glfw3.lib"
      "C:/VulkanSDK/1.3.216.0/Lib/vulkan-1.lib")

