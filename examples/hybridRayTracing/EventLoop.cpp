/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Event loop implementation.
 */

#include "EventLoop.h"

namespace rt {
void EventLoop::Loop()
{
    while (!m_quit) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wakeupCondition.wait(lock, [this]() { return m_wakeup == true; });
            m_wakeup = false;
        }

        DoPendingFunctors();
    }
}

void EventLoop::RunInLoop(Functor func)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingFunctors.push_back(std::move(func));

    Wakeup();
}

void EventLoop::DoPendingFunctors()
{
    std::vector<Functor> functors;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        functors.swap(m_pendingFunctors);
    }

    for (const auto &functor : functors) {
        functor();
    }
}
} // namespace rt
