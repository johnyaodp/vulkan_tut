#pragma once

#include <vector>
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

extern const bool enableValidationLayers;

void DestroyDebugUtilsMessengerEXT(
   VkInstance instance,
   VkDebugUtilsMessengerEXT debugMessenger,
   const VkAllocationCallbacks* pAllocator);


struct QueueFamilyIndices {
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete() {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }
};

struct SwapChainSupportDetails {
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
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
      createSurface();
      pickPhysicalDevice();
      createLogicalDevice();
      createSwapChain();
      createImageViews();
   }

   void mainLoop() {
      while (!glfwWindowShouldClose(window)) {
         glfwPollEvents();
      }
   }

   void cleanup() {

      for (auto imageView : swapChainImageViews) {
         vkDestroyImageView(device, imageView, nullptr);
      }

      vkDestroyDevice(device, nullptr);

      if (enableValidationLayers) {
         DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
      }

      vkDestroySurfaceKHR(instance, surface, nullptr);

      vkDestroyInstance(instance, nullptr);

      glfwDestroyWindow(window);

      glfwTerminate();

   }

   //____________________________________________________________________________
   void createImageViews();

   void createSurface();

   void createLogicalDevice();

   void createSwapChain();
   VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
   VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
   VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
   SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);


   void pickPhysicalDevice();

   virtual bool isDeviceSuitable(VkPhysicalDevice device);
   virtual bool checkDeviceExtensionSupport(VkPhysicalDevice device);

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

   VkSurfaceKHR surface;
   VkQueue presentQueue;

   VkSwapchainKHR swapChain;
   std::vector<VkImage> swapChainImages;
   VkFormat swapChainImageFormat;
   VkExtent2D swapChainExtent;
   std::vector<VkImageView> swapChainImageViews;

};
