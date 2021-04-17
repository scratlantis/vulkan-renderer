#include <iostream>
#include "VulkanUtils.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>



#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include "VulkanContext.h"
#include "TextureImage.h"
#include "DepthImage.h"
#include "Vertex.h"
#include "MeshLoader.h"
#include "GBuffer.h"
#include "Scene.h"
#include "DeferredRenderer.h"
#include "ForwardRenderer.h"
#include "VbRenderer.h"
#include "PointLightData.h"
#include "DecalData.h"
#include "Cursor.h"


#define CAMERA_SPEED 1.0f
#define LIGHT_SPEED 3.0f
#define DELTA_LIGHT_COLOR 1.01f
#define CAMERA_TURN_SPEED 60.0
#define INITIAL_CAM_POS glm::vec3(2.0, 0, .5)
#define INITIAL_CAM_FRONT glm::vec3(0, 1.0, 0)
#define INITIAL_CAM_UP glm::vec3(0, 0, 1.0)
#define DECAL_CURSOR_ATTRIBUTE_COUNT 4
#define LIGHT_CURSOR_ATTRIBUTE_COUNT 4

using namespace VulkanUtils;

float cam_turn_speed = CAMERA_TURN_SPEED;
float cam_speed = CAMERA_SPEED;



ResourceLoadInfo sceneInfos[3] =
{
    { "models/emeraldSquare/LittleDecalsLittleLights/emeraldSquareLittleDecalsLittleLights.obj", "models/emeraldSquare/LittleDecalsLittleLights", "models/emeraldSquare/TexturesHalfSize" },
    { "models/sunTemple/decalBlending/decalBlending.obj", "models/sunTemple/decalBlending", "models/sunTemple/Textures" },
    { "models/bistro/testScene4/bistroWithDecals.obj", "models/bistro/testScene4", "models/bistro/Textures" }
};


struct MaterialData
{
    //glm::vec4 ambient;		//Ka
    //glm::vec4 diffuse;		//Kd
    //glm::vec4 specular;		//Ks

    glm::uint slotUsed;
    //Texture map indices
    glm::uint idx_map_specular;
    glm::uint idx_map_diffuse;
    glm::uint idx_map_normal;

    glm::uint idx_map_alpha;
    glm::uint idx_map_emissive;
    // Placeholder values for 16 Byte memory alginement
    glm::uint placeholder2;
    glm::uint placeholder3;
};
// ToDo reduce data



struct Camera {
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
};

enum RenderMode
{
    FORWARD             = 0x00000001,
    DEFERRED            = 0x00000002,
    VISIBILITY_BUFFER   = 0x00000004,
    RAYTRAYCING         = 0x00000008, // TODO
};

enum LOAD_FLAGS
{
    UNIFORM_BUFFERS_CREATED         = 0x00000001,
    PER_FRAME_CONSTANTS_INITIALIZED = 0x00000002,
    STATIC_UNIFORM_BUFFERS_WRITTEN  = 0x00000004,
};

uint32_t loadStatus = 0x00000000;
uint32_t flags;



enum DecalAttributes
{
    DECAL_MATERIAL,
    DECAL_LAYER,
    DECAL_WEIGTH,
    DECAL_SCALE
};

enum LightAttributes
{
    LIGHT_R,
    LIGHT_G,
    LIGHT_B,
    LIGHT_A
};

uint32_t currentSelectedDecalAttribute = DECAL_MATERIAL;
uint32_t currentSelectedLigthAttribute = LIGHT_A;
float decalScale = 1.0;
uint32_t currentSelectedDecal = 0;

VulkanContext context;
RenderMode renderMode;
std::vector<VkPhysicalDevice> physicalDevices;
VkImageView* imageViews;
DeferredRenderer* deferredRenderer;
ForwardRenderer* forwardRenderer;
VbRenderer* vbRenderer;
VkSampler colorSampler;
uint32_t deviceIndex = DEVICE_INDEX;
bool tab_Pressed = false;
Camera cam;
Scene* scene;
std::vector<MaterialData> mat;
std::vector<DecalData> dec;
std::vector<PointLightData> pointLightData;
float timeSinceStart = 0;
std::chrono::steady_clock::time_point  frameTime;
double maxFrameTime = 0;
float  frameDeltaTime = 0.03f; // Initial value for frame time, updated after 30 frames
uint32_t frameCounter = 0;
std::chrono::steady_clock::time_point frameCounterTime;
AccelerationStructureBuilder asBuilder;
ResourceLoadInfo sceneInfo;
MaterialLoader materialLoader;
DecalLoader decalLoader;
PointLightLoader pointLightLoader;
MeshLoader meshLoader;
Cursor cursor;
bool q_Pressed = false;
bool t_Pressed = false;
bool f_Pressed = false;
bool e_Pressed = false;
std::string texturePath = "images";





void getDeviceFeatures()
{
    vkGetPhysicalDeviceProperties(context.m_physicalDevice, &context.m_deviceProperties);
}


