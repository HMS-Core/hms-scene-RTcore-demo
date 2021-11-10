/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Log header.
 */
#ifndef VULKANEXAMPLES_LOG_H
#define VULKANEXAMPLES_LOG_H

#include "TimeStamp.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "rt_demo", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "rt_demo", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "rt_demo", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "rt_demo", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "rt_demo", __VA_ARGS__))
#else
#ifndef LOGTAG
#define LOGTAG "RTCoreDemo"
#endif // LOGTAG

#define LOG(level, ...)                                                                                                \
    do {                                                                                                               \
        fprintf(stdout, "%s %s %s: ", utils::TimeStamp::Now().ToFormattedString().c_str(), (level), LOGTAG);		   \
        fprintf(stdout, __VA_ARGS__);                                                                                  \
        fprintf(stdout, "\n");                                                                                         \
    } while (false);

#define LOG_LEVEL_VERBOSE "V"
#define LOG_LEVEL_DEBUG "D"
#define LOG_LEVEL_INFO "I"
#define LOG_LEVEL_WARN "W"
#define LOG_LEVEL_ERROR "E"

#ifdef NDEBUG
#define LOGV(...)
#define LOGD(...)
#define LOGI(...) LOG(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOGW(...) LOG(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOGE(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#else
#define LOGV(...) LOG(LOG_LEVEL_VERBOSE, __VA_ARGS__)
#define LOGD(...) LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOGI(...) LOG(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOGW(...) LOG(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOGE(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#endif // NDEBUG

#endif // __ANDROID__

#ifdef NDEBUG
#define ASSERT(expr)
#else
#include <cassert>
#define ASSERT(expr) assert((expr))
#endif // NDEBUG

#endif // VULKANEXAMPLES_LOG_H
