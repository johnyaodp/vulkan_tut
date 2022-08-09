#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2021, all rights reserved
  SPDX-License-Identifier: None
]]
#
# vulkan_composition Executable
#

add_executable(Triangle "")

target_compile_options( Triangle PRIVATE /wd4100 /wd4458 /wd4459)

target_sources(
   Triangle
   PRIVATE
      HelloTriangle.cpp
   )
      
target_link_libraries(
   Triangle
   PRIVATE
      "C:/GL/glfw/out/build/x64-Debug/src/glfw3.lib"
      "C:/VulkanSDK/1.3.216.0/Lib/vulkan-1.lib")