void printStats(VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);


    std::cout << "Name: " << properties.deviceName << std::endl;

    uint32_t apiVer = properties.apiVersion;


    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    std::cout << "API Version:      " << VK_VERSION_MAJOR(apiVer) << "." << VK_VERSION_MINOR(apiVer) << "." << VK_VERSION_PATCH(apiVer) << std::endl;
    std::cout << "Driver Version:   " << properties.driverVersion << std::endl;
    std::cout << "Vendor ID:        " << properties.vendorID << std::endl;
    std::cout << "Device ID:        " << properties.deviceID << std::endl;
    std::cout << "Device Type:      " << properties.deviceType << std::endl;
    std::cout << "Geometry Shader:  " << features.geometryShader << std::endl;
    std::cout << "discreteQueuePriorities: " << properties.limits.discreteQueuePriorities << std::endl;

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(device, &memProp);


    uint32_t amountOfQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, nullptr);
    VkQueueFamilyProperties* familyProperties = new VkQueueFamilyProperties[amountOfQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &amountOfQueueFamilies, familyProperties);


    std::cout << "Amount of Queue Families: " << amountOfQueueFamilies << std::endl;
    for (int i = 0; i < amountOfQueueFamilies; i++) {
        std::cout << std::endl;
        std::cout << "Queue Family #" << i << std::endl;
        std::cout << "VK_QUEUE_GRAPHICS_BIT         " << ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_COMPUTE_BIT          " << ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_TRANSFER_BIT         " << ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "VK_QUEUE_SPARSE_BINDING_BIT   " << ((familyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
        std::cout << "Queue Count: " << familyProperties[i].queueCount << std::endl;
        std::cout << "Timestamp Valid Bits: " << familyProperties[i].timestampValidBits << std::endl;
        uint32_t width = familyProperties[i].minImageTransferGranularity.width;
        uint32_t height = familyProperties[i].minImageTransferGranularity.height;
        uint32_t depth = familyProperties[i].minImageTransferGranularity.depth;
        std::cout << "Min Image Transfer Granularity " << width << ", " << height << ", " << depth << std::endl;

    }


    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.m_surface, &surfaceCapabilities);
    std::cout << "\tSurface capabilities:     " << std::endl;
    std::cout << "\tminImageCount:            " << surfaceCapabilities.minImageCount << std::endl;
    std::cout << "\tmaxImageCount:            " << surfaceCapabilities.maxImageCount << std::endl;
    std::cout << "\tcurrentExtent:            " << surfaceCapabilities.currentExtent.width << "/" << surfaceCapabilities.currentExtent.height << std::endl;
    std::cout << "\tminImageExtent:           " << surfaceCapabilities.minImageExtent.width << "/" << surfaceCapabilities.minImageExtent.height << std::endl;
    std::cout << "\tmaxImageExtent:           " << surfaceCapabilities.maxImageExtent.width << "/" << surfaceCapabilities.maxImageExtent.height << std::endl;
    std::cout << "\tmaxImageArrayLayers:      " << surfaceCapabilities.maxImageArrayLayers << std::endl;
    std::cout << "\tsupportedTransforms:      " << surfaceCapabilities.supportedTransforms << std::endl;
    std::cout << "\tcurrentTransform:         " << surfaceCapabilities.currentTransform << std::endl;
    std::cout << "\tsupportedCompositeAlpha:  " << surfaceCapabilities.supportedCompositeAlpha << std::endl;
    std::cout << "\tsupportedUsageFlags:  " << surfaceCapabilities.supportedUsageFlags << std::endl;

    uint32_t amountOfFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.m_surface, &amountOfFormats, nullptr);
    VkSurfaceFormatKHR* surfaceFormats = new VkSurfaceFormatKHR[amountOfFormats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.m_surface, &amountOfFormats, surfaceFormats);


    std::cout << std::endl;
    std::cout << "Amount of Formats: " << amountOfFormats << std::endl;
    for (int i = 0; i < amountOfFormats; i++)
    {
        std::cout << "Formats: " << surfaceFormats[i].format << std::endl;
    }

    uint32_t amountOfPresentationModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.m_surface, &amountOfPresentationModes, nullptr);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[amountOfPresentationModes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.m_surface, &amountOfPresentationModes, presentModes);


    std::cout << std::endl;
    std::cout << "Amount of Presentation Modes: " << amountOfPresentationModes << std::endl;
    for (int i = 0; i < amountOfPresentationModes; i++)
    {
        std::cout << "Supported presentation mode: " << presentModes[i] << std::endl;
    }

    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
    VkExtensionProperties*  deviceExtensions = new VkExtensionProperties[deviceExtensionCount];
    VkResult result = vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExtensions);
    for (int i = 0; i < deviceExtensionCount; i++)
    {
        std::cout << "Supported extensions: " << deviceExtensions[i].extensionName << std::endl;
    }
    

    VkPhysicalDeviceRayTracingPropertiesKHR ray_tracing_properies = {};
    ray_tracing_properies.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 device_properties_2 = {};
    device_properties_2.pNext = &ray_tracing_properies;
    device_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkGetPhysicalDeviceProperties2(device, &device_properties_2);
    std::cout << "#############    ray tracing start   ##############" << std::endl;
    std::cout << "\tmaxDescriptorSetAccelerationStructures:            " << ray_tracing_properies.maxDescriptorSetAccelerationStructures << std::endl;
    std::cout << "#############    ray tracing end   ##############" << std::endl;

    std::cout << std::endl;
    delete[] familyProperties;
    delete[] surfaceFormats;
    delete[] presentModes;

}

void recreateSwapChain();




void onWindowResized(GLFWwindow* window, int w, int h) {

    std::cout << "Resizing window!" << std::endl;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[deviceIndex], context.m_surface, &surfaceCapabilities);

    if (w > surfaceCapabilities.maxImageExtent.width) w = surfaceCapabilities.maxImageExtent.width;
    if (h > surfaceCapabilities.maxImageExtent.height) h = surfaceCapabilities.maxImageExtent.height;

    if (w == 0 || h == 0) return;
    context.m_width = w;
    context.m_height = h;
    recreateSwapChain();
}

void initializePerFrameConstants()
{
    if (!(loadStatus & PER_FRAME_CONSTANTS_INITIALIZED))
    {
        context.m_perFrameConstants = {};
        context.m_perFrameConstants.counter = 0;
        //context.m_perFrameConstants.lightColor = glm::vec4(252.0 / 255.0, 238.0 / 255.0, 167.0 / 255.0, 10.0);
        context.m_perFrameConstants.lightColor = glm::vec4(255.0 / 255.0, 227.0 / 255.0, 115.0 / 255.0, 10.0);
        context.m_perFrameConstants.lightPosition = glm::vec4(1.5, 0.0, 3.0, 1.0);


        //Default values for decal cursor (without transformation)
        cursor.decal.channel = 0;
        cursor.decal.materialIndex = 0;// ToDo change
        cursor.decal.uvCoordAB.x = 0.0;
        cursor.decal.uvCoordAB.y = 0.0;
        cursor.decal.uvCoordAB.z = 1.0;
        cursor.decal.uvCoordAB.w = 0.0;
        cursor.decal.uvCoordCD.x = 1.0;
        cursor.decal.uvCoordCD.y = 1.0;
        cursor.decal.uvCoordCD.z = 0.0;
        cursor.decal.uvCoordCD.w = 1.0;
        cursor.decal.weight = 1;

        cursor.light.color = glm::vec4(255.0 / 255.0, 227.0 / 255.0, 115.0 / 255.0, 1.0);
        cursor.selection = 1;
    }
    loadStatus |= PER_FRAME_CONSTANTS_INITIALIZED;

}

void startGlfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    context.m_window = glfwCreateWindow(context.m_width, context.m_height, "Vulkan Renderer", nullptr, nullptr);
    //context.m_window = glfwCreateWindow(context.m_width, context.m_height, "Vulkan Renderer", glfwGetPrimaryMonitor(), nullptr);
    glfwSetWindowSizeCallback(context.m_window, onWindowResized);

    //cam.pos = INITIAL_CAM_POS;
    //cam.front = INITIAL_CAM_FRONT;
    cam.up = INITIAL_CAM_UP;
    //cam.pitch = 0.0;
    //cam.yaw = 180.0;

    cam.pos = glm::vec3(2.44f, -0.21f, 1.88f);
    cam.pitch = -29.5;
    cam.yaw = 183.8;


    glm::vec3 direction;
    direction.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    direction.z = sin(glm::radians(cam.pitch));
    direction.y = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    cam.front = glm::normalize(direction);


    //initializeMouse(width, height);
}






void createInstance() {
    VkApplicationInfo appInfo;
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Vulkan Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.pEngineName = "Renderbender";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;


    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        //"VK_LAYER_LUNARG_monitor"
    };

    uint32_t amountOfGlfwExtensions = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&amountOfGlfwExtensions);

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = validationLayers.size();
    instanceInfo.ppEnabledLayerNames = validationLayers.data();
    instanceInfo.enabledExtensionCount = amountOfGlfwExtensions;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &context.m_instance);
    ASSERT_VULKAN(result);
}


