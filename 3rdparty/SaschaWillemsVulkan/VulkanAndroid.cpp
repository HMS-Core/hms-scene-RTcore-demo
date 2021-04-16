/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: file for android platform
 */

#include "VulkanAndroid.h"

#if defined(__ANDROID__)
android_app* androidApp;

int32_t vks::android::screenDensity;

namespace vks
{
	namespace android
	{
		void getDeviceConfig()
		{
			// Screen density
			AConfiguration* config = AConfiguration_new();
			AConfiguration_fromAssetManager(config, androidApp->activity->assetManager);
			vks::android::screenDensity = AConfiguration_getDensity(config);
			AConfiguration_delete(config);
		}

		// Displays a native alert dialog using JNI
		void showAlert(const char* message) {
			JNIEnv* jni;
			androidApp->activity->vm->AttachCurrentThread(&jni, NULL);

			jstring jmessage = jni->NewStringUTF(message);

			jclass clazz = jni->GetObjectClass(androidApp->activity->clazz);
			// Signature has to match java implementation (arguments)
			jmethodID methodID = jni->GetMethodID(clazz, "showAlert", "(Ljava/lang/String;)V");
			jni->CallVoidMethod(androidApp->activity->clazz, methodID, jmessage);
			jni->DeleteLocalRef(jmessage);

			androidApp->activity->vm->DetachCurrentThread();
			return;
		}
	}
}
#endif
