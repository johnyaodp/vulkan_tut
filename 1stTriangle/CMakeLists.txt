#[[
  SPDX-FileCopyrightText: (c) Copyright Datapath Limited 2021, all rights reserved
  SPDX-License-Identifier: None
]]
#
# vulkan_composition Executable
#

add_executable(1stTriangle "")

target_sources(
   1stTriangle
   PRIVATE
      main.cpp)
      
#[[target_link_libraries(
   1stTriangle
   PRIVATE
      glfw) ]]

