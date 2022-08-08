#pragma once

#include <vulkan_utils/vulkan_utils.hpp>
#include <optional>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

using namespace datapath::vulkan_utils;

extern const bool enableValidationLayers;

void DestroyDebugUtilsMessengerEXT(
   VkInstance instance,
   VkDebugUtilsMessengerEXT debugMessenger,
   const VkAllocationCallbacks* pAllocator );

struct QueueFamilyIndices
{
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete()
   {
      return graphicsFamily.has_value() && presentFamily.has_value();
   }

   void reset()
   {
      graphicsFamily.reset();
      presentFamily.reset();
   }
};

struct SwapChainSupportDetails
{
   VkSurfaceCapabilitiesKHR capabilities;
   std::vector<VkSurfaceFormatKHR> formats;
   std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex
{
   glm::vec2 pos;
   glm::vec3 color;

   static
   auto getBindingDescription()
      -> VkVertexInputBindingDescription
   {
      VkVertexInputBindingDescription binding_description{};
      binding_description.binding = 0;
      binding_description.stride = sizeof( Vertex );
      binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

      return binding_description;
   }

   static
   auto getAttributeDescriptions()
      -> std::array<
         VkVertexInputAttributeDescription,
         2>
   {
      std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

      attribute_descriptions[0].binding = 0;
      attribute_descriptions[0].location = 0;
      attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
      attribute_descriptions[0].offset = offsetof( Vertex, pos );

      attribute_descriptions[1].binding = 0;
      attribute_descriptions[1].location = 1;
      attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
      attribute_descriptions[1].offset = offsetof( Vertex, color );

      return attribute_descriptions;
   }
};


class vulkan_wrapper
{
public:
   virtual
   void run(
      const char* appname,
      uint32_t width,
      uint32_t height )
   {
      if ( !appname || !*appname )
      {
         appname = "Vulkan";
      }

      init_window( appname, width, height );
      init_vulkan( appname );
      main_loop();
      cleanup();
   }

   virtual ~vulkan_wrapper()
   {
      destroy_window();
   }

protected:

private:
   static constexpr int max_frames_in_flight = 2;

   // Members
   GLFWwindow* window{};

   datapath::vulkan_utils::VkDebugUtilsMessengerEXT_resource_t debug_messenger;

   datapath::vulkan_utils::vulkan_engine_t vulkan_engine{};

   datapath::vulkan_utils::dispatcher_t initial_dispatcher{
      datapath::vulkan_utils::vulkan_engine_t::initialise_initial_dispatcher() };

   datapath::vulkan_utils::VkSurfaceKHR_resource_t surface;

   std::optional<datapath::vulkan_utils::physical_device_wrapper_t> physical_device;
   std::shared_ptr<const datapath::vulkan_utils::device_dispatcher_t> logical_device;
   datapath::vulkan_utils::VkSwapchainKHR_resource_t swapchain;

   std::optional<datapath::vulkan_utils::queue_wrapper_t> graphics_queue{};
   std::optional<datapath::vulkan_utils::queue_wrapper_t> present_queue{};

   std::vector<VkImage> swapchain_images;
   VkFormat swapchain_image_format{};
   VkExtent2D swapchain_extent{};

   std::vector<datapath::vulkan_utils::VkImageView_resource_t> swapchain_image_views;

   VkPipelineLayout_resource_t pipeline_layout;

   VkRenderPass_resource_t render_pass;

   std::vector<datapath::vulkan_utils::VkPipeline_resource_t> graphics_pipeline;

   std::vector<datapath::vulkan_utils::VkFramebuffer_resource_t> swapchain_framebuffers;

   VkCommandPool_resource_shared_t command_pool;
   std::vector<command_buffer_wrapper_t> command_buffer;

   VkBuffer_resource_t vertex_buffer;
   VkDeviceMemory_resource_t vertex_buffer_memory;

   std::vector<datapath::vulkan_utils::VkSemaphore_resource_t> image_available_semaphores;
   std::vector<datapath::vulkan_utils::VkSemaphore_resource_t> render_finished_semaphores;
   std::vector<datapath::vulkan_utils::VkFence_resource_t> in_flight_fences;
   uint32_t current_frame = 0;

   bool framebuffer_resized{ false };

#ifdef NDEBUG
   const bool enableValidationLayers = false;
#else
   const bool enableValidationLayers = true;
#endif

   // local functions

   virtual
   void init_vulkan(
      const char* appname );

   virtual
   void cleanup();

   virtual
   void main_loop()
   {
      while ( !glfwWindowShouldClose( window ) )
      {
         glfwPollEvents();
         draw_frame();
      }

      if ( logical_device->vkDeviceWaitIdle() != VK_SUCCESS )
      {
         throw std::runtime_error( "failed to wait for idle!" );
      }
   };

   // Windows related code
   virtual
   void init_window(
      const char* title,
      uint32_t width,
      uint32_t height );

   void destroy_window()
   {
      if ( window )
      {
         glfwDestroyWindow( window );
         window = nullptr;

         glfwTerminate();
      }
   }

   // Platform agnostic functions
   void create_instance(
      const char* appname );
   auto get_required_instance_extensions()
      -> datapath::vulkan_utils::vk_used_extensions_t;

   void create_surface();

   void pick_physical_device();
   auto is_device_suitable(
      const physical_device_wrapper_t& device )
      -> bool;
   auto find_queue_families(
      const physical_device_wrapper_t& device )
      -> QueueFamilyIndices;
   auto check_device_extension_support(
      const physical_device_wrapper_t& device )
      -> bool;

   void create_logical_device();

   auto check_validation_layer_support()
      -> bool;
   auto get_required_extensions()
      -> std::vector<const char*>;

   void create_swap_chain();
   auto query_swapchain_support()
      -> SwapChainSupportDetails;
   auto choose_swap_surface_format(
      const std::vector<VkSurfaceFormatKHR>& availableFormats )
      -> VkSurfaceFormatKHR;
   auto choose_swap_present_mode(
      const std::vector<VkPresentModeKHR>& availablePresentModes )
      -> VkPresentModeKHR;
   auto choose_swap_extent(
      const VkSurfaceCapabilitiesKHR& capabilities )
      -> VkExtent2D;

   void create_image_views();

   virtual
   void create_graphics_pipeline();

   void create_render_pass();

   void create_framebuffers();

   void create_command_pool();

   void create_command_buffer();

   // Loading shader
   static
   auto read_file(
      const std::string& filename )
      -> std::vector<char>;

   auto create_shader_module(
      const std::vector<char>& code )
      -> VkShaderModule_resource_t;

   //
   void create_vertex_buffer();
   auto find_memory_type(
      uint32_t typeFilter,
      VkMemoryPropertyFlags properties )
      -> uint32_t;

   // Drawing
   // command buffer
   void record_command_buffer(
      command_buffer_wrapper_t& command_buffer,
      uint32_t imageIndex );

   virtual
   void draw_frame();

   void create_sync_objects();

   void recreate_swapchain();
   void cleanup_swapchain();

   //
   static
   void framebuffer_resize_callback(
      GLFWwindow* window,
      int width,
      int height );

   // Debug messenger
   void setup_debug_messenger();
   void destroy_debug_messenger();
   void populate_debug_messenger_create_info(
      VkDebugUtilsMessengerCreateInfoEXT& createInfo );
   static
   VKAPI_ATTR auto VKAPI_CALL debug_callback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData )
      -> VkBool32;
};
