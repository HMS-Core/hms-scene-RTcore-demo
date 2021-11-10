/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: TimeStamp utility.
 */

#ifndef VULKANEXAMPLES_TIMESTAMP_H
#define VULKANEXAMPLES_TIMESTAMP_H

#include <sstream>
#include <string>
#include <ctime>

namespace utils {
constexpr int NANO_SECS_PER_MICROSECOND = 1000;
constexpr int MICRO_SECS_PER_MILLISECOND = 1000;
constexpr int MILLI_SECS_PER_SECOND = 1000;
constexpr int NANO_SECS_PER_MILLISECOND = NANO_SECS_PER_MICROSECOND * MICRO_SECS_PER_MILLISECOND;
constexpr int MICRO_SECS_PER_SECOND = MICRO_SECS_PER_MILLISECOND * MILLI_SECS_PER_SECOND;
constexpr int NANO_SECS_PER_SECOND = MILLI_SECS_PER_SECOND * NANO_SECS_PER_MILLISECOND;

using TimeType = ::time_t;
enum class TimePrecision { SECOND, MILLI, MICRO, NANO };

class TimeStamp {
public:
    TimeStamp() = default;
    ~TimeStamp() noexcept = default;

    explicit TimeStamp(TimeType nanoSecSinceEpoch) : m_nanoSecSinceEpoch(nanoSecSinceEpoch) {}

    // nano secs since epoch.
    static TimeStamp Now();
    explicit operator TimeType() const
    {
        return m_nanoSecSinceEpoch;
    }
    const TimeType Time() const
    {
        return m_nanoSecSinceEpoch;
    }
    const std::string ToString(TimePrecision precision = TimePrecision::MILLI) const;
    const std::string ToFormattedString(TimePrecision precision = TimePrecision::MILLI) const;

private:
    void AppendAccurateTime(std::stringstream &ss, TimePrecision precision) const;
    TimeType m_nanoSecSinceEpoch = 0;
};
} // namespace utils

#endif // VULKANEXAMPLES_TIMESTAMP_H
