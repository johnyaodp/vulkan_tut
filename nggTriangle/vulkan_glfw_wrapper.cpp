#include "vulkan_glfw_wrapper.h"

using namespace datapath::vulkan_utils;

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>


#pragma warning( disable : 4458 )


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#define VERTEX_ACTIVE 2
#if VERTEX_ACTIVE == 0
const std::vector<Vertex> vertices = {
   { { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
   { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
   { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } } };
#elif VERTEX_ACTIVE == 1
const std::vector<Vertex> vertices = {
   { { 0.0f, -0.5f }, { 1.0f, 1.0f, 1.0f } },
   { { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
   { { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } } };
#elif VERTEX_ACTIVE == 2
const std::vector<Vertex> vertices = {
   { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
   { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
   { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
   { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } } };
#endif

const std::vector<uint16_t> g_indices = { 0, 1, 2, 2, 3, 0 };

// Environment depdent code: Windows
void vulkan_wrapper::init_window(
   const char* title,
   uint32_t width,
   uint32_t height )
{
   glfwInit();

   glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
   // glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

   window = glfwCreateWindow( width, height, title, nullptr, nullptr );

   glfwSetWindowUserPointer( window, this );
   glfwSetFramebufferSizeCallback( window, framebuffer_resize_callback );
}


// Environment agnostic

void vulkan_wrapper::init_vulkan(
   const char* appname )
{
   create_instance( appname );
   setup_debug_messenger();
   create_surface();
   pick_physical_device();
   create_logical_device();
   create_swap_chain();
   create_image_views();
   create_render_pass();
   create_graphics_pipeline();

   create_framebuffers();

   create_command_pool();
   create_vertex_buffer();
   create_index_buffer();
   create_command_buffer();
   create_sync_objects();
}

void vulkan_wrapper::cleanup()
{
   destroy_debug_messenger();

   destroy_window();
}


//__________________________________________________________________________________________________
void vulkan_wrapper::framebuffer_resize_callback(
   GLFWwindow* window,
   [[maybe_unused]] int width,
   [[maybe_unused]] int height )
{
   auto app = reinterpret_cast<vulkan_wrapper*>( glfwGetWindowUserPointer( window ) );
   app->framebuffer_resized = true;
}

void vulkan_wrapper::create_instance(
   [[maybe_unused]] const char* app_name )
{
   if ( enableValidationLayers && !check_validation_layer_support() )
   {
      throw std::runtime_error( "validation layers requested, but not available!" );
   }

   std::string application_name( app_name );

   vulkan_engine.initialise(
      application_name,   //"Vulkan Datapath Tutorial", //app_name,
      VK_MAKE_VERSION( 1, 0, 0 ),
      get_required_instance_extensions() );
}


auto vulkan_wrapper::get_required_instance_extensions()
   -> vk_used_extensions_t
{
   vk_used_extensions_t used_extensions{};
   uint32_t glfw_used_vk_extensions_count{ 0 };

   auto* glfw_used_vk_extensions_raw = glfwGetRequiredInstanceExtensions( &glfw_used_vk_extensions_count );

   std::span<const char*> glfw_used_vk_extensions{
      glfw_used_vk_extensions_raw,
      glfw_used_vk_extensions_count };

   for ( const auto* const glfw_used_vk_extension_name_string : glfw_used_vk_extensions )
   {
      used_extensions.add_extension( glfw_used_vk_extension_name_string );
   }

   // Add extra instance extension here
   used_extensions.add_extension( "VK_KHR_surface" );

   return used_extensions;
}

void vulkan_wrapper::create_surface()
{
   auto alloc_func =
      [&]( VkSurfaceKHR* surface )
      {
         return
            glfwCreateWindowSurface(
               vulkan_engine.instance_handle(),
               window,
               vk_allocation_cb,
               surface );
      };

   auto [window_surface, create_error_value] =
      VkSurfaceKHR_resource_t::make(
         alloc_func,
         vulkan_engine.get_instance_dispatcher() );


   if ( create_error_value != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to create window surface!" );
   }

   surface = std::move( window_surface );
}

void vulkan_wrapper::pick_physical_device()
{
   const auto& physical_devices{ vulkan_engine.physical_devices() };

   if ( physical_devices.empty() )
   {
      throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
   }

   physical_device.reset();

   for ( const auto& device : physical_devices )
   {
      if ( is_device_suitable( device ) )
      {
         physical_device = device;
         break;
      }
   }

   if ( !physical_device.has_value() )
   {
      throw std::runtime_error( "failed to find a suitable GPU!" );
   }
}

auto vulkan_wrapper::is_device_suitable(
   const physical_device_wrapper_t& device )
   -> bool
{
   QueueFamilyIndices indices = find_queue_families( device );

   bool extensionsSupported = check_device_extension_support( device );

   return indices.isComplete() && extensionsSupported;
}

auto vulkan_wrapper::check_device_extension_support(
   const physical_device_wrapper_t& device )
   -> bool
{
   std::optional<const std::string> layer_name{ std::nullopt };
   auto available_extensions = device.vkEnumerateDeviceExtensionProperties( layer_name );

   std::set<std::string> required_extensions(
      deviceExtensions.begin(),
      deviceExtensions.end() );

   for ( const auto& extension : available_extensions )
   {
      required_extensions.erase( extension.extensionName );
   }

   return required_extensions.empty();
}


// Looks like something wrong here
auto vulkan_wrapper::find_queue_families(
   const physical_device_wrapper_t& device )
   -> QueueFamilyIndices
{
   QueueFamilyIndices indices;

   auto queue_families = device.vkGetPhysicalDeviceQueueFamilyProperties();

   for ( uint32_t i = 0;
         const auto& queueFamily : queue_families )
   {
      if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
      {
         indices.graphicsFamily = i;
      }

      if ( device.vkGetPhysicalDeviceSurfaceSupportKHR( i, surface ) )
      {
         indices.presentFamily = i;
      }

      if ( indices.isComplete() )
      {
         break;
      }

      indices.reset();
      ++i;
   }

   return indices;
}

void vulkan_wrapper::create_logical_device()
{
   if ( !physical_device.has_value() )
   {
      throw std::runtime_error( "Physical device has not been initialised!" );
   }

   QueueFamilyIndices indices = find_queue_families( *physical_device );

   float queue_priority = 1.0f;
   std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
   std::set<uint32_t> unique_queue_families = { *indices.graphicsFamily, *indices.presentFamily };

   for ( uint32_t queue_family : unique_queue_families )
   {
      queue_create_infos.emplace_back(
         VkDeviceQueueCreateInfo{
            .sType = get_sType<VkDeviceQueueCreateInfo>(),
            .queueFamilyIndex = queue_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority } );
   }

   VkPhysicalDeviceFeatures device_features{};

   std::vector<const char*> c_device_extensions;
   c_device_extensions.reserve(
      deviceExtensions.size() );

   for ( const auto& s : deviceExtensions )
   {
      c_device_extensions.push_back( s );
   }

   VkDeviceCreateInfo create_info{
      .sType = get_sType<VkDeviceCreateInfo>(),
      .queueCreateInfoCount = static_cast<uint32_t>( queue_create_infos.size() ),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>( c_device_extensions.size() ),
      .ppEnabledExtensionNames = c_device_extensions.data(),
      .pEnabledFeatures = &device_features };

   if ( enableValidationLayers )
   {
      create_info.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
      create_info.ppEnabledLayerNames = validationLayers.data();
   }
   else
   {
      create_info.enabledLayerCount = 0;
   }

   auto result = physical_device->vkCreateDevice( create_info );

   if ( result.holds_error() )
   {
      throw std::runtime_error( "failed to create logical device!" );
   }
   else
   {
      logical_device = result.value();
      graphics_queue = result.value()->vkGetDeviceQueue( *indices.graphicsFamily, 0 );
      present_queue = result.value()->vkGetDeviceQueue( *indices.presentFamily, 0 );
   }
}

auto vulkan_wrapper::check_validation_layer_support()
   -> bool
{
   auto available_layers = initial_dispatcher.vkEnumerateInstanceLayerProperties();

   for ( const char* layerName : validationLayers )
   {
      bool layerFound = false;

      for ( const auto& layerProperties : available_layers )
      {
         if ( strcmp( layerName, layerProperties.layerName ) == 0 )
         {
            layerFound = true;
            break;
         }
      }

      if ( !layerFound )
      {
         return false;
      }
   }

   return true;
}

auto vulkan_wrapper::get_required_extensions()
   -> std::vector<const char*>
{
   uint32_t glfwExtensionCount = 0;
   const char** glfwExtensions;
   glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

   std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

   if ( enableValidationLayers )
   {
      extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
   }

   return extensions;
}

VKAPI_ATTR auto VKAPI_CALL vulkan_wrapper::debug_callback(
   [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
   [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
   [[maybe_unused]] void* pUserData )
   -> VkBool32
{
   std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

   return VK_FALSE;
}

void vulkan_wrapper::populate_debug_messenger_create_info(
   VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
   createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
   createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
   createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
   createInfo.pfnUserCallback = debug_callback;
   createInfo.pUserData = this;   // Optional
}

void vulkan_wrapper::setup_debug_messenger()
{
   if ( !enableValidationLayers )
   {
      return;
   }

   VkDebugUtilsMessengerCreateInfoEXT create_info{};
   populate_debug_messenger_create_info( create_info );

   auto instance{ vulkan_engine.get_instance_dispatcher() };


   auto result = instance->vkCreateDebugUtilsMessengerEXT( create_info );

   if ( result.holds_error() )
   {
      throw std::runtime_error( "failed to set up debug messenger!" );
   }
   else
   {
      debug_messenger = std::move( result ).value();
   }
}

void vulkan_wrapper::destroy_debug_messenger()
{
   if ( !enableValidationLayers || debug_messenger.empty() )
   {
      return;
   }

   auto instance{ vulkan_engine.get_instance_dispatcher() };

   instance->vkDestroyDebugUtilsMessengerEXT( *debug_messenger );

   debug_messenger.reset();
}

void vulkan_wrapper::create_swap_chain()
{
   SwapChainSupportDetails swap_chain_support = query_swapchain_support();

   VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format( swap_chain_support.formats );
   VkPresentModeKHR presentMode = choose_swap_present_mode( swap_chain_support.presentModes );
   VkExtent2D extent = choose_swap_extent( swap_chain_support.capabilities );

   uint32_t imageCount = swap_chain_support.capabilities.minImageCount + 1;
   if ( swap_chain_support.capabilities.maxImageCount > 0 &&
        imageCount > swap_chain_support.capabilities.maxImageCount )
   {
      imageCount = swap_chain_support.capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR create_info{};
   create_info.sType =
      get_sType<VkSwapchainCreateInfoKHR>();   // VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   create_info.surface = surface;

   create_info.minImageCount = imageCount;
   create_info.imageFormat = surfaceFormat.format;
   create_info.imageColorSpace = surfaceFormat.colorSpace;
   create_info.imageExtent = extent;
   create_info.imageArrayLayers = 1;
   create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   QueueFamilyIndices indices = find_queue_families( *physical_device );
   uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

   if ( indices.graphicsFamily != indices.presentFamily )
   {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queueFamilyIndices;
   }
   else
   {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = nullptr;
   }

   create_info.preTransform = swap_chain_support.capabilities.currentTransform;
   create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   create_info.presentMode = presentMode;
   create_info.clipped = VK_TRUE;

   create_info.oldSwapchain = VK_NULL_HANDLE;

   auto result = logical_device->vkCreateSwapchainKHR( create_info );

   if ( result.holds_error() )
   {
      throw std::runtime_error( "failed to create swap chain!" );
   }

   swapchain = std::move( result ).value();
   swapchain_images = logical_device->vkGetSwapchainImagesKHR( *swapchain );
   swapchain_image_format = surfaceFormat.format;
   swapchain_extent = extent;
}


auto vulkan_wrapper::query_swapchain_support()
   -> SwapChainSupportDetails
{
   SwapChainSupportDetails details;

   details.capabilities = physical_device->vkGetPhysicalDeviceSurfaceCapabilitiesKHR( *surface );
   details.formats = physical_device->vkGetPhysicalDeviceSurfaceFormatsKHR( *surface );
   details.presentModes = physical_device->vkGetPhysicalDeviceSurfacePresentModesKHR( *surface );

   return details;
}


auto vulkan_wrapper::choose_swap_surface_format(
   const std::vector<VkSurfaceFormatKHR>& availableFormats )
   -> VkSurfaceFormatKHR
{
   for ( const auto& availableFormat : availableFormats )
   {
      if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
           availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
      {
         return availableFormat;
      }
   }

   return availableFormats[0];
}

auto vulkan_wrapper::choose_swap_present_mode(
   const std::vector<VkPresentModeKHR>& availablePresentModes )
   -> VkPresentModeKHR
{
   for ( const auto& availablePresentMode : availablePresentModes )
   {
      if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
      {
         return availablePresentMode;
      }
   }

   return VK_PRESENT_MODE_FIFO_KHR;
}

auto vulkan_wrapper::choose_swap_extent(
   const VkSurfaceCapabilitiesKHR& capabilities )
   -> VkExtent2D
{
   if ( capabilities.currentExtent.width != UINT_MAX )
   {   // std::numeric_limits<uint32_t>::max()) {
      return capabilities.currentExtent;
   }
   else
   {
      int width, height;
      glfwGetFramebufferSize( window, &width, &height );

      VkExtent2D actualExtent = { static_cast<uint32_t>( width ), static_cast<uint32_t>( height ) };

      actualExtent.width =
         std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width );
      actualExtent.height =
         std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height );

      return actualExtent;
   }
}

void vulkan_wrapper::create_image_views()
{
   swapchain_image_views.clear();
   swapchain_image_views.reserve(
      swapchain_images.size() );

   for ( const auto& swapchain_image : swapchain_images )
   {
      VkImageViewCreateInfo create_info{};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = swapchain_image;
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = swapchain_image_format;

      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;

      auto result = logical_device->vkCreateImageView( create_info );

      if ( result.holds_error() )
      {
         throw std::runtime_error( "failed to create image views!" );
      }

      swapchain_image_views.emplace_back(
         std::move( result ).value() );
   }
}

void vulkan_wrapper::create_graphics_pipeline()
{
   auto vert_shader_code = read_file( "shaders/vert.spv" );
   auto frag_shader_code = read_file( "shaders/frag.spv" );

   auto vert_shader_module = create_shader_module( vert_shader_code );
   auto frag_shader_module = create_shader_module( frag_shader_code );

   VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
   vert_shader_stage_info.sType = get_sType<VkPipelineShaderStageCreateInfo>();
   vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
   vert_shader_stage_info.module = vert_shader_module.get();
   vert_shader_stage_info.pName = "main";

   VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
   frag_shader_stage_info.sType = get_sType<VkPipelineShaderStageCreateInfo>();
   frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   frag_shader_stage_info.module = frag_shader_module.get();
   frag_shader_stage_info.pName = "main";

   std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
      vert_shader_stage_info,
      frag_shader_stage_info };

   // Dynamic state
   std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

   VkPipelineDynamicStateCreateInfo dynamic_state{};
   dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   dynamic_state.dynamicStateCount = static_cast<uint32_t>( dynamic_states.size() );
   dynamic_state.pDynamicStates = dynamic_states.data();

   // Vertex input
   auto bindingDescription = Vertex::getBindingDescription();
   auto attributeDescriptions = Vertex::getAttributeDescriptions();

   VkPipelineVertexInputStateCreateInfo vertex_input_info{
      .sType = get_sType<VkPipelineVertexInputStateCreateInfo>(),
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescriptions.size() ),
      .pVertexAttributeDescriptions = attributeDescriptions.data() };

   // Input assembly
   VkPipelineInputAssemblyStateCreateInfo input_assembly{
      .sType = get_sType<VkPipelineInputAssemblyStateCreateInfo>(),
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE };

   // Viewports and scissors
   VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)swapchain_extent.width,
      .height = (float)swapchain_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f };

   VkRect2D scissor{
      .offset = { 0, 0 },
      .extent = swapchain_extent };

   VkPipelineViewportStateCreateInfo viewport_state{
      .sType = get_sType<VkPipelineViewportStateCreateInfo>(),
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor };

   // Rasterizer
   VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = get_sType<VkPipelineRasterizationStateCreateInfo>(),
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f   // NOLINT (readability-uppercase-literal-suffix
   };

   // Multisampling
   VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = get_sType<VkPipelineMultisampleStateCreateInfo>(),
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE };

   // Depthand stencil testing

   // Color blending
   VkPipelineColorBlendAttachmentState color_blend_attachment{
      .blendEnable = VK_FALSE,
      //.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,    // Optional
      //.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,   // Optional
      //.colorBlendOp = VK_BLEND_OP_ADD,               // Optional
      //.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,    // Optional
      //.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,   // Optional
      //.alphaBlendOp = VK_BLEND_OP_ADD,               // Optional
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT };

   /*
      colorBlendAttachment.blendEnable = VK_TRUE;
      colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
      colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
   */

   VkPipelineColorBlendStateCreateInfo color_blending{
      .sType = get_sType<VkPipelineColorBlendStateCreateInfo>(),
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,   // Optional
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment,
      .blendConstants{ 0.0f, 0.0f, 0.0f, 0.0f }   // Optional
   };

   // Pipeline layout
   VkPipelineLayoutCreateInfo pipeline_layout_info{
      .sType = get_sType<VkPipelineLayoutCreateInfo>(),
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0 };

   auto pipeline_layout_result = logical_device->vkCreatePipelineLayout( pipeline_layout_info );
   if ( pipeline_layout_result.holds_error() )
   {
      throw std::runtime_error( "failed to create pipeline layout!" );
   }

   pipeline_layout = std::move( pipeline_layout_result ).value();

   // Create pipeline
   VkGraphicsPipelineCreateInfo pipeline_info{
      .sType = get_sType<VkGraphicsPipelineCreateInfo>(),
      .stageCount = 2,
      .pStages = shader_stages.data(),
      .pVertexInputState = &vertex_input_info,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = nullptr,   // Optional
      .pColorBlendState = &color_blending,
      //.pDynamicState = &dynamic_state,
      .layout = *pipeline_layout,
      .renderPass = *render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,   // Optional
      .basePipelineIndex = -1,                // Optional
   };

   VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
   std::span<VkGraphicsPipelineCreateInfo> pipeline_infos{ &pipeline_info, 1 };

   auto graphic_pipeline_result = logical_device->vkCreateGraphicsPipelines( pipeline_cache, pipeline_infos );
   if ( graphic_pipeline_result.holds_error() )
   {
      throw std::runtime_error( "failed to create graphics pipeline!" );
   }

   graphics_pipeline = std::move( graphic_pipeline_result ).value();
}


auto vulkan_wrapper::read_file(
   const std::string& filename )
   -> std::vector<char>
{
   std::ifstream file( filename, std::ios::ate | std::ios::binary );

   if ( !file.is_open() )
   {
      throw std::runtime_error( "failed to open file!" );
   }

   size_t fileSize = (size_t)file.tellg();
   std::vector<char> buffer( fileSize );

   file.seekg( 0 );
   file.read(
      buffer.data(),
      fileSize );

   file.close();

   return buffer;
}

auto vulkan_wrapper::create_shader_module(
   const std::vector<char>& code )
   -> VkShaderModule_resource_t
{
   VkShaderModuleCreateInfo create_info{};
   create_info.sType = get_sType<VkShaderModuleCreateInfo>();   // VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
   create_info.codeSize = code.size();
   create_info.pCode = reinterpret_cast<const uint32_t*>( code.data() );

   auto shader_module = logical_device->vkCreateShaderModule( create_info );

   if ( shader_module.holds_error() )
   {
      throw std::runtime_error( "failed to create shader module!" );
   }

   return std::move( shader_module ).value();
}

void vulkan_wrapper::create_render_pass()
{
   // Attachment description
   VkAttachmentDescription color_attachment{
      .format = swapchain_image_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };

   // Subpasses and attachment references
   VkAttachmentReference color_attachment_ref{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

   VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref };

   VkSubpassDependency dependency{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };

   // Render pass
   VkRenderPassCreateInfo render_pass_info{
      .sType = get_sType<VkRenderPassCreateInfo>(),
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency };

   auto render_pass_result = logical_device->vkCreateRenderPass( render_pass_info );

   if ( render_pass_result.holds_error() )
   {
      throw std::runtime_error( "failed to create render pass!" );
   }

   render_pass = std::move( render_pass_result ).value();
}

void vulkan_wrapper::create_framebuffers()
{
   swapchain_framebuffers.clear();
   swapchain_framebuffers.reserve(
      swapchain_image_views.size() );

   for ( auto& swapchain_image_view : swapchain_image_views )
   {
      VkImageView attachments[] = { *swapchain_image_view };

      VkFramebufferCreateInfo framebuffer_info{
         .sType = get_sType<VkFramebufferCreateInfo>(),
         .renderPass = render_pass.get(),
         .attachmentCount = 1,
         .pAttachments = attachments,
         .width = swapchain_extent.width,
         .height = swapchain_extent.height,
         .layers = 1,
      };

      auto swapchain_framebuffer_result = logical_device->vkCreateFramebuffer( framebuffer_info );

      if ( swapchain_framebuffer_result.holds_error() )
      {
         throw std::runtime_error( "failed to create framebuffer!" );
      }

      swapchain_framebuffers.emplace_back(
         std::move( swapchain_framebuffer_result ).value() );
   }
}

void vulkan_wrapper::create_command_pool()
{
   QueueFamilyIndices queue_family_indices = find_queue_families( *physical_device );

   VkCommandPoolCreateInfo pool_info{
      .sType = get_sType<VkCommandPoolCreateInfo>(),
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family_indices.graphicsFamily.value() };

   auto command_pool_result = logical_device->vkCreateCommandPool( pool_info );
   if ( command_pool_result.holds_error() )
   {
      throw std::runtime_error( "failed to create command pool!" );
   }

   command_pool = std::move( command_pool_result ).value();
}

void vulkan_wrapper::create_command_buffer()
{
   DPVkCommandBufferAllocateInfo_t command_buffer_alloc_info{
      .command_pool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .command_buffer_count = max_frames_in_flight };

   auto command_buffer_result = logical_device->vkAllocateCommandBuffers( command_buffer_alloc_info );

   if ( command_buffer_result.holds_error() )
   {
      throw std::runtime_error( "failed to allocate command buffers!" );
   }
   command_buffer = std::move( command_buffer_result ).value();
}

void vulkan_wrapper::record_command_buffer(
   command_buffer_wrapper_t& command_buffer,
   uint32_t imageIndex )
{
   // Begin recording command
   VkCommandBufferBeginInfo begin_info{
      .sType = get_sType<VkCommandBufferBeginInfo>(),
      .flags = 0,                   // Optional
      .pInheritanceInfo = nullptr   // Optional
   };

   if ( command_buffer.vkBeginCommandBuffer( begin_info ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to begin recording command buffer!" );
   }

   std::array<VkClearValue, 2> clear_values{};
   clear_values[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
   clear_values[1].depthStencil = { 1.0f, 0 };

   // starting a render pass
   VkRenderPassBeginInfo render_pass_info{
      .sType = get_sType<VkRenderPassBeginInfo>(),
      .renderPass = *render_pass,
      .framebuffer = *swapchain_framebuffers[imageIndex],
      .renderArea{
         .offset = { 0, 0 },
         .extent = swapchain_extent },
      .clearValueCount = static_cast<uint32_t>( clear_values.size() ),
      .pClearValues = clear_values.data() };

   command_buffer.vkCmdBeginRenderPass( render_pass_info, VK_SUBPASS_CONTENTS_INLINE );

   //  bind command_buffer to the graphics pipeline
   command_buffer.vkCmdBindPipeline(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      graphics_pipeline.front().get() );

   // Set viewport and scissor
   std::vector<VkViewport> viewports{ { .x = 0.0f, .y = 0.0f, .width = static_cast<float>( swapchain_extent.width ), .height = static_cast<float>( swapchain_extent
                                                                                                                                                      .height ),
                                        .minDepth = 0.0f,
                                        .maxDepth = 1.0f } };

   command_buffer.vkCmdSetViewport( 0, viewports );

   std::vector<VkRect2D> scissors{ { .offset = { 0, 0 }, .extent = swapchain_extent } };

   command_buffer.vkCmdSetScissor( 0, scissors );

   std::array<VkDeviceSize, 1> offsets{ 0 };

   command_buffer.vkCmdBindVertexBuffers(
      0,
      std::span<const VkBuffer>( &vertex_buffer.get(), 1 ),
      offsets );

   command_buffer.vkCmdBindIndexBuffer(
      index_buffer.get(),
      0,
      VK_INDEX_TYPE_UINT16 );

   // Draw command buffer
   //command_buffer.vkCmdDraw( 3, 1, 0, 0 );
   command_buffer.vkCmdDrawIndexed(static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0 );


   // Finishing up
   command_buffer.vkCmdEndRenderPass();

   if ( command_buffer.vkEndCommandBuffer() != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to record command buffer!" );
   }
}

void vulkan_wrapper::create_sync_objects()
{
   image_available_semaphores.clear();
   render_finished_semaphores.clear();
   in_flight_fences.clear();

   VkSemaphoreCreateInfo semaphore_info{
      .sType = get_sType<VkSemaphoreCreateInfo>() };

   VkFenceCreateInfo fence_info{
      .sType = get_sType<VkFenceCreateInfo>(),
      .flags = VK_FENCE_CREATE_SIGNALED_BIT };

   for ( size_t i = 0;
         i < max_frames_in_flight;
         i++ )
   {
      auto semaphore1_result = logical_device->vkCreateSemaphore( semaphore_info );
      auto semaphore2_result = logical_device->vkCreateSemaphore( semaphore_info );
      auto fence_result = logical_device->vkCreateFence( fence_info );
      if ( semaphore1_result.holds_error() || semaphore2_result.holds_error() || fence_result.holds_error() )
      {
         throw std::runtime_error( "failed to create semaphores!" );
      }
      image_available_semaphores.emplace_back(
         std::move( semaphore1_result ).value() );
      render_finished_semaphores.emplace_back(
         std::move( semaphore2_result ).value() );
      in_flight_fences.emplace_back(
         std::move( fence_result ).value() );
   }
}

void vulkan_wrapper::draw_frame()
{
   // Wait for the previous frame to finish
   std::span<const VkFence> fences{ &in_flight_fences[current_frame].get(), 1 };

   if ( logical_device->vkWaitForFences( fences, VK_TRUE, UINT64_MAX ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to wait for fences!" );
   }
   // Acquire an image from the swap chain

   uint32_t image_index =
      logical_device->vkAcquireNextImageKHR(
         *swapchain,
         UINT64_MAX,
         image_available_semaphores[current_frame].get(),
         VK_NULL_HANDLE );

   // Record a command buffer which draws the scene onto that image
   if ( command_buffer[current_frame].vkResetCommandBuffer( 0 ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to reset command buffer!" );
   }
   record_command_buffer( command_buffer[current_frame], image_index );

   // Submit the recorded command buffer
   std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
   auto cmd_buffer_handle = command_buffer[current_frame].handle();

   VkSubmitInfo submit_info{
      .sType = get_sType<VkSubmitInfo>(),
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &image_available_semaphores[current_frame].get(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_buffer_handle,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &render_finished_semaphores[current_frame].get() };

   // Only reset the fence if we are submitting work
   if ( logical_device->vkResetFences( fences ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to reset fences!" );
   }


   std::span<VkSubmitInfo> submit_info_span{ &submit_info, 1 };

   if ( ( *graphics_queue ).vkQueueSubmit( submit_info_span, in_flight_fences[current_frame] ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to submit draw command buffer!" );
   }

   // Present the swap chain image
   VkPresentInfoKHR present_info{
      .sType = get_sType<VkPresentInfoKHR>(),
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &render_finished_semaphores[current_frame].get(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain.get(),
      .pImageIndices = &image_index,
      .pResults = nullptr };

   auto result = present_queue.value().vkQueuePresentKHR( present_info );

   if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized )
   {
      framebuffer_resized = false;
      recreate_swapchain();
   }
   else if ( result != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to queue present KHR!" );
   }

   current_frame = ( current_frame + 1 ) % max_frames_in_flight;
}

void vulkan_wrapper::recreate_swapchain()
{
   if ( logical_device->vkDeviceWaitIdle() != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to wait for idle!" );
   }

   cleanup_swapchain();

   create_swap_chain();
   create_image_views();
   create_render_pass();
   create_graphics_pipeline();
   create_framebuffers();
}

void vulkan_wrapper::cleanup_swapchain()
{
   for ( auto& scfb : swapchain_framebuffers )
   {
      scfb.reset();
   }
   swapchain_framebuffers.clear();   //???

   for ( auto& gpl : graphics_pipeline )
   {
      gpl.reset();
   }
   graphics_pipeline.clear();

   pipeline_layout.reset();
   render_pass.reset();

   for ( auto& sciv : swapchain_image_views )
   {
      sciv.reset();
   }
   swapchain_image_views.clear();

   swapchain.reset();
}

void vulkan_wrapper::create_vertex_buffer()
{
   VkDeviceSize buffer_size = sizeof( Vertex ) * vertices.size();

   VkBuffer_resource_t staging_buffer;
   VkDeviceMemory_resource_t staging_buffer_memory;

   // Create staging buffer
   std::tie( staging_buffer, staging_buffer_memory ) =
      create_buffer(
         buffer_size,
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

   // Filling the staging buffer
   void* data;
   auto result =
      logical_device->vkMapMemory(
         staging_buffer_memory.get(),
         0,
         buffer_size,
         0,
         &data );

   if ( result != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to map vertex buffer memory!" );
   }
   memcpy(
      data,
      vertices.data(),
      buffer_size );

   logical_device->vkUnmapMemory(
      staging_buffer_memory.get() );

   // Create vertex buffer
   std::tie( vertex_buffer, vertex_buffer_memory ) =
      create_buffer(
         buffer_size,
         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

   // Copy data to vertex buffer
   copy_buffer(
      staging_buffer.get(),
      vertex_buffer.get(),
      buffer_size );
}

void vulkan_wrapper::create_index_buffer()
{
   VkDeviceSize buffer_size = sizeof( uint16_t ) * g_indices.size();

   VkBuffer_resource_t staging_buffer;
   VkDeviceMemory_resource_t staging_buffer_memory;

   // Create staging buffer
   std::tie( staging_buffer, staging_buffer_memory ) =
      create_buffer(
         buffer_size,
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

   // Filling the staging buffer
   void* data;
   auto result =
      logical_device->vkMapMemory(
         staging_buffer_memory.get(),
         0,
         buffer_size,
         0,
         &data );

   if ( result != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to map index buffer memory!" );
   }
   memcpy(
      data,
      g_indices.data(),
      buffer_size );

   logical_device->vkUnmapMemory(
      staging_buffer_memory.get() );

   // Create vertex buffer
   std::tie( index_buffer, index_buffer_memory ) =
      create_buffer(
         buffer_size,
         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

   // Copy data to vertex buffer
   copy_buffer(
      staging_buffer.get(),
      index_buffer.get(),
      buffer_size );
}


auto vulkan_wrapper::find_memory_type(
   uint32_t type_filter,
   VkMemoryPropertyFlags properties )
   -> uint32_t
{
   VkPhysicalDeviceMemoryProperties mem_properties = physical_device->vkGetPhysicalDeviceMemoryProperties();

   for ( uint32_t i = 0;
         i < mem_properties.memoryTypeCount;
         i++ )
   {
      if ( ( type_filter & ( 1 << i ) ) &&
           ( mem_properties.memoryTypes[i].propertyFlags & properties ) == properties )
      {
         return i;
      }
   }

   throw std::runtime_error( "failed to find suitable memory type!" );
}

auto vulkan_wrapper::create_buffer(
   VkDeviceSize size,
   VkBufferUsageFlags usage,
   VkMemoryPropertyFlags properties )
   -> std::pair<
      VkBuffer_resource_t,
      VkDeviceMemory_resource_t>
{
   VkBufferCreateInfo buffer_info{
      .sType = get_sType<VkBufferCreateInfo>(),
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE };

   auto buffer = logical_device->vkCreateBuffer( buffer_info );
   if ( buffer.holds_error() )
   {
      throw std::runtime_error( "failed to create buffer!" );
   }

   // Memory requirements
   VkMemoryRequirements mem_requirements = logical_device->vkGetBufferMemoryRequirements( *buffer.value() );

   // Memory allocation
   VkMemoryAllocateInfo alloc_info{
      .sType = get_sType<VkMemoryAllocateInfo>(),
      .allocationSize = mem_requirements.size,
      .memoryTypeIndex = find_memory_type( mem_requirements.memoryTypeBits, properties ) };

   auto buffer_memory = logical_device->vkAllocateMemory( alloc_info );

   if ( buffer_memory.holds_error() )
   {
      throw std::runtime_error( "failed to allocate buffer memory!" );
   }

   auto result =
      logical_device->vkBindBufferMemory(
         buffer.value().get(),
         buffer_memory.value().get(),
         0 );

   if ( result != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to bind buffer and alloced memory together!" );
   }

   return
      std::pair<VkBuffer_resource_t, VkDeviceMemory_resource_t>(
         std::move( buffer ).value(),
         std::move( buffer_memory ).value() );
}

void vulkan_wrapper::copy_buffer(
   VkBuffer srcBuffer,
   VkBuffer dstBuffer,
   VkDeviceSize size )
{
   DPVkCommandBufferAllocateInfo_t command_buffer_alloc_info{
      .command_pool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .command_buffer_count = 1 };


   auto command_buffer_result = logical_device->vkAllocateCommandBuffers( command_buffer_alloc_info );
   if ( command_buffer_result.holds_error() )
   {
      throw std::runtime_error( "failed to alloc command buffer!" );
   }

   const command_buffer_wrapper_t& command_buffer = command_buffer_result.value().front();

   // Begin recording command
   VkCommandBufferBeginInfo begin_info{
      .sType = get_sType<VkCommandBufferBeginInfo>(),
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr   // Optional
   };

   if ( command_buffer.vkBeginCommandBuffer( begin_info ) != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to begin recording command buffer!" );
   }

   VkBufferCopy copy_region{
      .srcOffset = 0,   // Optional
      .dstOffset = 0,   // Optional
      .size = size };

   command_buffer.vkCmdCopyBuffer(
      srcBuffer,
      dstBuffer,
      std::span<VkBufferCopy>( &copy_region, 1 ) );

   if ( command_buffer.vkEndCommandBuffer() != VK_SUCCESS )
   {
      throw std::runtime_error( "failed to record command buffer!" );
   }

   VkCommandBuffer command_buffer_handle = command_buffer.handle();
   VkSubmitInfo submit_info{
      .sType = get_sType<VkSubmitInfo>(),
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer_handle };

   auto result =
      graphics_queue->vkQueueSubmit(
         std::span<const VkSubmitInfo>( &submit_info, 1 ),
         VK_NULL_HANDLE );
   result = graphics_queue->vkQueueWaitIdle();

   // logical_device->vkFreeCommandBuffers(*command_pool, std::span<VkCommandBuffer>(&command_buffer_handle,
   // 1));
}