void printInstanceLayers() {
    uint32_t amountOfLayers = 0;
    vkEnumerateInstanceLayerProperties(&amountOfLayers, nullptr);
    VkLayerProperties* layers = new VkLayerProperties[amountOfLayers];
    vkEnumerateInstanceLayerProperties(&amountOfLayers, layers);

    std::cout << "Amount of Instance Layers: " << amountOfLayers << std::endl;
    for (int i = 0; i < amountOfLayers; i++) {
        std::cout << std::endl;
        std::cout << "Name:         " << layers[i].layerName << std::endl;
        std::cout << "Spec Version: " << layers[i].specVersion << std::endl;
        std::cout << "Impl Version: " << layers[i].implementationVersion << std::endl;
        std::cout << "Description:  " << layers[i].description << std::endl;
    }

    delete[] layers;
}

void printInstanceExtensions() {
    uint32_t amountOfExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, nullptr);
    VkExtensionProperties* extensions = new VkExtensionProperties[amountOfExtensions];
    vkEnumerateInstanceExtensionProperties(nullptr, &amountOfExtensions, extensions);


    std::cout << std::endl;
    std::cout << "Amount of Extensions: " << amountOfExtensions << std::endl;
    for (int i = 0; i < amountOfExtensions; i++) {
        std::cout << "Name: " << extensions[i].extensionName << std::endl;
        std::cout << "Spec Version: " << extensions[i].specVersion << std::endl;
    }

    delete[] extensions;
}


void createGlfwWindowSurface() {
    VkResult result = glfwCreateWindowSurface(context.m_instance, context.m_window, nullptr, &context.m_surface);
    ASSERT_VULKAN(result);
}


std::vector<VkPhysicalDevice> getAllPhysicalDevices() {
    uint32_t amountOfPhysicalDevices = 0;
    VkResult result = vkEnumeratePhysicalDevices(context.m_instance, &amountOfPhysicalDevices, nullptr);
    ASSERT_VULKAN(result);

    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(amountOfPhysicalDevices);

    result = vkEnumeratePhysicalDevices(context.m_instance, &amountOfPhysicalDevices, physicalDevices.data());
    ASSERT_VULKAN(result);

    return physicalDevices;
}

void printStatsOfAllPhysicalDevices() {

    for (int i = 0; i < physicalDevices.size(); i++) {
        std::cout << "Device Nr: " << i << std::endl;
        printStats(physicalDevices[i]);
    }
}



void createLogicalDevice() {


    float queuePrios[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = 0; //TODO: Choose correct family index
    deviceQueueCreateInfo.queueCount = 1;       //TODO Check idf this amount is valid
    deviceQueueCreateInfo.pQueuePriorities = queuePrios;


    VkPhysicalDeviceVulkan12Features enabled_new_features = {};
    enabled_new_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enabled_new_features.bufferDeviceAddress = VK_TRUE;
    enabled_new_features.descriptorIndexing = VK_TRUE;



    VkPhysicalDeviceRayTracingFeaturesKHR usedRayTracingFeatures;
    usedRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
    usedRayTracingFeatures.pNext = &enabled_new_features;
    usedRayTracingFeatures.rayTracing = VK_FALSE;
    usedRayTracingFeatures.rayTracingShaderGroupHandleCaptureReplay = VK_FALSE;
    usedRayTracingFeatures.rayTracingShaderGroupHandleCaptureReplayMixed = VK_FALSE;
    usedRayTracingFeatures.rayTracingAccelerationStructureCaptureReplay = VK_FALSE;
    usedRayTracingFeatures.rayTracingIndirectTraceRays = VK_FALSE;
    usedRayTracingFeatures.rayTracingIndirectAccelerationStructureBuild = VK_FALSE;
    usedRayTracingFeatures.rayTracingHostAccelerationStructureCommands = VK_FALSE;
    usedRayTracingFeatures.rayQuery = VK_TRUE; // Enable ray queries
    usedRayTracingFeatures.rayTracingPrimitiveCulling = VK_FALSE;


    VkPhysicalDeviceFeatures usedFeatures = {};
    usedFeatures.samplerAnisotropy = VK_TRUE;
    usedFeatures.fragmentStoresAndAtomics = VK_TRUE;
    usedFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;



    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_EXTENSION_NAME,
        // Added from nvidia ray tracing tutorial
        //VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &usedRayTracingFeatures;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &usedFeatures;


    VkResult result = vkCreateDevice(physicalDevices[deviceIndex], &deviceCreateInfo, nullptr, &context.m_device); //civ
    ASSERT_VULKAN(result);
}



void createQueue() {
    vkGetDeviceQueue(context.m_device, 0, 0, &context.m_queue);
}


void checkSurfaceSupport() {
    VkBool32 surfaceSupport = false;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[deviceIndex], 0, context.m_surface, &surfaceSupport);
    ASSERT_VULKAN(result);
    if (!surfaceSupport) {
        std::cerr << "Surface not supported" << std::endl;
        __debugbreak();
    }
}


void createSwapChain() {
    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = context.m_surface;
    swapchainCreateInfo.minImageCount = 3; //TODO civ
    swapchainCreateInfo.imageFormat = context.m_colorFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent = VkExtent2D{ context.m_width, context.m_height };
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //TODO civ;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR; //TODO civ
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = context.m_swapchain;


    VkResult result = vkCreateSwapchainKHR(context.m_device, &swapchainCreateInfo, nullptr, &context.m_swapchain);
    ASSERT_VULKAN(result);
}



void createImageViews() {
    vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &context.m_amountOfImagesInSwapchain, nullptr);
    VkImage* swapChainImages = new VkImage[context.m_amountOfImagesInSwapchain];
    VkResult result  = vkGetSwapchainImagesKHR(context.m_device, context.m_swapchain, &context.m_amountOfImagesInSwapchain, swapChainImages);
    ASSERT_VULKAN(result);


    context.m_imageViews = new VkImageView[context.m_amountOfImagesInSwapchain];
    for (int i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        createImageView(context.m_device, swapChainImages[i], context.m_colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, context.m_imageViews[i]);

       

    }
    delete[] swapChainImages;
}

void createCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = 0; //TODO civ


    VkResult result = vkCreateCommandPool(context.m_device, &commandPoolCreateInfo, nullptr, &context.m_commandPool);
    ASSERT_VULKAN(result);
}



