#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2021, all rights reserved
  SPDX-License-Identifier: None
]]
#
# vulkan_composition Executable
#

add_executable(NggTriangle "")

target_sources(
   NggTriangle
   PRIVATE
      main.cpp)
      
target_link_libraries(
   NggTriangle
   PRIVATE
      "C:/GL/glfw/out/build/x64-Debug/src/glfw3.lib"
      "C:/VulkanSDK/1.3.216.0/Lib/vulkan-1.lib")

