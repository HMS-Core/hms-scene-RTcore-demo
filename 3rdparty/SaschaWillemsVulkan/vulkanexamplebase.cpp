/*
 * Vulkan Example base class
 *
 * Copyright (C) by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 *  Modified by Huawei, we Fix the bugs when minimize the windows
 *  Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: Vulkan show hit render pipeline head file
 */

#include <unistd.h>
#include "vulkanexamplebase.h"

#define GUI_SIZE 10
#define API_VERSION_OFFSET_MAIN 22
#define API_VERSION_OFFSET_MIDDLE 12
#define DMBITS_PERPEL 32
#define MINTRACK_SIZE 64
#define RADIX 10
#define BUFFER_LEN 20
// 1 For color, 1 for depth
#define ATTACHMENT_NUM 2
#define SUBPASS_NUM 2
#if (defined(VK_USE_PLATFORM_MACOS_MVK) && defined(VK_EXAMPLE_XCODE_GENERATED))
#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <QuartzCore/CAMetalLayer.h>
#include <CoreVideo/CVDisplayLink.h>
#endif

std::vector<const char *> VulkanExampleBase::args;

std::vector<const char *> VulkanExampleBase::getInstanceExtensions()
{
    std::vector<const char *> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};

    // Enable surface extensions depending on os
#if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

    // Get extensions supported by the instance and store for later use
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    if (extCount > 0) {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
            for (VkExtensionProperties extension : extensions) {
                supportedInstanceExtensions.push_back(extension.extensionName);
            }
        }
    }

    // Enabled requested instance extensions
    if (enabledInstanceExtensions.size() > 0) {
        for (const char *enabledExtension : enabledInstanceExtensions) {
            // Output message if requested extension is not available
            if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) ==
                supportedInstanceExtensions.end()) {
                std::cerr << "Enabled instance extension \"" << enabledExtension
                          << "\" is not present at instance level\n";
            }
            instanceExtensions.push_back(enabledExtension);
        }
    }
    return instanceExtensions;
}

VkResult VulkanExampleBase::createInstance(bool enableValidation)
{
    this->settings.validation = enableValidation;

    // Validation can also be forced via a define
#if defined(_VALIDATION)
    this->settings.validation = true;
#endif

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = name.c_str();
    appInfo.pEngineName = name.c_str();
    appInfo.apiVersion = apiVersion;

    std::vector<const char *> instanceExtensions = this->getInstanceExtensions();

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = NULL;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    if (instanceExtensions.size() > 0) {
        if (settings.validation) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    }

    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    // Note that on Android this layer requires at least NDK r20
    const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
    if (settings.validation) {
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
        bool validationLayerPresent = false;
        for (VkLayerProperties layer : instanceLayerProperties) {
            if (strcmp(layer.layerName, validationLayerName) == 0) {
                validationLayerPresent = true;
                break;
            }
        }
        if (validationLayerPresent) {
            instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
            instanceCreateInfo.enabledLayerCount = 1;
        } else {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
        }
    }
    return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
}

void VulkanExampleBase::renderFrame()
{
    VulkanExampleBase::prepareFrame();
    VulkanExampleBase::submitFrame();
}

std::string VulkanExampleBase::getWindowTitle()
{
    std::string device(deviceProperties.deviceName);
    std::string windowTitle("");
    windowTitle = title + " - " + device;
    if (!settings.overlay) {
        windowTitle += " - " + std::to_string(frameCounter) + " fps";
    }
    return windowTitle;
}

