#pragma once

#include <vector>
#include <optional>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

extern const bool enableValidationLayers;

void DestroyDebugUtilsMessengerEXT(
   VkInstance instance,
   VkDebugUtilsMessengerEXT debugMessenger,
   const VkAllocationCallbacks* pAllocator);


struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;

   bool isComplete() {
      return graphicsFamily.has_value();
   }
};


class HelloTriangleApplication {
public:
   virtual void run() {
      initWindow();
      initVulkan();
      mainLoop();
      cleanup();
   }

private:
   void initWindow();

   void initVulkan() {
      createInstance();
      setupDebugMessenger();
      pickPhysicalDevice();
      createLogicalDevice();
   }

   void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
         glfwPollEvents();
      }
   }

   void cleanup() {
      vkDestroyDevice(device, nullptr);

      if (enableValidationLayers) {
         DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
      }

      vkDestroyInstance(instance, nullptr);

      glfwDestroyWindow(window);

      glfwTerminate();

   }

   //____________________________________________________________________________

   void createLogicalDevice();

   void pickPhysicalDevice();

   virtual bool isDeviceSuitable(VkPhysicalDevice device);

   auto findQueueFamilies(VkPhysicalDevice device) -> QueueFamilyIndices;

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
   VkDevice device;

   VkQueue graphicsQueue;
};
