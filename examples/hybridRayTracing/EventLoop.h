/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: Event loop declaration.
 */

#ifndef VULKANEXAMPLES_EVENTLOOP_H
#define VULKANEXAMPLES_EVENTLOOP_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

#include "NonCopyable.h"

namespace rt {
class EventLoop : private NonCopyable {
public:
    using Functor = std::function<void()>;

    EventLoop() : m_quit(false), m_wakeup(false) {}
    ~EventLoop() noexcept
    {
        Quit();
    }

    void Loop();
    void Quit()
    {
        m_quit = true;
        Wakeup();
    }

    void RunInLoop(Functor func);

private:
    void DoPendingFunctors();

    void Wakeup()
    {
        m_wakeup = true;
        m_wakeupCondition.notify_one();
    }

    std::atomic<bool> m_quit;
    std::atomic<bool> m_wakeup;
    std::condition_variable m_wakeupCondition;
    std::vector<Functor> m_pendingFunctors;

    std::mutex m_mutex;
};
} // namespace rt

#endif // VULKANEXAMPLES_EVENTLOOP_H