void VulkanExampleBase::createCommandBuffers()
{
    // Create one command buffer for each swap chain image and reuse for rendering
    drawCmdBuffers.resize(swapChain.imageCount);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(
        cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
}

void VulkanExampleBase::destroyCommandBuffers()
{
    vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
}

void VulkanExampleBase::createPipelineCache()
{
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void VulkanExampleBase::prepare()
{
    if (vulkanDevice->enableDebugMarkers) {
        vks::debugmarker::setup(device);
    }
    checkAssetPath();
    initSwapchain();
    createCommandPool();
    setupSwapChain();
    createCommandBuffers();
    createSynchronizationPrimitives();
    setupDepthStencil();
    setupRenderPass();
    createPipelineCache();
    setupFrameBuffer();
    settings.overlay = settings.overlay && (!benchmark.active);
    if (settings.overlay) {
        UIOverlay.device = vulkanDevice;
        UIOverlay.queue = queue;
        UIOverlay.shaders = {
                vks::LoadShaders::LoadShader(vulkanDevice, "base/uioverlay.vert.spv",
                                             VK_SHADER_STAGE_VERTEX_BIT),
            vks::LoadShaders::LoadShader(vulkanDevice, "base/uioverlay.frag.spv",
                                         VK_SHADER_STAGE_FRAGMENT_BIT),
        };
        UIOverlay.prepareResources();
        UIOverlay.preparePipeline(pipelineCache, renderPass);
    }
}

void VulkanExampleBase::nextFrame()
{
    auto tStart = std::chrono::high_resolution_clock::now();
    if (viewUpdated) {
        viewUpdated = false;
        viewChanged();
    }

    render();
    frameCounter++;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimer = (float)tDiff / 1000.0f;
    if(frameTimer < FLT_EPSILON){
        frameTimer = FLT_EPSILON;
    }
    camera.update(frameTimer);
    if (camera.moving()) {
        viewUpdated = true;
    }
    // Convert to clamped timer value
    if (!paused) {
        timer += timerSpeed * frameTimer;
        if (timer > 1.0) {
            timer -= 1.0f;
        }
    }
    float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
    if (fpsTimer > 1000.0f) {
        lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
#if defined(_WIN32)
        if (!settings.overlay) {
            std::string windowTitle = getWindowTitle();
            SetWindowText(window, windowTitle.c_str());
        }
#endif
        frameCounter = 0;
        lastTimestamp = tEnd;
    }
    OnNextFrame();
    // Cap UI overlay update rates
    updateOverlay();
}
#if defined(_WIN32)
void VulkanExampleBase::winRender()
{
    MSG msg;
    bool quitMessageReceived = false;
    while (!quitMessageReceived) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                quitMessageReceived = true;
                break;
            }
        }
        if (prepared && !IsIconic(window)) {
            nextFrame();
        }
    }
}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
void VulkanExampleBase::androidRenderFrame()
{
    auto tStart = std::chrono::high_resolution_clock::now();
    render();
    frameCounter++;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimer = tDiff / 1000.0f;
    camera.update(frameTimer);
    // Convert to clamped timer value
    if (!paused) {
        timer += timerSpeed * frameTimer;
        if (timer > 1.0) {
            timer -= 1.0f;
        }
    }
    float fpsTimer = std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count();
    if (fpsTimer > 1000.0f) {
        lastFPS = (float)frameCounter * (1000.0f / fpsTimer);
        frameCounter = 0;
        lastTimestamp = tEnd;
    }
    // call additional func before next frame.
    OnNextFrame();
    // Cap UI overlay update rates/only issue when update requested
    updateOverlay();
    bool updateView = false;

    // Check touch state (for movement)
    if (touchDown) {
        touchTimer += frameTimer;
    }
    if (touchTimer >= 1.0) {
        camera.keys.up = true;
        viewChanged();
    }
    // Check gamepad state
    const float deadZone = 0.0015f;
    // check if gamepad is present
    // time based and relative axis positions
    if (camera.type != Camera::CameraType::firstperson) {
        // Rotate
        if (std::abs(gamePadState.axisLeft.x) > deadZone) {
            camera.rotate(glm::vec3(0.0f, gamePadState.axisLeft.x * 0.5f, 0.0f));
            updateView = true;
        }
        if (std::abs(gamePadState.axisLeft.y) > deadZone) {
            camera.rotate(glm::vec3(gamePadState.axisLeft.y * 0.5f, 0.0f, 0.0f));
            updateView = true;
        }
        // Zoom
        if (std::abs(gamePadState.axisRight.y) > deadZone) {
            camera.translate(glm::vec3(0.0f, 0.0f, gamePadState.axisRight.y * 0.01f));
            updateView = true;
        }
        if (updateView) {
            viewChanged();
        }
    } else {
        updateView = camera.updatePad(gamePadState.axisLeft, gamePadState.axisRight, frameTimer);
        if (updateView) {
            viewChanged();
        }
    }
}
void VulkanExampleBase::androidRender()
{
    while (1) {
        int ident;
        int events;
        struct android_poll_source *source = nullptr;
        bool destroy = false;
        focused = true;
        while ((ident = ALooper_pollAll(focused ? 0 : -1, NULL, &events, (void **)&source)) >= 0) {
            if (source != NULL) {
                source->process(androidApp, source);
            }
            if (androidApp->destroyRequested != 0) {
                LOGD("Android app Destroy requested");
                destroy = true;
                break;
            }
        }
        // App destruction requested
        // Exit loop, example will be destroyed in application main
        if (destroy) {
            ANativeActivity_finish(androidApp->activity);
            break;
        }
        // Render frame
        if (swapChain.prepared) {
            this->androidRenderFrame();
        }
    }
}
#endif

void VulkanExampleBase::renderLoop()
{
    if (benchmark.active) {
        benchmark.run([=] { render(); }, vulkanDevice->properties);
        vkDeviceWaitIdle(device);
        if (benchmark.filename != "") {
            benchmark.saveResults();
        }
        return;
    }

    destWidth = width;
    destHeight = height;
    lastTimestamp = std::chrono::high_resolution_clock::now();
#if defined(_WIN32)
    this->winRender();
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    this->androidRender();
#endif
    // Flush device to make sure all resources can be freed
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
}