void createUniformBuffers()
{
    if (!(loadStatus & UNIFORM_BUFFERS_CREATED))
    {
        context.m_uniformBuffers = new VkBuffer[UNIFORM_BUFFER_COUNT];
        context.m_uniformBufferMemory = new VkDeviceMemory[UNIFORM_BUFFER_COUNT];
        VkDeviceSize bufferSize = getUniformBufferOffset(sizeof(context.m_perFrameConstants), context.m_amountOfImagesInSwapchain - 1, context.m_deviceProperties) + sizeof(context.m_perFrameConstants);
        createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, context.m_uniformBuffers[0],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.m_uniformBufferMemory[0]);

        bufferSize = sizeof(MaterialData) * scene->m_materialCount;
        createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, context.m_uniformBuffers[1],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.m_uniformBufferMemory[1]);


        bufferSize = sizeof(DecalData) * ((scene->m_decalCount == 0) ? 1 : scene->m_decalCount);
        createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, context.m_uniformBuffers[2],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.m_uniformBufferMemory[2]);

        bufferSize = sizeof(PointLight) * ((scene->m_pointLightCount == 0) ? 1 : scene->m_pointLightCount);
        //createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, context.m_uniformBuffers[3],
        createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, context.m_uniformBuffers[3],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.m_uniformBufferMemory[3]);
        loadStatus |= UNIFORM_BUFFERS_CREATED;

        bufferSize = getUniformBufferOffset(sizeof(Cursor), context.m_amountOfImagesInSwapchain - 1, context.m_deviceProperties) + sizeof(Cursor);
        createBuffer(context.m_device, physicalDevices[deviceIndex], bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, context.m_uniformBuffers[4],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, context.m_uniformBufferMemory[4]);
    }
}


void writeStaticUniformBuffers() {

    mat.clear();
    dec.clear();
    pointLightData.clear();
    if (!(loadStatus & STATIC_UNIFORM_BUFFERS_WRITTEN))
    {
        std::vector<MaterialProperties> materialProperties = materialLoader.getMaterialProperties();

        for (size_t i = 0; i < materialProperties.size(); i++)
        {
            MaterialData matData;
            matData.slotUsed = 1;
            //matData.ambient = glm::vec4(materialProperties[i].ambient, 1.0f);
            //matData.diffuse = glm::vec4(materialProperties[i].diffuse, 1.0f);
            //matData.specular = glm::vec4(materialProperties[i].specular, 1.0f);


            matData.idx_map_alpha = materialProperties[i].idx_map_alpha;
            matData.idx_map_specular = materialProperties[i].idx_map_specular;
            matData.idx_map_diffuse = materialProperties[i].idx_map_diffuse;
            matData.idx_map_normal = materialProperties[i].idx_map_normal;
            matData.idx_map_emissive = materialProperties[i].idx_map_emissive;

            mat.push_back(matData);

        }

        void* data;
        vkMapMemory(context.m_device, context.m_uniformBufferMemory[1], 0, sizeof(MaterialData) * scene->m_materialCount, 0, &data);
        memcpy(data, mat.data(), sizeof(MaterialData) * scene->m_materialCount);
        vkUnmapMemory(context.m_device, context.m_uniformBufferMemory[1]);



        std::vector<Decal> decals = scene->m_rootNode.m_decals;
        glm::mat4 inverseModelMatrix = glm::inverse(scene->m_rootNode.m_transformMatrix);

        for (size_t i = 0; i < decals.size(); i++)
        {
            DecalData decalData;

            decalData.inverseTransposedTransformation = scene->m_rootNode.m_transformMatrix*decals[i].inverseTransposedTransformation;
            //decalData.inverseTransposedTransformation = decals[i].inverseTransposedTransformation;
            decalData.inverseTransformation = decals[i].inverseTransformation*inverseModelMatrix;
            //decalData.inverseTransformation = decals[i].inverseTransformation;
            decalData.uvCoordAB = glm::vec4(decals[i].uvCoord[0], decals[i].uvCoord[1]);
            decalData.uvCoordCD = glm::vec4(decals[i].uvCoord[2], decals[i].uvCoord[3]);
            decalData.materialIndex = decals[i].materialIndex;
            decalData.channel = decals[i].channel;
            decalData.weight = decals[i].weight;

            dec.push_back(decalData);
        }

        vkMapMemory(context.m_device, context.m_uniformBufferMemory[2], 0, sizeof(DecalData) * scene->m_decalCount, 0, &data);
        memcpy(data, dec.data(), sizeof(DecalData) * scene->m_decalCount);
        vkUnmapMemory(context.m_device, context.m_uniformBufferMemory[2]);



        std::vector<PointLight> pointLights = scene->m_rootNode.m_pointLights;
        float scale = glm::length(glm::vec3(((scene->m_rootNode.m_transformMatrix * glm::vec4(1.0, 1.0, 1.0, 0.0)))));
        for (size_t i = 0; i < pointLights.size(); i++)
        {
            PointLightData light;
            light.position = scene->m_rootNode.m_transformMatrix * glm::vec4(pointLights[i].position, 1.0);
            light.color = pointLights[i].color;
            light.color.a *= scale * scale;
            pointLightData.push_back(light);
        }

        vkMapMemory(context.m_device, context.m_uniformBufferMemory[3], 0, sizeof(PointLightData) * scene->m_pointLightCount, 0, &data);
        memcpy(data, pointLightData.data(), sizeof(PointLightData) * scene->m_pointLightCount);
        vkUnmapMemory(context.m_device, context.m_uniformBufferMemory[3]);
    }
    loadStatus |= STATIC_UNIFORM_BUFFERS_WRITTEN;
}



void createDescriptorPool() {

    uint32_t setCount;
    if (renderMode & (DEFERRED|VISIBILITY_BUFFER))
    {
        // 1 set per subpass per swapchain
        setCount = 3 * context.m_amountOfImagesInSwapchain; // TODO maybe add size for inputAttachements
    }
    if (renderMode & FORWARD)
    {
        setCount = context.m_amountOfImagesInSwapchain;
    }

    VkDescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = setCount;

    VkDescriptorPoolSize materialDescriptorPoolSize;
    materialDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialDescriptorPoolSize.descriptorCount = setCount;

    VkDescriptorPoolSize decalDescriptorPoolSize;
    decalDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    decalDescriptorPoolSize.descriptorCount = setCount;

    VkDescriptorPoolSize pointLightDescriptorPoolSize;
    //pointLightDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pointLightDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorPoolSize.descriptorCount = setCount;

    VkDescriptorPoolSize accelerationStructureDescriptorSize;
    accelerationStructureDescriptorSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureDescriptorSize.descriptorCount = setCount;

    VkDescriptorPoolSize accelerationStructureSceneDescriptorSize;
    accelerationStructureSceneDescriptorSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureSceneDescriptorSize.descriptorCount = setCount;

    VkDescriptorPoolSize accelerationStructurePointLightDescriptorSize;
    accelerationStructurePointLightDescriptorSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructurePointLightDescriptorSize.descriptorCount = setCount;

    VkDescriptorPoolSize samplerPoolSize;
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = setCount;

    std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    descriptorPoolSizes.push_back(descriptorPoolSize);
    descriptorPoolSizes.push_back(samplerPoolSize);
    descriptorPoolSizes.push_back(materialDescriptorPoolSize);
    descriptorPoolSizes.push_back(decalDescriptorPoolSize);
    descriptorPoolSizes.push_back(accelerationStructureDescriptorSize);
    descriptorPoolSizes.push_back(accelerationStructureSceneDescriptorSize);
    descriptorPoolSizes.push_back(pointLightDescriptorPoolSize);
    descriptorPoolSizes.push_back(accelerationStructurePointLightDescriptorSize);

    if (renderMode & VISIBILITY_BUFFER)
    {
        for (size_t i = 0; i < VERTEX_ATTRIBUTE_COUNT; i++)
        {
            VkDescriptorPoolSize vertexAttrbutesPoolSize;
            vertexAttrbutesPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            vertexAttrbutesPoolSize.descriptorCount = setCount;

            descriptorPoolSizes.push_back(vertexAttrbutesPoolSize);
        }
        VkDescriptorPoolSize decalCursorDescriptorPoolSize;
        decalCursorDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        decalCursorDescriptorPoolSize.descriptorCount = setCount;

        descriptorPoolSizes.push_back(decalCursorDescriptorPoolSize);
    }

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = setCount;
    descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

    VkResult result = vkCreateDescriptorPool(context.m_device, &descriptorPoolCreateInfo, nullptr, &context.m_descriptorPool);
    ASSERT_VULKAN(result);
}

