/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: NonCopyable utility.
 */

#ifndef VULKANEXAMPLES_NONCOPYABLE_H
#define VULKANEXAMPLES_NONCOPYABLE_H

class NonCopyable {
public:
    NonCopyable(const NonCopyable &) = delete;
    void operator=(const NonCopyable &) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

#endif // VULKANEXAMPLES_NONCOPYABLE_H