void VulkanExampleBase::updateOverlay()
{
    if (!settings.overlay)
        return;

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    io.DeltaTime = frameTimer;
    io.MousePos = ImVec2(mousePos.x, mousePos.y);
    io.MouseDown[0] = mouseButtons.left;
    io.MouseDown[1] = mouseButtons.right;
    ImGui::NewFrame();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowBgAlpha(UIOverlay.overlayerAlpha);
    ImGui::SetNextWindowPos(ImVec2(GUI_SIZE, GUI_SIZE));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Vulkan Example", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextUnformatted(title.c_str());
    ImGui::TextUnformatted(deviceProperties.deviceName);
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * UIOverlay.scale));
#endif
    ImGui::PushItemWidth(110.0f * UIOverlay.scale);
    OnUpdateUIOverlay(&UIOverlay);
    ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    ImGui::PopStyleVar();
#endif
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::Render();
    if (UIOverlay.update() || UIOverlay.updated) {
        buildCommandBuffers();
        UIOverlay.updated = false;
    }
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    if (mouseButtons.left) {
        mouseButtons.left = false;
    }
#endif
}

void VulkanExampleBase::drawUI(const VkCommandBuffer commandBuffer)
{
    if (settings.overlay) {
        const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
        const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        UIOverlay.draw(commandBuffer);
    }
}

void VulkanExampleBase::prepareFrame()
{
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[frameIdx], VK_TRUE, UINT64_MAX));

    // Acquire the next image from the swap chain
    VkResult result = swapChain.acquireNextImage(semaphores[frameIdx].presentComplete, &currentBuffer);
    // Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for
    // presentation (SUBOPTIMAL)
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
        windowResize();
    } else {
        VK_CHECK_RESULT(result);
    }

    if (imageFences[currentBuffer] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imageFences[currentBuffer], VK_TRUE, UINT64_MAX);
    }
    imageFences[currentBuffer] = waitFences[frameIdx];
}

void VulkanExampleBase::submitFrame(useconds_t waits)
{
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores[frameIdx].presentComplete;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores[frameIdx].renderComplete;
    vkResetFences(device, 1, &waitFences[frameIdx]);
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[frameIdx]));

    VkResult result = swapChain.queuePresent(queue, currentBuffer, semaphores[frameIdx].renderComplete);
    if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swap chain is no longer compatible with the surface and needs to be recreated
            windowResize();
            return;
        } else {
            VK_CHECK_RESULT(result);
        }
    }

    ++frameIdx;
    if (frameIdx == m_maxFrameInFlight) {
        frameIdx = 0;
    }
    usleep(waits);          // additional waiting, hack for frame tear on snapdragon 888 chip.
//    VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

void VulkanExampleBase::parseCommandArgs()
{
    // Command line arguments
    commandLineParser.parse(args);
    if (commandLineParser.isSet("help")) {
#if defined(_WIN32)
        setupConsole("Vulkan example");
#endif
        commandLineParser.printHelp();
        std::cin.get();
        exit(0);
    }
    if (commandLineParser.isSet("validation")) {
        settings.validation = true;
    }
    if (commandLineParser.isSet("vsync")) {
        settings.vsync = true;
    }
    if (commandLineParser.isSet("height")) {
        height = commandLineParser.getValueAsInt("height", width);
    }
    if (commandLineParser.isSet("width")) {
        width = commandLineParser.getValueAsInt("width", width);
    }
    if (commandLineParser.isSet("fullscreen")) {
        settings.fullscreen = true;
    }
    if (commandLineParser.isSet("shaders")) {
        std::string value = commandLineParser.getValueAsString("shaders", "glsl");
        if ((value != "glsl") && (value != "hlsl")) {
            std::cerr << "Shader type must be one of 'glsl' or 'hlsl'\n";
        } else {
            shaderDir = value;
        }
    }
    if (commandLineParser.isSet("benchmark")) {
        benchmark.active = true;
        vks::tools::errorModeSilent = true;
    }
    if (commandLineParser.isSet("benchmarkwarmup")) {
        benchmark.warmup = commandLineParser.getValueAsInt("benchmarkwarmup", benchmark.warmup);
    }
    if (commandLineParser.isSet("benchmarkruntime")) {
        benchmark.duration = commandLineParser.getValueAsInt("benchmarkruntime", benchmark.duration);
    }
    if (commandLineParser.isSet("benchmarkresultfile")) {
        benchmark.filename = commandLineParser.getValueAsString("benchmarkresultfile", benchmark.filename);
    }
    if (commandLineParser.isSet("benchmarkframetimes")) {
        benchmark.outputFrameTimes = true;
    }
}

VulkanExampleBase::VulkanExampleBase(bool enableValidation)
{
    settings.validation = enableValidation;
    // Command line arguments
    this->parseCommandArgs();
#if defined(_WIN32)
    // Enable console if validation is active, debug message callback will output to it
    if (this->settings.validation) {
        setupConsole("Vulkan example");
    }
    setupDPIAwareness();
#endif
#if defined(__ANDROID__)
    // set m_maxFrameInFlight = 3 for vivo samsung exynos chip
    char tmpValue[BUFFER_LEN] = {0};
    const char targetValue[] = "samsung";
    int len = __system_property_get("persist.vivo.voicewakeup.chip.type", tmpValue);
    if (len > 0 && strcmp(tmpValue, targetValue) == 0) {
        m_maxFrameInFlight = 3;
        LOGW("Init info: set maxFrameInFlight = 3 (for samsung chip on vivo).");
    }
#endif
}