void createFences()
{
    
    context.m_fences = new VkFence[context.m_amountOfImagesInSwapchain];

    for (size_t i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkResult result = vkCreateFence(context.m_device, &fenceCreateInfo, nullptr, &(context.m_fences[i]));
        ASSERT_VULKAN(result);
    }
}

void recreateSwapChain() {
    VkResult result;

    vkDeviceWaitIdle(context.m_device);


    if (renderMode & FORWARD)
    {
        forwardRenderer->destroyForwardPass();
    }
    if (renderMode & DEFERRED)
    {
        deferredRenderer->destroyDeferredPass();
    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer->destroyVbPass();
    }

    for (size_t i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        result = vkResetFences(context.m_device, 1, &(context.m_fences[i]));
        ASSERT_VULKAN(result);
        vkDestroyFence(context.m_device, context.m_fences[i], nullptr);
    }
    for (int i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyImageView(context.m_device, context.m_imageViews[i], nullptr);
    }
    delete[] context.m_imageViews;
    VkSwapchainKHR oldSwapchain = context.m_swapchain;
    createSwapChain();
    createImageViews();

    if (renderMode & FORWARD)
    {
        forwardRenderer->restoreForwardPass();
    }
    if (renderMode & DEFERRED)
    {
        deferredRenderer->restoreDeferredPass();
    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer->restoreVbPass();
    }

    vkDestroySwapchainKHR(context.m_device, oldSwapchain, nullptr);
    createFences();
    frameCounterTime = std::chrono::high_resolution_clock::now();

    context.m_imageUpdateIndex = 0;

}

void initialiseVulkanContext()
{
    createInstance();
    physicalDevices = getAllPhysicalDevices();
    context.m_physicalDevice = physicalDevices[deviceIndex];

    

    createGlfwWindowSurface();
    getDeviceFeatures();
#ifdef _DEBUG
    //printStats(physicalDevices[deviceIndex]);
    printInstanceLayers();
    printInstanceExtensions();
    printStatsOfAllPhysicalDevices();
#endif // _DEBUG

    printStatsOfAllPhysicalDevices();

    createLogicalDevice();
    createQueue();
    checkSurfaceSupport();
    createSwapChain();
    createImageViews();
    createCommandPool();
    createDescriptorPool();
    createFences();
}

// Tichy would cry! But just work.tm
void recompileShaders()
{
    if (renderMode & FORWARD)
    {
        forwardRenderer->shaderHotReload();
    }
    if (renderMode & DEFERRED)
    {
        deferredRenderer->shaderHotReload();
    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer->shaderHotReload();
    }
}

void load()
{
    std::cout << "Reloading..." << std::endl;
    std::cout << "Cam Position: " << cam.pos.x << " ," << cam.pos.y << " ," << cam.pos.z << std::endl;
    std::cout << "Cam Pitch: " << cam.pitch << std::endl;
    std::cout << "Cam Yaw: " << cam.yaw << std::endl;
    if (renderMode & DEFERRED)
    {
        deferredRenderer = new DeferredRenderer(&context, scene, flags);
        deferredRenderer->createRenderPassDeferred();
        scene->loadMesh();
        deferredRenderer->createDescriptorSetLayoutDeferredGeometry();
        if (flags & DEFERRED_DECALS)
        {
            deferredRenderer->createDescriptorSetLayoutDeferredDecal();
        }
        deferredRenderer->createDescriptorSetLayoutDeferredShading();
        deferredRenderer->compileShader();
        deferredRenderer->createPipelineDeferredGeometry();
        if (flags & DEFERRED_DECALS)
        {
            deferredRenderer->createPipelineDeferredDecal();
        }
        deferredRenderer->createPipelineDeferredShading();
        deferredRenderer->createGBuffer();
        deferredRenderer->createFramebuffersDeferred();
        deferredRenderer->createCommandBuffers();
        scene->loadMaterials();
        scene->createVertexBuffer();
        scene->createIndexBuffer();

        if (flags & DEFERRED_DECALS)
        {
            scene->createDecalVertexBuffer();
            scene->createDecalIndexBuffer();
        }
        scene->createAccelerationStructure();
        createUniformBuffers();
        deferredRenderer->createDescriptorSetsDeferredGeometryPass();
        if (flags & DEFERRED_DECALS)
        {
            deferredRenderer->createDescriptorSetsDeferredDecalPass();
        }
        deferredRenderer->createDescriptorSetsDeferredShadingPass();
        initializePerFrameConstants();
        writeStaticUniformBuffers();
        deferredRenderer->recordCommandBuffersDeferred();
        deferredRenderer->createSemaphores();
    }
    if (renderMode & FORWARD)
    {
        forwardRenderer = new ForwardRenderer(&context, scene);
        forwardRenderer->createRenderPass();
        forwardRenderer->createDepthPass();
        scene->loadMesh();
        forwardRenderer->createDescriptorSetLayout();
        forwardRenderer->compileShader();
        forwardRenderer->createPipeline();
        forwardRenderer->createDepthPassPipeline();
        forwardRenderer->createDepthImage();
        forwardRenderer->createFramebuffers();
        forwardRenderer->createFramebuffersDepthPass();
        forwardRenderer->createCommandBuffers();
        forwardRenderer->createDepthPassCommandBuffers();
        scene->loadMaterials();
        scene->createVertexBuffer();
        scene->createIndexBuffer();

        scene->createAccelerationStructure();
        createUniformBuffers();
        forwardRenderer->createDescriptorSets();
        initializePerFrameConstants();
        writeStaticUniformBuffers();
        forwardRenderer->recordCommandBuffers();
        forwardRenderer->recordDepthPassCommandBuffers();
        forwardRenderer->createSemaphores();

    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer = new VbRenderer(&context, scene);
        vbRenderer->createRenderPassVb();
        scene->loadMesh();
        vbRenderer->createDescriptorSetLayoutVbGeometry();
        vbRenderer->createDescriptorSetLayoutVbShading();
        vbRenderer->compileShader();
        vbRenderer->createPipelineVbGeometry();
        vbRenderer->createPipelineVbShading();
        vbRenderer->createVisibilityBuffer();
        vbRenderer->createDepthImage();
        vbRenderer->createFramebuffersVb();
        vbRenderer->createCommandBuffers();
        scene->loadMaterials();
        scene->createVertexAttributeBuffersBuffers();
        scene->createIndexBuffer();
        // Destroy if exist: acceleration Structure, uniform buffers, descriptor sets, command buffers,
        // Add: Decals to node
        scene->createAccelerationStructure();
        createUniformBuffers();
        vbRenderer->createDescriptorSetsVbGeometryPass();
        vbRenderer->createDescriptorSetsVbShadingPass();
        initializePerFrameConstants();
        writeStaticUniformBuffers();
        vbRenderer->recordCommandBuffersVb();
        vbRenderer->createSemaphores();
    }
    std::cout << "Light Count: " << scene->m_pointLightCount<<std::endl;
    std::cout << "Decal Count: " << scene->m_decalCount<<std::endl;
    std::cout << "Width: " << context.m_width << std::endl;
    std::cout << "Height: " << context.m_height << std::endl;
    
}



