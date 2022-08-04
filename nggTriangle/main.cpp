#include "vulkan_tutorial.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };


int main()
{
   vulkan_tutorial app;

   try
   {
      app.run( "Vulkan DP Tutorial", WIDTH, HEIGHT );
   }
   catch ( const std::exception& e )
   {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