VulkanExampleBase::~VulkanExampleBase() noexcept
{
    // Clean up Vulkan resources
    swapChain.cleanup();
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    destroyCommandBuffers();
    vkDestroyRenderPass(device, renderPass, nullptr);
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
    }
    // Destroy shaders
    vks::LoadShaders::Destroy(vulkanDevice);
    vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.mem, nullptr);
    vkDestroyPipelineCache(device, pipelineCache, nullptr);
    vkDestroyCommandPool(device, cmdPool, nullptr);
    for (auto &semaphore : semaphores) {
        vkDestroySemaphore(device, semaphore.presentComplete, nullptr);
        vkDestroySemaphore(device, semaphore.renderComplete, nullptr);
    }
    for (auto &fence : waitFences) {
        vkDestroyFence(device, fence, nullptr);
    }
    if (settings.overlay) {
        UIOverlay.freeResources();
    }
    delete vulkanDevice;
    if (settings.validation) {
        vks::debug::freeDebugCallback(instance);
    }
    vkDestroyInstance(instance, nullptr);
}

bool VulkanExampleBase::getPhysicDevice()
{
    VkResult err;
    // Physical device
    uint32_t gpuCount = 0;
    // Get number of available physical devices
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
    if (gpuCount == 0) {
        vks::tools::exitFatal("No device with Vulkan support found", -1);
        return false;
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
    err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
    if (err) {
        vks::tools::exitFatal("Could not enumerate physical devices : \n" + vks::tools::errorString(err), err);
        return false;
    }
    // GPU selection
    // Select physical device to be used for the Vulkan example
    // Defaults to the first device unless specified by command line
    uint32_t selectedDevice = 0;
#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
    // GPU selection via command line argument
    if (commandLineParser.isSet("gpuselection")) {
        uint32_t index = commandLineParser.getValueAsInt("gpuselection", 0);
        if (index > gpuCount - 1) {
            std::cerr << "Selected device index " << index
                      << " is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)"
                      << "\n";
        } else {
            selectedDevice = index;
        }
    }
    if (commandLineParser.isSet("gpulist")) {
        std::cout << "Available Vulkan devices"
                  << "\n";
        for (uint32_t i = 0; i < gpuCount; i++) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
            std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
            std::cout << " Type: " << vks::tools::physicalDeviceTypeString(deviceProperties.deviceType) << "\n";
            std::cout << " API: " << (deviceProperties.apiVersion >> API_VERSION_OFFSET_MAIN) << "."
                      << ((deviceProperties.apiVersion >> API_VERSION_OFFSET_MIDDLE) & 0x3ff) << "."
                      << (deviceProperties.apiVersion & 0xfff) << "\n";
        }
    }
#endif
    physicalDevice = physicalDevices[selectedDevice];
    return true;
}

bool VulkanExampleBase::initVulkan()
{
    VkResult err;
    // Vulkan instance
    err = createInstance(settings.validation);
    if (err) {
        vks::tools::exitFatal("Could not create Vulkan instance : \n" + vks::tools::errorString(err), err);
        return false;
    }
    // If requested, we enable the default validation layers for debugging
    if (settings.validation) {
        // The report flags determine what type of messages for the layers will be displayed
        // For validating (debugging) an application the error and warning bits should suffice
        VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        // Additional flags include performance info, loader and layer debug messages, etc.
        vks::debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
    }
    // Physical device
    if (!this->getPhysicDevice()) {
        return false;
    }
    // Store properties (including limits), features and memory properties of the physical device (so that examples can
    // check against them)
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
    // Derived examples can override this to set actual features (based on above readings) to enable for logical device
    // creation
    getEnabledFeatures();
    // Vulkan device creation
    // This is handled by a separate class that gets a logical device representation
    // and encapsulates functions related to a device
    vulkanDevice = new vks::VulkanDevice(physicalDevice);
    VkResult res = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
    if (res != VK_SUCCESS) {
        vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(res), res);
        return false;
    }
    device = vulkanDevice->logicalDevice;
    // Get a graphics queue from the device
    vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphics, 0, &queue);
    // Find a suitable depth format
    VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
    swapChain.connect(instance, physicalDevice, device);
    return true;
}

#if defined(_WIN32)
// Win32 : Sets up a console window and redirects standard output to it
void VulkanExampleBase::setupConsole(std::string title)
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE *stream = nullptr;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    freopen_s(&stream, "CONOUT$", "w+", stderr);
    SetConsoleTitle(TEXT(title.c_str()));
}

void VulkanExampleBase::setupDPIAwareness()
{
}

