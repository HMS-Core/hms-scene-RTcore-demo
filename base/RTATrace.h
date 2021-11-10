/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: android trace
 */
#ifndef HYBRIDRT_TRACE_H
#define HYBRIDRT_TRACE_H

#if defined(__ANDROID__)
#include <android/trace.h>
#define ATRACE_NAME(name) RTATrace renderTrace(name)
#define ATRACE_CALL() ATRACE_NAME(__func__)
class RTATrace {
public:
    inline explicit RTATrace(const char *name)
    {
        ATrace_beginSection(name);
    }

    inline ~RTATrace()
    {
        ATrace_endSection();
    }
};
#elif defined(_WIN32)
#define ATRACE_NAME(name)
#define ATRACE_CALL() ATRACE_NAME(__func__)
#endif

#endif  // HYBRIDRT_TRACE_H