void changeRenderMode(RenderMode newRenderMode, uint32_t newFlags)
{
    vkDeviceWaitIdle(context.m_device);
    vkDestroyDescriptorPool(context.m_device, context.m_descriptorPool, nullptr);
    for (size_t i = 0; i < UNIFORM_BUFFER_COUNT; i++)
    {
        vkFreeMemory(context.m_device, context.m_uniformBufferMemory[i], nullptr);
        vkDestroyBuffer(context.m_device, context.m_uniformBuffers[i], nullptr);
    }
    if (renderMode & DEFERRED)
    {
        deferredRenderer->destroy();
    }
    if (renderMode & FORWARD)
    {
        forwardRenderer->destroy();
    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer->destroy();
    }
    renderMode = newRenderMode;
    flags = newFlags;
    createDescriptorPool();
    uint32_t mask = ~(UNIFORM_BUFFERS_CREATED | STATIC_UNIFORM_BUFFERS_WRITTEN);
    loadStatus = loadStatus & (~(UNIFORM_BUFFERS_CREATED | STATIC_UNIFORM_BUFFERS_WRITTEN));
    load();
}

void startVulkan() {


    initialiseVulkanContext();
    scene = new Scene(&context, sceneInfo, &meshLoader, &materialLoader, &decalLoader, &pointLightLoader, &asBuilder);
    load();
}

auto gameStartTime = std::chrono::high_resolution_clock::now();

void update() {
    VkResult result;
    result = vkWaitForFences(context.m_device, 1, &(context.m_fences[context.m_imageRenderIndex]), true, UINT64_MAX);
    ASSERT_VULKAN(result);
    result = vkResetFences(context.m_device, 1, &(context.m_fences[context.m_imageRenderIndex]));
    ASSERT_VULKAN(result);
    //---------------------------------------------------- Update Per frame constants------------------------------------
    frameCounter += 1;
    if ((frameCounter%200) == 0)
    {
        frameDeltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(// nano sec
            std::chrono::high_resolution_clock::now() - frameCounterTime).count() / (200 * pow(10.0,9));
        frameCounterTime = std::chrono::high_resolution_clock::now();
        std::string windowName = "Vulkan Renderer";
        windowName.append(" (");
        windowName.append(std::to_string(frameDeltaTime * 1000.0f));
        windowName.append(" ms)");
        windowName.append("(Max: ");
        windowName.append(std::to_string(maxFrameTime * 1000.0f));
        windowName.append(" ms)");
        if (renderMode & FORWARD) windowName.append(" - [FORWARD]");
        if ((renderMode & DEFERRED)&&(flags ^ DEFERRED_DECALS)) windowName.append(" - [DEFERRED]");
        if ((renderMode & DEFERRED) && (flags & DEFERRED_DECALS)) windowName.append(" - [DEFERRED DECALS]");
        if (renderMode & VISIBILITY_BUFFER) windowName.append(" - [VISIBILITY_BUFFER]");
        windowName.append(createShaderFeatureFlagsWindowName(context.shaderFeatureFlags));
        glfwSetWindowTitle(context.m_window, windowName.c_str());
        maxFrameTime = 0;
    }
    std::chrono::steady_clock::time_point newFrameTime = std::chrono::high_resolution_clock::now();
    maxFrameTime = std::max(maxFrameTime, std::chrono::duration_cast<std::chrono::nanoseconds>(newFrameTime - frameTime).count() / pow(10.0, 9));
    frameTime = newFrameTime;
    timeSinceStart = std::chrono::duration_cast<std::chrono::nanoseconds>(frameTime - gameStartTime).count() / pow(10.0, 9);

    //glm::mat4 model = glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f)), timeSinceStart * glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3( 0.0f, 0.0f, 0.0f));
    glm::mat4 model = scene->m_rootNode.m_transformMatrix;//glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f)), glm::vec3( 0.0f, 0.0f, 0.0f));
    glm::mat4 view = glm::lookAt(cam.pos, cam.pos + cam.front, cam.up);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), context.m_width / (float)context.m_height, 0.01f, 10.0f);
    projection[1][1] *= -1;

    glm::mat4 modelInverse = glm::inverse(model);
    context.m_perFrameConstants.lightPositionModel = modelInverse * context.m_perFrameConstants.lightPosition;
    context.m_perFrameConstants.decalCount = scene->m_decalCount;
    context.m_perFrameConstants.model = model;
    context.m_perFrameConstants.view = view;
    context.m_perFrameConstants.projection = projection;
    context.m_perFrameConstants.camPos = glm::vec4(cam.pos, 1.0);
    context.m_perFrameConstants.camPosModel = modelInverse * context.m_perFrameConstants.camPos;
    context.m_perFrameConstants.inverseModel = glm::inverse(model);
    context.m_perFrameConstants.inverseView = glm::inverse(view);
    context.m_perFrameConstants.inverseProjection = glm::inverse(projection);
    context.m_perFrameConstants.width = context.m_width;
    context.m_perFrameConstants.height = context.m_height;

    void* data;
    uint32_t offset = getUniformBufferOffset(sizeof(context.m_perFrameConstants), context.m_imageRenderIndex, context.m_deviceProperties);
    vkMapMemory(context.m_device, context.m_uniformBufferMemory[0], offset,
        sizeof(context.m_perFrameConstants), 0, &data);
    memcpy(data, &context.m_perFrameConstants, sizeof(context.m_perFrameConstants));
    vkUnmapMemory(context.m_device, context.m_uniformBufferMemory[0]);

    cursor.decal.inverseTransformation = glm::mat4(-2.0f * decalScale * view[0], -2.0f * decalScale * view[1], -2.0f * decalScale * view[2], glm::vec4(-2.0f * decalScale * view[3].x, -2.0f * decalScale * view[3].y, -2.0f * decalScale * view[3].z, 1.0));
    cursor.decal.inverseTransformation = glm::translate(cursor.decal.inverseTransformation, glm::vec3(glm::inverse(view)*glm::vec4(-0.25 * (1.0/decalScale),-0.25 * (1.0 / decalScale),0.5,0.0)));
    cursor.decal.inverseTransposedTransformation = glm::transpose(cursor.decal.inverseTransformation);

    cursor.light.position = glm::vec4(cam.pos + 0.5f*cam.front,1.0);

    offset = getUniformBufferOffset(sizeof(Cursor), context.m_imageRenderIndex, context.m_deviceProperties);
    vkMapMemory(context.m_device, context.m_uniformBufferMemory[4], offset,
        sizeof(Cursor), 0, &data);
    memcpy(data, &cursor, sizeof(Cursor));
    vkUnmapMemory(context.m_device, context.m_uniformBufferMemory[4]);

    //---------------------------------------------------------------------------------------------------------------------
    //context.m_imageUpdateIndex = (context.m_imageUpdateIndex + 1) % context.m_amountOfImagesInSwapchain;
}