HWND VulkanExampleBase::adjustWindows(HINSTANCE hinstance, RECT &outWindowRect)
{
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (settings.fullscreen) {
        if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight)) {
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
            dmScreenSettings.dmSize = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth = width;
            dmScreenSettings.dmPelsHeight = height;
            dmScreenSettings.dmBitsPerPel = DMBITS_PERPEL;
            dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
                if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error",
                               MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
                    settings.fullscreen = false;
                } else {
                    return nullptr;
                }
            }
            screenWidth = width;
            screenHeight = height;
        }
    }
    DWORD dwExStyle;
    DWORD dwStyle;
    if (settings.fullscreen) {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    } else {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }
    outWindowRect.left = 0L;
    outWindowRect.top = 0L;
    outWindowRect.right = settings.fullscreen ? (long)screenWidth : (long)width;
    outWindowRect.bottom = settings.fullscreen ? (long)screenHeight : (long)height;
    AdjustWindowRectEx(&outWindowRect, dwStyle, FALSE, dwExStyle);
    std::string windowTitle = getWindowTitle();
    window = CreateWindowEx(0, name.c_str(), windowTitle.c_str(), dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                            outWindowRect.right - outWindowRect.left, outWindowRect.bottom - outWindowRect.top, NULL,
                            NULL, hinstance, NULL);
    return window;
}

HWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
    this->windowInstance = hinstance;
    WNDCLASSEX wndClass;
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = wndproc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hinstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = name.c_str();
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    if (!RegisterClassEx(&wndClass)) {
        std::cout << "Could not register window class!\n";
        fflush(stdout);
        exit(1);
    }
    RECT windowRect;
    if (this->adjustWindows(hinstance, windowRect) == nullptr) {
        std::cout << "Could not Adjust the window class!\n";
        return nullptr;
    }
    if (!settings.fullscreen) {
        // Center on screen
        uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
        uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
        SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
    if (!window) {
        printf("Could not create window!\n");
        fflush(stdout);
        return nullptr;
    }
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(window);
    return window;
}

void keyPressFirstPerson(WPARAM wParam, Camera &camera, bool down)
{
    switch (wParam) {
        case KEY_W:
            camera.keys.up = down;
            break;
        case KEY_S:
            camera.keys.down = down;
            break;
        case KEY_A:
            camera.keys.left = down;
            break;
        case KEY_D:
            camera.keys.right = down;
            break;
        default:
            break;
    }
}

