/*
 * Vulkan Example base class
 *
 * Copyright (C) by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 *
 * Modified by Huawei, we Fix the bugs when minimize the windows
 *  * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan show hit render pipeline head file
 */

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "SaschaWillemsVulkan/VulkanAndroid.h"
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
#include <directfb.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <numeric>
#include <array>

#include "vulkan/vulkan.h"

#include "SaschaWillemsVulkan/keycodes.hpp"
#include "SaschaWillemsVulkan/VulkanTools.h"
#include "SaschaWillemsVulkan/VulkanDebug.h"
#include "SaschaWillemsVulkan/VulkanUIOverlay.h"
#include "SaschaWillemsVulkan/VulkanSwapChain.h"
#include "SaschaWillemsVulkan/VulkanBuffer.h"
#include "SaschaWillemsVulkan/VulkanDevice.h"
#include "SaschaWillemsVulkan/VulkanTexture.h"
#include "SaschaWillemsVulkan/VulkanInitializers.hpp"
#include "SaschaWillemsVulkan/camera.hpp"
#include "SaschaWillemsVulkan/benchmark.hpp"
#include "VulkanShader.h"
#include "VulkanRenderPass.h"

class CommandLineParser {
public:
    struct CommandLineOption {
        std::vector<std::string> commands;
        std::string value;
        bool hasValue = false;
        std::string help;
        bool set = false;
    };
    std::unordered_map<std::string, CommandLineOption> options;
    CommandLineParser();
    ~CommandLineParser() = default;
    void add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help);
    void printHelp();
    void parse(std::vector<const char *> arguments);
    bool isSet(std::string name);
    std::string getValueAsString(std::string name, std::string defaultValue);
    int32_t getValueAsInt(std::string name, int32_t defaultValue);
};

class VulkanExampleBase {
public:
    bool prepared = false;
    bool resized = false;
    uint32_t width = 1280;
    uint32_t height = 720;

    vks::UIOverlay UIOverlay;
    CommandLineParser commandLineParser;

    /** @brief Last frame time measured using a high performance timer (if available) */
    float frameTimer = 1.0f;

    vks::Benchmark benchmark;

    /** @brief Encapsulated physical and logical vulkan device */
    vks::VulkanDevice *vulkanDevice;

    /** @brief Example settings that can be changed e.g. by command line arguments */
    struct Settings {
        /** @brief Activates validation layers (and message output) when set to true */
        bool validation = false;
        /** @brief Set to true if fullscreen mode has been requested via command line */
        bool fullscreen = false;
        /** @brief Set to true if v-sync will be forced for the swapchain */
        bool vsync = false;
        /** @brief Enable UI overlay */
        bool overlay = false;
    } settings;

    VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

    static std::vector<const char *> args;

    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;
    bool paused = false;

    Camera camera;
    glm::vec2 mousePos;

    std::string title = "Vulkan Example";
    std::string name = "vulkanExample";
    uint32_t apiVersion = VK_API_VERSION_1_0;

    struct {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depthStencil;

    struct {
        glm::vec2 axisLeft = glm::vec2(0.0f);
        glm::vec2 axisRight = glm::vec2(0.0f);
    } gamePadState;

    struct {
        bool left = false;
        bool right = false;
        bool middle = false;
    } mouseButtons;

    // OS specific
#if defined(_WIN32)
    HWND window;
    HINSTANCE windowInstance;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    // true if application has focused, false if moved to background
    bool focused = false;
    struct TouchPos {
        int32_t x;
        int32_t y;
    } touchPos;
    bool touchDown = false;
    double touchTimer = 0.0;
    int64_t lastTapTime = 0;
#endif

    VulkanExampleBase(bool enableValidation = false);
    virtual ~VulkanExampleBase() noexcept;
    /** @brief Setup the vulkan instance, enable required extensions and connect to the physical device (GPU) */
    bool initVulkan();

#if defined(_WIN32)
    void setupConsole(std::string title);
    void setupDPIAwareness();
    HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
    void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    static int32_t handleAppInput(struct android_app *app, AInputEvent *event);
    static void handleAppCommand(android_app *app, int32_t cmd);
#endif
    /** @brief (Virtual) Creates the application wide Vulkan instance */
    virtual VkResult createInstance(bool enableValidation);
    /** @brief (Pure virtual) Render function to be implemented by the sample application */
    virtual void render() = 0;
    /** @brief (Virtual) Called when the camera view has changed */
    virtual void viewChanged();
    /** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
    virtual void keyPressed(uint32_t);
    /** @brief (Virtual) Called after the mouse cursor moved and before internal events (like camera rotation) is
     * handled */
    virtual void mouseMoved(double x, double y, bool &handled);
    /** @brief (Virtual) Called when the window has been resized, can be used by the sample application to recreate
     * resources */
    virtual void windowResized();
    /** @brief (Virtual) Called when resources have been recreated that require a rebuild of the command buffers (e.g.
     * frame buffer), to be implemented by the sample application */
    virtual void buildCommandBuffers();
    /** @brief (Virtual) Setup default depth and stencil views */
    virtual void setupDepthStencil();
    /** @brief (Virtual) Setup default framebuffers for all requested swapchain images */
    virtual void setupFrameBuffer();
    /** @brief (Virtual) Setup a default renderpass */
    virtual void setupRenderPass();
    /** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable
     * on the device */
    virtual void getEnabledFeatures();

    /** @brief Prepares all Vulkan resources and functions required to run the sample */
    virtual void prepare();

