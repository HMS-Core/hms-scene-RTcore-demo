# Sample Codes for RTCore
[中文](README_ZH.md) | English

## Contents

* [Introduction](#introduction)
* [Compilation](#compilation)
* [Demos](#demos)
* [Reference Projects](#reference-projects)
* [Technical Support](#technical-support)
* [License](#license)

## Introduction
This project mainly demonstrates how to use RTCore APIs to build hybrid rendering pipelines and simulate partial reflection in ray tracing. It provides two demos: one for drawing a triangle using RTCore APIs, and the other for implementing physically based rendering (PBR) pipelines and simulating partial reflection based on hybrid rendering.
In this project, we make use of Vulkan by referring to SaschaWillems' VulkanExample[[1]](https://github.com/SaschaWillems/Vulkan) project. In the demo of drawing a triangle, the primary ray for each pixel on the screen is calculated by referring to Ray Tracing in One Weekend[[3]](https://raytracing.github.io/books/RayTracingInOneWeekend.html). PBR-related functions are implemented by referring to LearnOpenGL[[2]](https://learnopengl-cn.github.io/07%20PBR/02%20Lighting/#pbr).

## Compilation
The two demos have been tested on Android / Harmony OSplatforms and contain all necessary configuration files.
**Android / Harmony OS platform**
1\. Development Environment
* Android studio 4.0 or later
* ndk 20.1.5948944 or later
* Android SDK 29.0.0 or later

2\. Compile and Run

2.1 using IDE

Open `android` in Android Studio and synchronize the project. Run the project to generate the APK and install it on a phone. Then launch the app to check whether it works as expected.

2.2 using command line

adb connect with mobile phone and run following.

```bat
cd android
call .\gradlew clean
call .\gradlew installDebug 	# or `call .\gradlew assembleDebug` for just build apk
adb shell am start -n "com.huawei.rtcore.vkhybridrt/.VulkanActivity"
```



## Demos
### [Drawing a Triangle using Ray Tracing](examples/triangle)
This demo shows you how to use RTCore APIs to calculate the intersections between primary rays and triangles, to replace the original rasterization method. Below is the detailed procedure.

<img src="images/trianglepipeline_en.png" width="850px">

1\. Use the `generatePrimary` function in `RaytracingTriangle.cpp` to calculate primary rays in the World Coordinate System (WCS) based on the camera coordinates and pixel coordinates.

2\. Use the `updateRayBuffers` function in `RaytracingTriangle.cpp` to copy rays from CPU to GPU.

3\. Build bvh and calculate the ray intersections. This task includes three steps: **performing initialization**, **building an acceleration structure**, and **calculating the ray intersections**, framed in red in the preceding figure. In this demo, RTCore APIs is used in the `VulkanTraceRay` class.


* The `prepare` function obtains **compute queue** and call the `Setup` function of RTCore to perform initialization.
* `buildBVH(vertices, indices, modelMatrix)` first converts vertex coordinates into WCS coordinates, and then calls the `CreateBLAS` and `CreateTLAS` functions to build the bottom-level and top-level acceleration structures, respectively.
* The `trayRay` function calls the **TraceRays** API of RTCore to calculate the ray intersections and get the calculation result. Below is the result.

<img src="images/triangle.png" width="850px">

4\. Use graphics pipelines to virtualize the intersections. That is, call the `triangle/VulkanTrianglePipeline` class to process pixels one by one, obtain the intersection calculation result, and show the intersections in a specific color.

### [Simulating Partial Reflection Based on Hybrid Rendering](examples/hybridRayTracing)
This demo builds a set of ray tracing-based hybrid rendering pipelines to implement partial reflection. Below is the detailed procedure.

<img src="images/hybridpipeline_en.png" width="850px">

1\. Use the traditional rasterization method to implement PBR pipelines (as shown in the area framed by yellow dotted lines). Here is the code:

* `examples/hybridreflection/VulkanScenePipeline`
* `examples/hybridreflection/VulkanImageBasedLighting`
* `examples/hybridreflection/VulkanSkyboxPipeline`;

2\. Add a raytraing pipeline to generate reflection texture  which builds of resources that RTCore needs. Then generate gays, intersect with scene objects, get color and return a reflection texture in shader codes.

* `examples/hybridreRayTracing/RayTracingPass`
* `data/shaders/glsl/hybridRayTracing/raytracing_color.frag`

3\. Use a hybrid pipeline to fuse specular maps and images displayed by original graphics pipelines.

* `examples/hybridreRayTracing/VulkanOnscreenPipeline`

Below is an example of partial reflection:

<img src="images/hybridreflection.png" width="850px">

## Reference Projects
[1] https://github.com/SaschaWillems/Vulkan

[2] [learnOpenGL/PBR](https://learnopengl-cn.github.io/07%20PBR/02%20Lighting/#pbr)

[3] https://raytracing.github.io/books/RayTracingInOneWeekend.html

## Technical Support

If you are still evaluating HMS Core, obtain the latest information about HMS Core and share your insights with other developers at [Reddit](https://www.reddit.com/r/HuaweiDevelopers/.).

- To resolve development issues, please go to [Stack Overflow](https://stackoverflow.com/questions/tagged/huawei-mobile-services?tab=Votes). You can ask questions below the huawei-mobile-services tag, and Huawei R&D experts can solve your problem online on a one-to-one basis.
- To join the developer discussion, please visit [Huawei Developer Forum](https://forums.developer.huawei.com/forumPortal/en/forum/hms-core).

If you have problems using the sample code, submit [issues](https://github.com/HMS-Core/hms-scene-fine--grained-demo/issues) and [pull requests](https://github.com/HMS-Core/hms-scene-fine--grained-demo/pulls) to the repository.

## License
The sample code is licensed under Apache License 2.0. Please refer to  [LICENSE.md](LICENSE.md)  for more information.