void VulkanExampleBase::keyDown(WPARAM wParam)
{
    switch (wParam) {
        case KEY_P:
            paused = !paused;
            break;
        case KEY_F1:
            if (settings.overlay) {
                UIOverlay.visible = !UIOverlay.visible;
            }
            break;
        case KEY_ESCAPE:
            PostQuitMessage(0);
            break;
        default:
            break;
    }
    if (camera.type == Camera::firstperson) {
        keyPressFirstPerson(wParam, camera, true);
    }
    keyPressed((uint32_t)wParam);
}
void VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CLOSE:
            prepared = false;
            DestroyWindow(hWnd);
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            ValidateRect(window, NULL);
            break;
        case WM_KEYDOWN:
            this->keyDown(wParam);
            break;
        case WM_KEYUP:
            if (camera.type == Camera::firstperson) {
                keyPressFirstPerson(wParam, camera, false);
            }
            break;
        case WM_LBUTTONDOWN:
            mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            mouseButtons.left = true;
            break;
        case WM_RBUTTONDOWN:
            mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            mouseButtons.right = true;
            break;
        case WM_MBUTTONDOWN:
            mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            mouseButtons.middle = true;
            break;
        case WM_LBUTTONUP:
            mouseButtons.left = false;
            break;
        case WM_RBUTTONUP:
            mouseButtons.right = false;
            break;
        case WM_MBUTTONUP:
            mouseButtons.middle = false;
            break;
        case WM_MOUSEWHEEL: {
            short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            camera.translate(glm::vec3(0.0f, 0.0f, -(float)wheelDelta * 0.005f * camera.movementSpeed));
            viewUpdated = true;
            break;
        }
        case WM_MOUSEMOVE: {
            handleMouseMove(LOWORD(lParam), HIWORD(lParam));
            break;
        }
        case WM_SIZE:
            if ((prepared) && (wParam != SIZE_MINIMIZED)) {
                if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
                    destWidth = LOWORD(lParam);
                    destHeight = HIWORD(lParam);
                    windowResize();
                }
            }
            break;
        case WM_GETMINMAXINFO: {
            LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
            minMaxInfo->ptMinTrackSize.x = MINTRACK_SIZE;
            minMaxInfo->ptMinTrackSize.y = MINTRACK_SIZE;
            break;
        }
        case WM_ENTERSIZEMOVE:
            resizing = true;
            break;
        case WM_EXITSIZEMOVE:
            resizing = false;
            break;
        default:
            break;
    }
}
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
int32_t VulkanExampleBase::handleAppInput(struct android_app *app, AInputEvent *event)
{
    VulkanExampleBase *vulkanExample = reinterpret_cast<VulkanExampleBase *>(app->userData);
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t eventSource = AInputEvent_getSource(event);
        switch (eventSource) {
            case AINPUT_SOURCE_JOYSTICK: {
                // Left thumbstick
                vulkanExample->gamePadState.axisLeft.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
                vulkanExample->gamePadState.axisLeft.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
                // Right thumbstick
                vulkanExample->gamePadState.axisRight.x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
                vulkanExample->gamePadState.axisRight.y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);
                break;
            }
            case AINPUT_SOURCE_TOUCHSCREEN: {
                int32_t action = AMotionEvent_getAction(event);
                switch (action) {
                    case AMOTION_EVENT_ACTION_UP: {
                        vulkanExample->lastTapTime = AMotionEvent_getEventTime(event);
                        vulkanExample->touchPos.x = AMotionEvent_getX(event, 0);
                        vulkanExample->touchPos.y = AMotionEvent_getY(event, 0);
                        vulkanExample->touchTimer = 0.0;
                        vulkanExample->touchDown = false;
                        vulkanExample->camera.keys.up = false;
                        // Detect single tap
                        int64_t eventTime = AMotionEvent_getEventTime(event);
                        int64_t downTime = AMotionEvent_getDownTime(event);
                        if (eventTime - downTime <= vks::android::TAP_TIMEOUT) {
                            float deadZone =
                                (160.f / vks::android::screenDensity) * vks::android::TAP_SLOP * vks::android::TAP_SLOP;
                            float x = AMotionEvent_getX(event, 0) - vulkanExample->touchPos.x;
                            float y = AMotionEvent_getY(event, 0) - vulkanExample->touchPos.y;
                            if ((x * x + y * y) < deadZone) {
                                vulkanExample->mouseButtons.left = true;
                            }
                        };
                        return 1;
                        break;
                    }
                    case AMOTION_EVENT_ACTION_DOWN: {
                        // Detect double tap
                        int64_t eventTime = AMotionEvent_getEventTime(event);
                        if (eventTime - vulkanExample->lastTapTime <= vks::android::DOUBLE_TAP_TIMEOUT) {
                            float deadZone = (160.f / vks::android::screenDensity) * vks::android::DOUBLE_TAP_SLOP *
                                             vks::android::DOUBLE_TAP_SLOP;
                            float x = AMotionEvent_getX(event, 0) - vulkanExample->touchPos.x;
                            float y = AMotionEvent_getY(event, 0) - vulkanExample->touchPos.y;
                            if ((x * x + y * y) < deadZone) {
                                vulkanExample->keyPressed(TOUCH_DOUBLE_TAP);
                                vulkanExample->touchDown = false;
                            }
                        } else {
                            vulkanExample->touchDown = true;
                        }
                        vulkanExample->touchPos.x = AMotionEvent_getX(event, 0);
                        vulkanExample->touchPos.y = AMotionEvent_getY(event, 0);
                        vulkanExample->mousePos.x = AMotionEvent_getX(event, 0);
                        vulkanExample->mousePos.y = AMotionEvent_getY(event, 0);
                        break;
                    }
                    case AMOTION_EVENT_ACTION_MOVE: {
                        bool handled = false;
                        if (vulkanExample->settings.overlay) {
                            ImGuiIO &io = ImGui::GetIO();
                            handled = io.WantCaptureMouse;
                        }
                        if (!handled) {
                            int32_t eventX = AMotionEvent_getX(event, 0);
                            int32_t eventY = AMotionEvent_getY(event, 0);

                            float deltaX = (float)(vulkanExample->touchPos.y - eventY) *
                                           vulkanExample->camera.rotationSpeed * 0.5f;
                            float deltaY = (float)(vulkanExample->touchPos.x - eventX) *
                                           vulkanExample->camera.rotationSpeed * 0.5f;

                            vulkanExample->camera.rotate(glm::vec3(deltaX, 0.0f, 0.0f));
                            vulkanExample->camera.rotate(glm::vec3(0.0f, -deltaY, 0.0f));

                            vulkanExample->viewChanged();

                            vulkanExample->touchPos.x = eventX;
                            vulkanExample->touchPos.y = eventY;
                        }
                        break;
                    }
                    default:
                        return 1;
                        break;
                }
            }
            default:
                return 1;
                break;
        }
    }

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        int32_t keyCode = AKeyEvent_getKeyCode((const AInputEvent *)event);
        int32_t action = AKeyEvent_getAction((const AInputEvent *)event);
        int32_t button = 0;

        if (action == AKEY_EVENT_ACTION_UP)
            return 0;
        switch (keyCode) {
            case AKEYCODE_BUTTON_A:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_A);
                break;
            case AKEYCODE_BUTTON_B:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_B);
                break;
            case AKEYCODE_BUTTON_X:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_X);
                break;
            case AKEYCODE_BUTTON_Y:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_Y);
                break;
            case AKEYCODE_BUTTON_L1:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_L1);
                break;
            case AKEYCODE_BUTTON_R1:
                vulkanExample->keyPressed(GAMEPAD_BUTTON_R1);
                break;
            case AKEYCODE_BUTTON_START:
                vulkanExample->paused = !vulkanExample->paused;
                break;
            default:
                break;
        };
        LOGD("Button %d pressed", keyCode);
    }
    return 0;
}