    /** @brief Entry point for the main render loop */
    void renderLoop();

    /** @brief Adds the drawing commands for the ImGui overlay to the given command buffer */
    void drawUI(const VkCommandBuffer commandBuffer);

    /** Prepare the next frame for workload submission by acquiring the next swap chain image */
    void prepareFrame();
    /** @brief Presents the current image to the swap chain */
    void submitFrame(useconds_t waits = 0);
    /** @brief (Virtual) Default image acquire + submission and command buffer submission function */
    virtual void renderFrame();

    /** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
    virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay);

    /* @brief (Virtual) Called during func `next frame` */
    virtual void OnNextFrame();

protected:
    // Frame counter to display fps
    uint32_t frameCounter = 0;
    uint32_t lastFPS = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;
    // Vulkan instance, stores all per-application states
    VkInstance instance;
    std::vector<std::string> supportedInstanceExtensions;
    // Physical device (GPU) that Vulkan will use
    VkPhysicalDevice physicalDevice;
    // Stores physical device properties (for e.g. checking device limits)
    VkPhysicalDeviceProperties deviceProperties;
    // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
    VkPhysicalDeviceFeatures deviceFeatures;
    // Stores all available memory (type) properties for the physical device
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    /** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor)
     */
    VkPhysicalDeviceFeatures enabledFeatures{};
    /** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
    std::vector<const char *> enabledDeviceExtensions;
    std::vector<const char *> enabledInstanceExtensions;
    /** @brief Optional pNext structure for passing extension structures to device creation */
    void *deviceCreatepNextChain = nullptr;
    /** @brief Logical device, application's view of the physical device (GPU) */
    VkDevice device;
    // Handle to the device graphics queue that command buffers are submitted to
    VkQueue queue;
    // Depth buffer format (selected during Vulkan initialization)
    VkFormat depthFormat;
    // Command buffer pool
    VkCommandPool cmdPool;
    /** @brief Pipeline stages used to wait at for graphics queue submissions */
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // Command buffers used for rendering
    std::vector<VkCommandBuffer> drawCmdBuffers;
    // Global render pass for frame buffer writes
    VkRenderPass renderPass = VK_NULL_HANDLE;
    // List of available frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer> frameBuffers;
    // Active frame buffer index
    uint32_t currentBuffer = 0;
    // Descriptor set pool
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    // Pipeline cache object
    VkPipelineCache pipelineCache;
    // Wraps the swap chain to present images (framebuffers) to the windowing system
    VulkanSwapChain swapChain;
    // Synchronization semaphores
    struct Semaphore {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
    };
    std::vector<Semaphore> semaphores;
    std::vector<VkFence> waitFences;
    std::vector<VkFence> imageFences;
    uint32_t frameIdx = 0;

    void windowResize();

private:
    std::string getWindowTitle();
    bool viewUpdated = false;
    uint32_t destWidth;
    uint32_t destHeight;
    bool resizing = false;
    void handleMouseMove(int32_t x, int32_t y);
    void nextFrame();
    void updateOverlay();
    void createPipelineCache();
    void createCommandPool();
    void createSynchronizationPrimitives();
    void initSwapchain();
    void setupSwapChain();
    void createCommandBuffers();
    void destroyCommandBuffers();
    void waitGPUIdle();
    std::vector<const char *> getInstanceExtensions();
    void parseCommandArgs();
    bool getPhysicDevice();
#if defined(_WIN32)
    void winRender();
    HWND adjustWindows(HINSTANCE hinstance, RECT& outWindowRect);
    void keyDown(WPARAM wParam);
#elif(VK_USE_PLATFORM_ANDROID_KHR)
    void androidRender();
    void androidRenderFrame();
#endif
    std::string shaderDir = "glsl";
    uint32_t m_maxFrameInFlight = 1;
};

// OS specific macros for the example main entry points
#if defined(_WIN32)
// Windows entry point
#define VULKAN_EXAMPLE_MAIN(ExampleClassName)                                                    \
    ExampleClassName *vulkanExample;                                                \
    LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
    {                                                                            \
        if (vulkanExample != NULL) {                                             \
            vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);           \
        }                                                                        \
        return (DefWindowProc(hWnd, uMsg, wParam, lParam));                      \
    }                                                                            \
    int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)             \
    {                                                                            \
        for (int32_t i = 0; i < __argc; i++) {                                   \
            ExampleClassName::args.push_back(__argv[i]);                            \
        };                                                                       \
        vulkanExample = new ExampleClassName();                                     \
        vulkanExample->initVulkan();                                             \
        vulkanExample->setupWindow(hInstance, WndProc);                          \
        vulkanExample->prepare();                                                \
        vulkanExample->renderLoop();                                             \
        delete (vulkanExample);                                                  \
        return 0;                                                                \
    }
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
// Android entry point
#define VULKAN_EXAMPLE_MAIN(ExampleClassName)                                \
    ExampleClassName *vulkanExample;                            \
    void android_main(android_app *state)                    \
    {                                                        \
        vulkanExample = new ExampleClassName();                 \
        state->userData = vulkanExample;                     \
        state->onAppCmd = ExampleClassName::handleAppCommand;   \
        state->onInputEvent = ExampleClassName::handleAppInput; \
        androidApp = state;                                  \
        vks::android::getDeviceConfig();                     \
        vulkanExample->renderLoop();                         \
        delete (vulkanExample);                              \
    }
#endif
