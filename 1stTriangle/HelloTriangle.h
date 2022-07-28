#pragma once

#include <vector>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>


class HelloTriangleApplication {
public:
   void run();

private:
   void initWindow();

   void initVulkan();

   void mainLoop();

   void cleanup();

   void pickPhysicalDevice();

   void createInstance();
   auto checkValidationLayerSupport()
      -> bool;

   auto getRequiredExtensions()
      ->std::vector<const char*>;

   static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData);

   void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

   void setupDebugMessenger();

   // members
   GLFWwindow* window = nullptr;
   VkInstance instance = nullptr;
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
   VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};