void VulkanExampleBase::handleAppCommand(android_app *app, int32_t cmd)
{
    VulkanExampleBase *vulkanExample = reinterpret_cast<VulkanExampleBase *>(app->userData);
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            LOGD("APP_CMD_SAVE_STATE");
            break;
        case APP_CMD_INIT_WINDOW:
            LOGD("APP_CMD_INIT_WINDOW");
            if (androidApp->window != NULL) {
                if (!vulkanExample->prepared) {
                    if (vulkanExample->initVulkan()) {
                        vulkanExample->prepare();
                    } else {
                        LOGE("Could not initialize Vulkan, exiting!");
                        androidApp->destroyRequested = 1;
                    }
                } else if (!vulkanExample->swapChain.prepared) {
                    vulkanExample->initSwapchain();
                    vulkanExample->windowResize();
                }
            } else {
                LOGE("No window assigned!");
            }
            vulkanExample->waitGPUIdle();
            break;
        case APP_CMD_LOST_FOCUS:
            LOGD("APP_CMD_LOST_FOCUS");
            vulkanExample->waitGPUIdle();
            vulkanExample->focused = false;
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGD("APP_CMD_GAINED_FOCUS");
            vulkanExample->waitGPUIdle();
            vulkanExample->focused = true;
            break;
        case APP_CMD_TERM_WINDOW:
            // Window is hidden or closed, clean up resources
            LOGD("APP_CMD_TERM_WINDOW");
            vulkanExample->waitGPUIdle();
            if (vulkanExample->prepared) {
                vulkanExample->swapChain.cleanup();
            }
            break;
        default:
            break;
    }
}
#endif

void VulkanExampleBase::viewChanged()
{
}

void VulkanExampleBase::keyPressed(uint32_t)
{
}

void VulkanExampleBase::mouseMoved(double x, double y, bool &handled)
{
}

void VulkanExampleBase::buildCommandBuffers()
{
}

void VulkanExampleBase::createSynchronizationPrimitives()
{
    // Create semaphore objects
    VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
    semaphores.resize(m_maxFrameInFlight);
    for (auto &semaphore : semaphores) {
        // Create a semaphore used to synchronize image presentation
        // Ensures that the image is displayed before we start submitting new commands to the queue
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore.presentComplete));
        // Create a semaphore used to synchronize command submission
        // Ensures that the image is not presented until all commands have been submitted and executed
        VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore.renderComplete));
    }

    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    waitFences.resize(m_maxFrameInFlight);
    for (auto &fence : waitFences) {
        VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }

    imageFences.resize(drawCmdBuffers.size());
}

void VulkanExampleBase::createCommandPool()
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
}

void VulkanExampleBase::setupDepthStencil()
{
    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = depthFormat;
    imageCI.extent = {width, height, 1};
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));
    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex =
        vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem));
    VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));
    VkImageViewCreateInfo imageViewCI{};
    imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCI.image = depthStencil.image;
    imageViewCI.format = depthFormat;
    imageViewCI.subresourceRange.baseMipLevel = 0;
    imageViewCI.subresourceRange.levelCount = 1;
    imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    // Stencil aspect should only be set on depth + stencil formats
    // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
        imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
}