void processInput(GLFWwindow* window)
{
    const float cameraTurnSpeed = cam_turn_speed * (frameDeltaTime);
    const float cameraSpeed = cam_speed * (frameDeltaTime);
    const float lightSpeed = LIGHT_SPEED * (frameDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.pos += cameraSpeed * cam.front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.pos -= cameraSpeed * cam.front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.pos -= glm::normalize(glm::cross(cam.front, cam.up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.pos += glm::normalize(glm::cross(cam.front, cam.up)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cam.pos -= cameraSpeed * cam.up;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cam.pos += cameraSpeed * cam.up;


    if (glfwGetKey(window, GLFW_KEY_KP_8) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition += lightSpeed * glm::vec4(cam.front, 0);
    if (glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition -= lightSpeed * glm::vec4(cam.front, 0);
    if (glfwGetKey(window, GLFW_KEY_KP_4) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition -= lightSpeed * glm::vec4(glm::normalize(glm::cross(cam.front, cam.up)), 0);
    if (glfwGetKey(window, GLFW_KEY_KP_6) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition += lightSpeed * glm::vec4(glm::normalize(glm::cross(cam.front, cam.up)), 0);
    if (glfwGetKey(window, GLFW_KEY_KP_7) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition -= lightSpeed * glm::vec4(cam.up, 0);
    if (glfwGetKey(window, GLFW_KEY_KP_9) == GLFW_PRESS)
        context.m_perFrameConstants.lightPosition += lightSpeed * glm::vec4(cam.up, 0);
    if (glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS)
        context.m_perFrameConstants.lightColor.a *= DELTA_LIGHT_COLOR;
    if (glfwGetKey(window, GLFW_KEY_KP_3) == GLFW_PRESS)
        context.m_perFrameConstants.lightColor.a /= DELTA_LIGHT_COLOR;


    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cam.yaw += cameraTurnSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cam.yaw -= cameraTurnSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cam.pitch += cameraTurnSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cam.pitch -= cameraTurnSpeed;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {

        if(!tab_Pressed) //context.m_perFrameConstants.counter++;
        tab_Pressed = true;
    }
        
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
        tab_Pressed = false;

    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS)
    {
        if (renderMode & FORWARD)
        {
            forwardRenderer->shaderHotReload();
        }
        if (renderMode & DEFERRED)
        {
            deferredRenderer->shaderHotReload();
        }
        if (renderMode & VISIBILITY_BUFFER)
        {
            vbRenderer->shaderHotReload();
        }
    }


    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        changeRenderMode(FORWARD, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        changeRenderMode(DEFERRED, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
    {
        changeRenderMode(DEFERRED, DEFERRED_DECALS);
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
    {
        changeRenderMode(VISIBILITY_BUFFER, 0);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_RAY_QUERY_DECALS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    {
        context.shaderFeatureFlags = (context.shaderFeatureFlags & ENABLE_RAY_QUERY_LIGHTS)?
            context.shaderFeatureFlags^ENABLE_RAY_QUERY_LIGHTS : context.shaderFeatureFlags | ENABLE_RAY_QUERY_LIGHTS | ENABLE_SCENE_POINT_LIGHTS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_RAY_QUERY_DECAL_AABBS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_RAY_QUERY_SHADOWS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_RAY_QUERY_REFLECTIONS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_AMBIENT_TERM;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_SCENE_POINT_LIGHTS;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_PRIMARY_POINT_LIGHT;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_EMISSIVE_TERM;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
    {
        context.shaderFeatureFlags ^= ENABLE_EDITOR;
        recompileShaders();
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        if (cursor.selection == 0)
        {
            glm::mat4 transformation = cursor.decal.inverseTransformation * scene->m_rootNode.m_transformMatrix;
            decalLoader.generateDecal(glm::inverse(transformation), cursor.decal.materialIndex, scene->m_rootNode, cursor.decal.weight, cursor.decal.channel);
            scene->m_decalCount++;
            vkDeviceWaitIdle(context.m_device);
            scene->recreateDecalAccelerationStructure();
            changeRenderMode(renderMode, flags);
        }
        if (cursor.selection == 1)
        {
            float scale = glm::length(glm::vec3(((scene->m_rootNode.m_transformMatrix * glm::vec4(1.0, 1.0, 1.0, 0.0)))));
            glm::vec4 color = cursor.light.color;
            color.a /= scale * scale;
            pointLightLoader.generatePointLight(glm::vec3(context.m_perFrameConstants.inverseModel*glm::vec4(cursor.light.position)),
                color, scene->m_rootNode);
            scene->m_pointLightCount++;
            vkDeviceWaitIdle(context.m_device);
            scene->recreateLightAccelerationStructure();
            changeRenderMode(renderMode, flags);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) t_Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !t_Pressed)
    {
        if (cursor.selection == 0)
        {
            currentSelectedDecalAttribute = (currentSelectedDecalAttribute + 1) % DECAL_CURSOR_ATTRIBUTE_COUNT;
        }
        else if (cursor.selection == 1) {
            currentSelectedLigthAttribute = (currentSelectedLigthAttribute + 1) % LIGHT_CURSOR_ATTRIBUTE_COUNT;
        }
        t_Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE) f_Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !f_Pressed)
    {
        cursor.selection = (cursor.selection + 1) % 3;
        f_Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE) e_Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && !e_Pressed)
    {
        if (cursor.selection == 0) {
            if (currentSelectedDecalAttribute == DECAL_MATERIAL)
            {
                currentSelectedDecal = ((currentSelectedDecal + 1) % materialLoader.getDecalMaterialIndices().size());
                cursor.decal.materialIndex = materialLoader.getDecalMaterialIndices()[currentSelectedDecal];
            }
            if (currentSelectedDecalAttribute == DECAL_LAYER)
            {
                cursor.decal.channel = (cursor.decal.channel + 1) % 4;
            }
            if (currentSelectedDecalAttribute == DECAL_WEIGTH)
            {
                cursor.decal.weight *= 2;
            }
        }
        if (cursor.selection == 2) {
            cam_speed += 0.1;
            cam_turn_speed += 5;
        }
        e_Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        if (currentSelectedDecalAttribute == DECAL_SCALE && cursor.selection == 0)
        {
            decalScale = (decalScale > 0.01) ? decalScale * 0.99 : 0.01;
        }
        if (cursor.selection == 1)
        {
            if (currentSelectedLigthAttribute == LIGHT_R)
            {
                cursor.light.color.r = (cursor.light.color.r < 254.0/255.0)?cursor.light.color.r+1.0/255.0:1.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_G)
            {
                cursor.light.color.g = (cursor.light.color.g < 254.0 / 255.0) ? cursor.light.color.g + 1.0 / 255.0 : 1.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_B)
            {
                cursor.light.color.b = (cursor.light.color.b < 254.0 / 255.0) ? cursor.light.color.b + 1.0 / 255.0 : 1.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_A)
            {
                cursor.light.color.a = cursor.light.color.a*1.01;
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_RELEASE) q_Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && !q_Pressed)
    {
        if (cursor.selection == 0) {
            if (currentSelectedDecalAttribute == DECAL_MATERIAL)
            {
                currentSelectedDecal = ((materialLoader.getDecalMaterialIndices().size() + currentSelectedDecal - 1) % materialLoader.getDecalMaterialIndices().size());
                cursor.decal.materialIndex = materialLoader.getDecalMaterialIndices()[currentSelectedDecal];
            }
            if (currentSelectedDecalAttribute == DECAL_LAYER)
            {
                cursor.decal.channel = (4 + cursor.decal.channel - 1) % 4;
            }
            if (currentSelectedDecalAttribute == DECAL_WEIGTH)
            {
                cursor.decal.weight *= 0.5;
            }
        }
        if (cursor.selection == 2) {
            cam_speed -= 0.1;
            cam_turn_speed -= 5;
        }
        q_Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (currentSelectedDecalAttribute == DECAL_SCALE && cursor.selection == 0)
        {
            decalScale = (decalScale < 10.0) ? decalScale * 1.01 : 10.0;
        }
        if (cursor.selection == 1)
        {
            if (currentSelectedLigthAttribute == LIGHT_R)
            {
                cursor.light.color.r = (cursor.light.color.r > 1.0 / 255.0) ? cursor.light.color.r - 1.0 / 255.0 : 0.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_G)
            {
                cursor.light.color.g = (cursor.light.color.g > 1.0 / 255.0) ? cursor.light.color.g - 1.0 / 255.0 : 0.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_B)
            {
                cursor.light.color.b = (cursor.light.color.b > 1.0 / 255.0) ? cursor.light.color.b - 1.0 / 255.0 : 0.0;
            }
            if (currentSelectedLigthAttribute == LIGHT_A)
            {
                cursor.light.color.a = cursor.light.color.a * 0.99;
            }
        }
    }

    if (cam.pitch > 89.0f)
        cam.pitch = 89.0f;
    if (cam.pitch < -89.0f)
        cam.pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    direction.z = sin(glm::radians(cam.pitch));
    direction.y = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    cam.front = glm::normalize(direction);


}

void gameLoop() {
    while (!glfwWindowShouldClose(context.m_window))
    {
        glfwPollEvents();
        processInput(context.m_window);


        VkResult result = vkAcquireNextImageKHR(context.m_device, context.m_swapchain, std::numeric_limits<uint64_t>::max(), context.semaphoreImageAvailable, VK_NULL_HANDLE, &context.m_imageRenderIndex);
        ASSERT_VULKAN(result);
        update();
        if (renderMode & FORWARD)
        {
            forwardRenderer->drawFrame();
        }
        if (renderMode & DEFERRED)
        {
            deferredRenderer->drawFrameDeferred();
        }
        if (renderMode & VISIBILITY_BUFFER)
        {
            vbRenderer->drawFrameVb();
        }
    }
}


void shutdownVulkan() {
    vkDeviceWaitIdle(context.m_device);
    scene->destroy();
    vkDestroyDescriptorPool(context.m_device, context.m_descriptorPool, nullptr);
    for (size_t i = 0; i < UNIFORM_BUFFER_COUNT; i++)
    {
        vkFreeMemory(context.m_device, context.m_uniformBufferMemory[i], nullptr);
        vkDestroyBuffer(context.m_device, context.m_uniformBuffers[i], nullptr);
    }
    for (size_t i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFence(context.m_device, context.m_fences[i], nullptr);
    }

    if (renderMode & DEFERRED)
    {
        deferredRenderer->destroy();
    }
    if (renderMode & FORWARD)
    {
        forwardRenderer->destroy();
    }
    if (renderMode & VISIBILITY_BUFFER)
    {
        vbRenderer->destroy();
    }

    vkDestroyCommandPool(context.m_device, context.m_commandPool, nullptr);
    for (int i = 0; i < context.m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyImageView(context.m_device, context.m_imageViews[i], nullptr);
    }
    delete[] context.m_imageViews;
    vkDestroySwapchainKHR(context.m_device, context.m_swapchain, nullptr);
    vkDestroyDevice(context.m_device, nullptr);
    vkDestroySurfaceKHR(context.m_instance, context.m_surface, nullptr);
    vkDestroyInstance(context.m_instance, nullptr);
}

void shutdownGlfw() {
    glfwDestroyWindow(context.m_window);
    glfwTerminate();
}

int main()
{
    context.shaderFeatureFlags = 0;
    //sceneInfo = sceneInfos[0]; // Emerald square (Currently not in repo)
    //sceneInfo = sceneInfos[2]; // Bistro (Currently not in repo)
    sceneInfo = sceneInfos[1]; // Suntemple
    renderMode = FORWARD;
    context.shaderFeatureFlags = ENABLE_RAY_QUERY_DECALS | ENABLE_RAY_QUERY_LIGHTS | ENABLE_PRIMARY_POINT_LIGHT;


    // Controls
    // Movement: W,A,S,D,Crtl,Space
    // To change renderers at runtime:
    // X: forward
    // C: deferred
    // V: deferred with deferred decals
    // B: visibility buffer

    // To enable/disable features:
    // 1,2,3...9,0

    // Current configuration is displayed in title bar

    // Move primary Light source / change intensity: Numpad

    // Editor:
    // Toggle On, Off: 0
    // Toggle Decals, Lights, Camera speed: F
    // Toggle Decal Type/Light intensity, Decal Layer/ Light R, Decal Weight/ Light G, Decal Size/ Light B: T
    // Increas: Q
    // Decreas: E



    

    flags = 0;


    startGlfw();
    startVulkan();
    gameLoop();
    shutdownVulkan();
    shutdownGlfw();

    return 0;
}

