/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: head file for android platform
 */

#ifndef VULKANEXAMPLES_VULKANANDROID_H
#define VULKANEXAMPLES_VULKANANDROID_H
#include "vulkan/vulkan.h"

#if defined(__ANDROID__)
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/configuration.h>
#include <memory>
#include <string>

// Global reference to android application object
extern android_app* androidApp;
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "vulkanExample", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "vulkanExample", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "vulkanExample", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "vulkanExample", __VA_ARGS__))


namespace vks
{
	namespace android
	{
		/* @brief Touch control thresholds from Android NDK samples */
		const int32_t DOUBLE_TAP_TIMEOUT = 300 * 1000000;
		const int32_t TAP_TIMEOUT = 180 * 1000000;
		const int32_t DOUBLE_TAP_SLOP = 100;
		const int32_t TAP_SLOP = 8;

		/** @brief Density of the device screen (in DPI) */
		extern int32_t screenDensity;

		void getDeviceConfig();
		void showAlert(const char* message);
	}
}

#endif

#endif // VULKANEXAMPLES_VULKANANDROID_H