void VulkanExampleBase::setupFrameBuffer()
{
    VkImageView attachments[2];
    // Depth/Stencil attachment is the same for all frame buffers
    attachments[1] = depthStencil.view;
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = ATTACHMENT_NUM;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;
    // Create frame buffers for every swap chain image
    frameBuffers.resize(swapChain.imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        attachments[0] = swapChain.buffers[i].view;
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

void VulkanExampleBase::setupRenderPass()
{
    vkpass::VulkanRenderPass vulkanRenderPass(vulkanDevice);
    vulkanRenderPass.AddAttachmentDescriptions(swapChain.colorFormat,
                                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vulkanRenderPass.AddAttachmentDescriptions(depthFormat,
                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vulkanRenderPass.PrepareRenderPass();
    VK_CHECK_RESULT(vkCreateRenderPass(device, &vulkanRenderPass.renderPassCreateInfo, nullptr, &renderPass));
}

void VulkanExampleBase::getEnabledFeatures()
{
}

void VulkanExampleBase::waitGPUIdle()
{
    if (!prepared) {
        return;
    }
    vkQueueWaitIdle(queue);
    vkDeviceWaitIdle(device);
}

void VulkanExampleBase::windowResize()
{
    if (!prepared) {
        return;
    }
    prepared = false;
    resized = true;
    // Ensure all operations on the device have been finished before destroying resources
    vkDeviceWaitIdle(device);
    // Recreate swap chain
    width = destWidth;
    height = destHeight;
    setupSwapChain();
    // Recreate the frame buffers
    vkDestroyImageView(device, depthStencil.view, nullptr);
    vkDestroyImage(device, depthStencil.image, nullptr);
    vkFreeMemory(device, depthStencil.mem, nullptr);
    setupDepthStencil();
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
    }
    setupFrameBuffer();
    if ((width > 0.0f) && (height > 0.0f)) {
        if (settings.overlay) {
            UIOverlay.resize(width, height);
        }
    }
    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    destroyCommandBuffers();
    createCommandBuffers();
    buildCommandBuffers();
    vkDeviceWaitIdle(device);
    if ((width > 0.0f) && (height > 0.0f)) {
        camera.updateAspectRatio((float)width / (float)height);
    }
    // Notify derived class
    windowResized();
    viewChanged();
    prepared = true;
}

void VulkanExampleBase::handleMouseMove(int32_t x, int32_t y)
{
    int32_t dx = (int32_t)mousePos.x - x;
    int32_t dy = (int32_t)mousePos.y - y;
    bool handled = false;
    if (settings.overlay) {
        ImGuiIO &io = ImGui::GetIO();
        handled = io.WantCaptureMouse;
    }
    mouseMoved((float)x, (float)y, handled);
    if (handled) {
        mousePos = glm::vec2((float)x, (float)y);
        return;
    }
    if (mouseButtons.left) {
        camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
        viewUpdated = true;
    }
    if (mouseButtons.right) {
        camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f * camera.movementSpeed));
        viewUpdated = true;
    }
    if (mouseButtons.middle) {
        camera.translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
        viewUpdated = true;
    }
    mousePos = glm::vec2((float)x, (float)y);
}

void VulkanExampleBase::windowResized()
{
}

void VulkanExampleBase::initSwapchain()
{
#if defined(_WIN32)
    swapChain.initSurface(windowInstance, window);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    swapChain.initSurface(androidApp->window);
#endif
}

void VulkanExampleBase::setupSwapChain()
{
    swapChain.create(&width, &height, settings.vsync);
}

void VulkanExampleBase::OnUpdateUIOverlay(vks::UIOverlay *overlay) {}

void VulkanExampleBase::OnNextFrame() {}

// Command line argument parser class
CommandLineParser::CommandLineParser()
{
    add("help", {"--help"}, 0, "Show help");
    add("validation", {"-v", "--validation"}, 0, "Enable validation layers");
    add("vsync", {"-vs", "--vsync"}, 0, "Enable V-Sync");
    add("fullscreen", {"-f", "--fullscreen"}, 0, "Start in fullscreen mode");
    add("width", {"-w", "--width"}, 1, "Set window width");
    add("height", {"-h", "--height"}, 1, "Set window height");
    add("shaders", {"-s", "--shaders"}, 1, "Select shader type to use (glsl or hlsl)");
    add("gpuselection", {"-g", "--gpu"}, 1, "Select GPU to run on");
    add("gpulist", {"-gl", "--listgpus"}, 0, "Display a list of available Vulkan devices");
    add("benchmark", {"-b", "--benchmark"}, 0, "Run example in benchmark mode");
    add("benchmarkwarmup", {"-bw", "--benchwarmup"}, 1, "Set warmup time for benchmark mode in seconds");
    add("benchmarkruntime", {"-br", "--benchruntime"}, 1, "Set duration time for benchmark mode in seconds");
    add("benchmarkresultfile", {"-bf", "--benchfilename"}, 1, "Set file name for benchmark results");
    add("benchmarkresultframes", {"-bt", "--benchframetimes"}, 1, "Save frame times to benchmark results file");
}

void CommandLineParser::add(std::string name, std::vector<std::string> commands, bool hasValue, std::string help)
{
    options[name].commands = commands;
    options[name].help = help;
    options[name].set = false;
    options[name].hasValue = hasValue;
    options[name].value = "";
}

void CommandLineParser::printHelp()
{
    std::cout << "Available command line options:\n";
    for (auto option : options) {
        std::cout << " ";
        for (size_t i = 0; i < option.second.commands.size(); i++) {
            std::cout << option.second.commands[i];
            if (i < option.second.commands.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << ": " << option.second.help << "\n";
    }
    std::cout << "Press any key to close...";
}

void CommandLineParser::parse(std::vector<const char *> arguments)
{
    bool printHelp = false;
    // Known arguments
    for (auto &option : options) {
        for (auto &command : option.second.commands) {
            for (size_t i = 0; i < arguments.size(); i++) {
                if (strcmp(arguments[i], command.c_str()) == 0) {
                    option.second.set = true;
                    // Get value
                    if (option.second.hasValue) {
                        if (arguments.size() > i + 1) {
                            option.second.value = arguments[i + 1];
                        }
                        if (option.second.value == "") {
                            printHelp = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    // Print help for unknown arguments or missing argument values
    if (printHelp) {
        options["help"].set = true;
    }
}

bool CommandLineParser::isSet(std::string name)
{
    return ((options.find(name) != options.end()) && options[name].set);
}

std::string CommandLineParser::getValueAsString(std::string name, std::string defaultValue)
{
    std::string value = options[name].value;
    return (value != "") ? value : defaultValue;
}

int32_t CommandLineParser::getValueAsInt(std::string name, int32_t defaultValue)
{
    std::string value = options[name].value;
    if (value != "") {
        char *numConvPtr = nullptr;
        int32_t intVal = strtol(value.c_str(), &numConvPtr, RADIX);
        return (intVal > 0) ? intVal : defaultValue;
    } else {
        return defaultValue;
    }
    return int32_t();
}
